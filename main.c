// 放在所有 include 之前，启用 GNU 扩展（pthread_timedjoin_np）
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <limits.h>

#include "log/log.h"
#include "config/config.h"
#include "device/device.h"
#include "grpcclient/register.h"       
#include "grpcserver/server.h"        
#include "httpserver/httpserver.h"     
#include "common/configmaptype.h"
#include "common/const.h"
#include "data/dbmethod/mysql/mysql_client.h"  // 新增

static volatile int running = 1;
static DeviceManager *g_deviceManager = NULL;
static GrpcServer *g_grpcServer = NULL;          
static RestServer *g_httpServer = NULL;          
static MySQLDataBaseConfig *g_mysql = NULL;    // 新增
static pthread_t g_grpcThread = 0;
static char g_grpcSockPath[PATH_MAX] = {0};
// 新增：设备启动线程句柄
static pthread_t g_devStartThread = 0;

static void cleanup_resources(void);
static void signal_handler(int sig) {
    static int signal_received = 0;
    signal_received++;
    if (signal_received == 1) {
        log_info("Received signal %d, shutting down gracefully...", sig);
        running = 0;
        return;
    }
    // 第二次 Ctrl+C 直接硬退，避免卡死
    log_warn("Received signal %d again, force exiting now.", sig);
    log_flush();
    _exit(128 + sig);
}
static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); 
}

static void cleanup_resources(void) {
    log_info("Cleaning up resources...");

    // 1) HTTP
    log_info("[cleanup] stopping HTTP...");
    if (g_httpServer) {
        rest_server_stop(g_httpServer);
        rest_server_free(g_httpServer);
        g_httpServer = NULL;
    }
    log_info("[cleanup] HTTP done");

    // 2) 设备：先发 stop
    log_info("[cleanup] stopping devices...");
    if (g_deviceManager) {
        device_manager_stop_all(g_deviceManager);
    }
    log_info("[cleanup] devices stop_all issued");

    // 2.1) 限时等待设备启动线程退出；先尝试 cancel，再 timedjoin
#ifdef __linux__
    if (g_devStartThread) {
        log_info("[cleanup] joining device_start_thread...");
        // 先请求取消（若 start_all 内部阻塞，join 可能等不到）
        pthread_cancel(g_devStartThread);
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 3; // 最多等 3 秒
        if (pthread_timedjoin_np(g_devStartThread, NULL, &ts) != 0) {
            log_warn("device_start_thread timed out, force cancel+join");
            pthread_cancel(g_devStartThread);
            pthread_join(g_devStartThread, NULL);
        }
        g_devStartThread = 0;
    }
#endif
    log_info("[cleanup] device_start_thread done");

    // 2.2) 释放设备管理器
    log_info("[cleanup] freeing device manager...");
    if (g_deviceManager) {
        device_manager_free(g_deviceManager);
        g_deviceManager = NULL;
    }
    log_info("[cleanup] device manager freed");

    // 3) gRPC
    log_info("[cleanup] stopping gRPC...");
    if (g_grpcServer) {
        grpcserver_stop(g_grpcServer);
        if (g_grpcThread) {
            pthread_join(g_grpcThread, NULL);
            g_grpcThread = 0;
        }
        grpcserver_free(g_grpcServer);
        g_grpcServer = NULL;

        if (g_grpcSockPath[0]) {
            if (unlink(g_grpcSockPath) != 0) {
                log_warn("failed to remove uds socket: %s", g_grpcSockPath);
            } else {
                log_info("uds socket removed: %s", g_grpcSockPath);
            }
            g_grpcSockPath[0] = '\0';
        }
    }
    log_info("[cleanup] gRPC done");

    // 4) MySQL
    log_info("[cleanup] closing MySQL...");
    if (g_mysql) {
        mysql_close_client(g_mysql);
        free(g_mysql->config.addr);
        free(g_mysql->config.database);
        free(g_mysql->config.userName);
        free(g_mysql->config.password);
        free(g_mysql);
        g_mysql = NULL;
    }
    log_info("[cleanup] MySQL done");

    log_info("Cleanup completed");
}

static void* grpc_server_thread(void *arg) {
    GrpcServer *srv = (GrpcServer*)arg;
    // 阻塞运行在子线程，不阻塞主线程
    grpcserver_start(srv);
    return NULL;
}

// 新增：设备启动线程函数（避免主线程阻塞在 start_all）
static void* device_start_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    DeviceManager *mgr = (DeviceManager*)arg;
    device_manager_start_all(mgr);
    return NULL;
}

static int wait_uds_ready(const char *path, int timeout_ms) {
    struct stat st;
    int waited = 0;
    while (waited < timeout_ms) {
        if (stat(path, &st) == 0) return 0;
        usleep(100 * 1000);
        waited += 100;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    int ret = 0;
    Config *config = NULL;
    DeviceInstance *deviceList = NULL;
    DeviceModel *deviceModelList = NULL;
    int deviceCount = 0;
    int modelCount = 0;
    
    log_init();        
    
    log_info("=== KubeEdge Mapper Framework C Version Starting ===");
    
    setup_signal_handlers();
    
    const char *configFile = "../config.yaml";
    if (argc > 1) {
        configFile = argv[1];               
    }
    
    config = config_parse(configFile);    
    if (!config) {
        log_error("Failed to parse configuration: %s", configFile);
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    // 确认解析后打印配置，便于诊断
    log_info("Configuration loaded successfully");
    log_info("MySQL cfg parsed: enabled=%d addr=%s db=%s user=%s",
             config->database.mysql.enabled,
             config->database.mysql.addr[0] ? config->database.mysql.addr : "(empty)",
             config->database.mysql.database[0] ? config->database.mysql.database : "(empty)",
             config->database.mysql.username[0] ? config->database.mysql.username : "(empty)");

    // 环境变量兜底：如果解析器没填 enabled，可用 MYSQL_ENABLED=true/1 打开
    if (!config->database.mysql.enabled) {
        const char *en = getenv("MYSQL_ENABLED");
        if (en && (*en=='1' || strcasecmp(en, "true")==0)) {
            log_warn("MYSQL_ENABLED env overrides config: enabling MySQL");
            config->database.mysql.enabled = 1;
        }
    }

    // 初始化 MySQL（自检）
    if (config->database.mysql.enabled) {
        // 放大缓冲，消除 snprintf 警告
        char json[512];
        snprintf(json, sizeof(json),
                 "{\"addr\":\"%s\",\"database\":\"%s\",\"userName\":\"%s\"}",
                 config->database.mysql.addr[0] ? config->database.mysql.addr : "127.0.0.1",
                 config->database.mysql.database[0] ? config->database.mysql.database : "testdb",
                 config->database.mysql.username[0] ? config->database.mysql.username : "mapper");

        MySQLClientConfig clientCfg = {0};
        if (mysql_parse_client_config(json, &clientCfg) != 0) {
            log_error("MySQL client config parse failed");
        } else {
            g_mysql = (MySQLDataBaseConfig*)calloc(1, sizeof(MySQLDataBaseConfig));
            g_mysql->config = clientCfg;
            if (mysql_init_client(g_mysql) != 0) {
                log_error("MySQL init failed (host=%s db=%s user=%s). Set MYSQL_PASSWORD and MYSQL_PORT if needed.",
                          clientCfg.addr, clientCfg.database, clientCfg.userName);
            } else {
                log_info("MySQL connected (host=%s db=%s user=%s)",
                         clientCfg.addr, clientCfg.database, clientCfg.userName);
                DataModel dm = {0};
                dm.namespace_   = "default";
                dm.deviceName   = "mysql-selftest";
                dm.propertyName = "ping";
                dm.type         = "string";
                dm.value        = "ok";
                dm.timeStamp    = time(NULL);
                if (mysql_add_data(g_mysql, &dm) == 0) {
                    log_info("MySQL self-test OK -> `%s/%s/%s`", dm.namespace_, dm.deviceName, dm.propertyName);
                } else {
                    log_error("MySQL self-test insert failed");
                }
            }
        }
    } else {
        log_info("MySQL disabled in config");
    }

    // 先创建 DeviceManager（供 gRPC 回调使用）
    g_deviceManager = device_manager_new();
    if (!g_deviceManager) {
        log_error("Failed to create device manager");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    // 先启动本地 gRPC（后台线程），确保 EdgeCore 能回连
    const char *grpc_sock = (config->grpc_server.socket_path[0]
                             ? config->grpc_server.socket_path
                             : "/tmp/mapper_dmi.sock");
    unlink(grpc_sock); // 启动前清残留
    // 保存路径，供清理时 unlink
    strncpy(g_grpcSockPath, grpc_sock, sizeof(g_grpcSockPath)-1);

    log_info("Starting GRPC server on socket: %s", grpc_sock);
    ServerConfig *grpcConfig = server_config_new(grpc_sock, "customized");
    g_grpcServer = grpcserver_new(grpcConfig, g_deviceManager);
    if (!g_grpcServer) {
        log_error("Failed to create GRPC server");
        server_config_free(grpcConfig);
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    if (pthread_create(&g_grpcThread, NULL, grpc_server_thread, g_grpcServer) != 0) {
        log_error("Failed to create GRPC server thread");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    server_config_free(grpcConfig);

    // 等待 UDS 文件就绪（最多 3 秒）
    if (wait_uds_ready(grpc_sock, 3000) != 0) {
        log_warn("GRPC UDS not ready yet: %s", grpc_sock);
    } else {
        chmod(grpc_sock, 0666); // 关键：放宽 UDS 权限，便于 edgecore 回连
        log_info("GRPC server started successfully (pre-register)");
    }

    // 再执行注册
    log_info("Mapper will register to edgecore");
    ret = RegisterMapper(1, &deviceList, &deviceCount, &deviceModelList, &modelCount);
    if (ret != 0) {
        log_error("Failed to register mapper to edgecore");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    log_info("Mapper register finished (devices: %d, models: %d)", deviceCount, modelCount);
    
    log_info("Initializing devices...");
    for (int i = 0; i < deviceCount; i++) {
        DeviceModel *model = NULL;
        for (int j = 0; j < modelCount; j++) {
            if (deviceModelList[j].name && deviceList[i].model &&
                strcmp(deviceModelList[j].name, deviceList[i].model) == 0) {
                model = &deviceModelList[j];
                break;
            }
        }
        
        if (!model) {
            log_warn("No model found for device %s", deviceList[i].name);
            continue;
        }
        
        Device *device = device_new(&deviceList[i], model);
        if (!device) {
            log_error("Failed to create device %s", deviceList[i].name);
            continue;
        }
        
        if (device_manager_add(g_deviceManager, device) != 0) {
            log_error("Failed to add device %s to manager", deviceList[i].name);
            device_free(device);
            continue;
        }
        
        log_info("Device %s initialized successfully", deviceList[i].name);
    }
    
    if (g_deviceManager->deviceCount == 0) {
        log_warn("No devices initialized - mapper will run with empty device list");
    } else {
        log_info("Device initialization finished (%d devices)", 
                 g_deviceManager->deviceCount);
    }
    
    log_info("Starting all devices...");
    // 原来是：device_manager_start_all(g_deviceManager);
    // 改为放到独立线程，避免主线程被阻塞，便于 Ctrl+C 立即生效
    if (pthread_create(&g_devStartThread, NULL, device_start_thread, g_deviceManager) != 0) {
        log_error("Failed to create device_start_thread");
    }

    const char *httpPortStr = config->common.http_port;
    if (httpPortStr && strlen(httpPortStr) > 0) {
        log_info("Starting HTTP server on port %s", httpPortStr);
        g_httpServer = rest_server_new(g_deviceManager, httpPortStr);
        if (!g_httpServer) {
            log_error("Failed to create HTTP server");
        } else {
            rest_server_start(g_httpServer);
            log_info("HTTP server started successfully");
        }
    } else {
        log_info("HTTP server disabled (no port configured)");
    }

    log_info("=== Mapper startup completed, running... ===");

    while (running) {
        usleep(1000000);
        
        if (g_deviceManager && g_deviceManager->deviceCount > 0) {
            static int health_check_counter = 0;
            health_check_counter++;
            
            if (health_check_counter >= 30) {
                health_check_counter = 0;
                
                pthread_mutex_lock(&g_deviceManager->managerMutex);
                for (int i = 0; i < g_deviceManager->deviceCount; i++) {
                    Device *device = g_deviceManager->devices[i];
                    if (device) {
                        const char *status = device_get_status(device);
                        if (strcmp(status, DEVICE_STATUS_OK) != 0) {
                            log_warn("Device %s status: %s", 
                                   device->instance.name, status);
                        }
                    }
                }
                pthread_mutex_unlock(&g_deviceManager->managerMutex);
            }
        }
    }
    log_info("Main loop exited, shutting down...");
cleanup:
    cleanup_resources();

    if (config) {
        config_free(config);
    }
    
    log_flush();
    
    log_info("=== Mapper shutdown completed ===");
    
    return ret;
}