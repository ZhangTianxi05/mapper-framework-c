#include "httpserver/httpserver.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "global/global.h"
#include <stdlib.h>

DevPanel* dev_panel_new_mock() {
    // 返回一个简单的 mock，实际可根据需要扩展
    return NULL;
}

static volatile int keep_running = 1;
void int_handler(int dummy) {
    keep_running = 0;
}

int main() {
    signal(SIGINT, int_handler);

    // 创建 mock DevPanel
    DevPanel *panel = dev_panel_new_mock();

    // 启动 HTTP 服务器
    RestServer *server = rest_server_new(panel, "7777");
    rest_server_start(server);

    printf("HTTP server started on port 7777. Press Ctrl+C to stop.\n");

    // 阻塞直到 Ctrl+C
    while (keep_running) {
        sleep(1);
    }

    printf("Stopping HTTP server...\n");
    rest_server_stop(server);
    rest_server_free(server);

    // 释放 DevPanel
    // dev_panel_free(panel);

    return 0;
}