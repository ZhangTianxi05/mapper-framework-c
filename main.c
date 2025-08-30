#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>                 // 新增

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

static void cleanup_resources(void);
static void signal_handler(int sig) {
    static int signal_received = 0;
    
    if (signal_received) {
        return;
    }
    signal_received = 1;
    
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            log_info("Received signal %d, shutting down gracefully...", sig);
            running = 0;
            
            cleanup_resources();
            log_info("=== Mapper shutdown completed ===");
            exit(0);
            break;
        default:
            break;
    }
}
static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); 
}

static void cleanup_resources(void) {
    log_info("Cleaning up resources...");
    
    if (g_grpcServer) {
        grpcserver_stop(g_grpcServer);      
        grpcserver_free(g_grpcServer);
        g_grpcServer = NULL;
    }
    
    if (g_httpServer) {
        rest_server_stop(g_httpServer);     
        rest_server_free(g_httpServer);     
        g_httpServer = NULL;
    }
    
    if (g_deviceManager) {
        device_manager_stop_all(g_deviceManager);
        device_manager_free(g_deviceManager);
        g_deviceManager = NULL;
    }

    // 关闭 MySQL
    if (g_mysql) {
        mysql_close_client(g_mysql);
        // 释放通过 mysql_parse_client_config 分配的字符串
        free(g_mysql->config.addr);
        free(g_mysql->config.database);
        free(g_mysql->config.userName);
        free(g_mysql->config.password);
        free(g_mysql);
        g_mysql = NULL;
    }

    log_info("Cleanup completed");
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

    // 注册 Mapper 到 EdgeCore
    log_info("Mapper will register to edgecore");
    ret = RegisterMapper(1, &deviceList, &deviceCount, &deviceModelList, &modelCount); // [`RegisterMapper`](mapper-framework-c/grpcclient/register.cc)
    if (ret != 0) {
        log_error("Failed to register mapper to edgecore");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    log_info("Mapper register finished (devices: %d, models: %d)", deviceCount, modelCount);
    
    g_deviceManager = device_manager_new();
    if (!g_deviceManager) {
        log_error("Failed to create device manager");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
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
    device_manager_start_all(g_deviceManager);
    
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
    

    const char *grpc_sock = (config->grpc_server.socket_path[0]
                             ? config->grpc_server.socket_path
                             : "/tmp/mapper_dmi.sock");
    log_info("Starting GRPC server on socket: %s",
             config->grpc_server.socket_path[0] ? config->grpc_server.socket_path : "default");
    ServerConfig *grpcConfig = server_config_new(grpc_sock, "customized");
    
    g_grpcServer = grpcserver_new(grpcConfig, g_deviceManager);     
    if (!g_grpcServer) {
        log_error("Failed to create GRPC server");
        server_config_free(grpcConfig);
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    if (grpcserver_start(g_grpcServer) != 0) {                      
        log_error("Failed to start GRPC server");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    server_config_free(grpcConfig);
    log_info("GRPC server started successfully");
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