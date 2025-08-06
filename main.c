#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

// 引入已有的 C 代码模块
#include "log/log.h"
#include "config/config.h"
#include "device/device.h"
#include "grpc/grpcclient.h"
#include "grpc/grpcserver.h"
#include "http/httpserver.h"
#include "common/configmaptype.h"

// 全局变量
static volatile int running = 1;
static DeviceManager *g_deviceManager = NULL;
static GrpcServer *g_grpcServer = NULL;
static HttpServer *g_httpServer = NULL;

// 信号处理函数
static void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            log_info("Received signal %d, shutting down gracefully...", sig);
            running = 0;
            break;
        default:
            break;
    }
}

// 注册信号处理器
static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // 忽略 SIGPIPE
}

// 清理资源
static void cleanup_resources(void) {
    log_info("Cleaning up resources...");
    
    // 停止 GRPC 服务器
    if (g_grpcServer) {
        grpcserver_stop(g_grpcServer);
        grpcserver_free(g_grpcServer);
        g_grpcServer = NULL;
    }
    
    // 停止 HTTP 服务器
    if (g_httpServer) {
        httpserver_stop(g_httpServer);
        httpserver_free(g_httpServer);
        g_httpServer = NULL;
    }
    
    // 停止所有设备
    if (g_deviceManager) {
        device_manager_stop_all(g_deviceManager);
        device_manager_free(g_deviceManager);
        g_deviceManager = NULL;
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
    
    // 初始化日志系统
    if (log_init() != 0) {
        fprintf(stderr, "Failed to initialize logging system\n");
        return EXIT_FAILURE;
    }
    
    log_info("=== KubeEdge Mapper Framework C Version Starting ===");
    
    // 设置信号处理器
    setup_signal_handlers();
    
    // 解析配置
    config = config_parse(argc, argv);
    if (!config) {
        log_error("Failed to parse configuration");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    log_info("Configuration loaded successfully");
    config_print(config); // 打印配置信息
    
    // 注册 Mapper 到 EdgeCore
    log_info("Mapper will register to edgecore");
    ret = grpcclient_register_mapper(1, &deviceList, &deviceModelList, 
                                   &deviceCount, &modelCount);
    if (ret != 0) {
        log_error("Failed to register mapper to edgecore");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    log_info("Mapper register finished (devices: %d, models: %d)", 
             deviceCount, modelCount);
    
    // 创建设备管理器（相当于 Go 版本的 NewDevPanel）
    g_deviceManager = device_manager_new();
    if (!g_deviceManager) {
        log_error("Failed to create device manager");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    // 初始化设备（相当于 Go 版本的 DevInit）
    log_info("Initializing devices...");
    for (int i = 0; i < deviceCount; i++) {
        // 查找对应的设备模型
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
        
        // 创建设备
        Device *device = device_new(&deviceList[i], model);
        if (!device) {
            log_error("Failed to create device %s", deviceList[i].name);
            continue;
        }
        
        // 添加到设备管理器
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
    
    // 启动所有设备（相当于 Go 版本的 DevStart）
    log_info("Starting all devices...");
    device_manager_start_all(g_deviceManager);
    
    // 启动 HTTP 服务器
    if (config->common.httpPort > 0) {
        log_info("Starting HTTP server on port %d", config->common.httpPort);
        g_httpServer = httpserver_new(g_deviceManager, config->common.httpPort);
        if (!g_httpServer) {
            log_error("Failed to create HTTP server");
        } else {
            if (httpserver_start(g_httpServer) != 0) {
                log_error("Failed to start HTTP server");
                httpserver_free(g_httpServer);
                g_httpServer = NULL;
            } else {
                log_info("HTTP server started successfully");
            }
        }
    }
    
    // 启动 GRPC 服务器
    log_info("Starting GRPC server on socket: %s", 
             config->grpcServer.socketPath ? config->grpcServer.socketPath : "default");
    
    GrpcServerConfig grpcConfig = {
        .socketPath = config->grpcServer.socketPath,
        .protocol = "customized", // 对应 Go 版本的 common.ProtocolCustomized
    };
    
    g_grpcServer = grpcserver_new(&grpcConfig, g_deviceManager);
    if (!g_grpcServer) {
        log_error("Failed to create GRPC server");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    if (grpcserver_start(g_grpcServer) != 0) {
        log_error("Failed to start GRPC server");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    
    log_info("GRPC server started successfully");
    log_info("=== Mapper startup completed, running... ===");
    
    // 主循环 - 等待信号
    while (running) {
        usleep(1000000); // 1秒
        
        // 可选：定期检查设备状态
        if (g_deviceManager && g_deviceManager->deviceCount > 0) {
            static int health_check_counter = 0;
            health_check_counter++;
            
            // 每30秒检查一次设备健康状态
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
    // 清理资源
    cleanup_resources();
    
    // 释放设备列表
    if (deviceList) {
        // TODO: 实现 grpcclient_free_device_list 函数
        // grpcclient_free_device_list(deviceList, deviceCount);
    }
    
    if (deviceModelList) {
        // TODO: 实现 grpcclient_free_model_list 函数
        // grpcclient_free_model_list(deviceModelList, modelCount);
    }
    
    // 释放配置
    if (config) {
        config_free(config);
    }
    
    // 关闭日志系统
    log_cleanup();
    
    log_info("=== Mapper shutdown completed ===");
    
    return ret;
}