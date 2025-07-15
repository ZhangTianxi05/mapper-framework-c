/*
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "config/config.h"
#include "log/log.h"
#include "device.h"
#include "grpc_client.h"
#include "grpc_server.h"
#include "http_server.h"

static volatile int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main() {
    Config *c = NULL;
    DeviceList *device_list = NULL;
    DeviceModelList *device_model_list = NULL;
    DevPanel *panel = NULL;
    GrpcServer *grpc_server = NULL;
    HttpServer *http_server = NULL;

    log_init();
    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    // 解析配置
    c = config_parse("config/config.yaml");
    if (!c) {
        log_fatal("Failed to parse config");
        exit(EXIT_FAILURE);
    }
    log_info("Config loaded");

    // 注册到 edgecore
    log_info("Mapper will register to edgecore");
    if (grpc_client_register_mapper(&device_list, &device_model_list, 1) != 0) {
        log_fatal("RegisterMapper failed");
        exit(EXIT_FAILURE);
    }
    log_info("Mapper register finished");

    // 初始化设备面板
    panel = device_new_dev_panel();
    if (device_panel_init(panel, device_list, device_model_list) != 0) {
        log_fatal("DevInit failed");
        exit(EXIT_FAILURE);
    }
    log_info("devInit finished");

    // 启动设备面板
    device_panel_start(panel);

    // 启动 HTTP server
    http_server = http_server_new(panel, c->http_port);
    http_server_start(http_server);

    // 启动 gRPC server
    grpc_server = grpc_server_new(c->grpc_sock_path, panel);
    grpc_server_start(grpc_server);

    // 主循环
    while (keep_running) {
        sleep(1);
    }

    // 清理资源
    grpc_server_stop(grpc_server);
    http_server_stop(http_server);
    device_panel_free(panel);
    config_free(c);
    log_flush();

    return 0;
}*/
#include "log/log.h"

int main() {
    log_init();
    log_info("This is an info log");
    log_error("This is an error log");
    log_flush();
    return 0;
}