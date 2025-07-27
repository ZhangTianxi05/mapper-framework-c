#ifndef HTTPSERVER_HTTPSERVER_H
#define HTTPSERVER_HTTPSERVER_H

#include <microhttpd.h>
#include "global/global.h"

typedef struct {
    char ip[32];
    char port[8];
    struct MHD_Daemon *daemon;
    DevPanel *dev_panel;
    // 可扩展：TLS配置、数据库client等
} RestServer;

RestServer *rest_server_new(DevPanel *panel, const char *port);
void rest_server_start(RestServer *server);
void rest_server_stop(RestServer *server);
void rest_server_free(RestServer *server);

#endif // HTTPSERVER_HTTPSERVER_H