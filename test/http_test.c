#include "httpserver/httpserver.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "device/device.h"
#include "log/log.h"

// 创建一个简单的 mock DeviceManager
DeviceManager* device_manager_new_mock() {
    DeviceManager *manager = calloc(1, sizeof(DeviceManager));
    if (!manager) return NULL;
    
    manager->capacity = 10;
    manager->devices = calloc(manager->capacity, sizeof(Device*));
    manager->deviceCount = 0;
    
    if (pthread_mutex_init(&manager->managerMutex, NULL) != 0) {
        free(manager->devices);
        free(manager);
        return NULL;
    }
    
    return manager;
}

static volatile int keep_running = 1;
void int_handler(int dummy) {
    keep_running = 0;
}

int main() {
    // 初始化日志
    log_init();
    
    signal(SIGINT, int_handler);

    // 创建 mock DeviceManager
    DeviceManager *manager = device_manager_new_mock();
    if (!manager) {
        log_error("Failed to create mock device manager");
        return 1;
    }

    // 启动 HTTP 服务器
    RestServer *server = rest_server_new(manager, "7777");
    if (!server) {
        log_error("Failed to create HTTP server");
        device_manager_free(manager);
        return 1;
    }
    
    rest_server_start(server);

    printf("HTTP server started on port 7777. Press Ctrl+C to stop.\n");
    printf("Test URLs:\n");
    printf("  GET http://localhost:7777/api/v1/ping\n");
    printf("  GET http://localhost:7777/api/v1/device/{namespace}/{name}/{property}\n");
    printf("  GET http://localhost:7777/api/v1/devicemethod/{namespace}/{name}\n");
    printf("  GET http://localhost:7777/api/v1/meta/{namespace}/{name}\n");

    // 阻塞直到 Ctrl+C
    while (keep_running) {
        sleep(1);
    }

    printf("Stopping HTTP server...\n");
    rest_server_stop(server);
    rest_server_free(server);

    // 释放 DeviceManager
    device_manager_free(manager);

    log_info("HTTP test completed");
    return 0;
}