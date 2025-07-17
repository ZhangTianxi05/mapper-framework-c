#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char socket_path[256];
} GRPCServerConfig;

typedef struct {
    char name[64];
    char version[32];
    char api_version[32];
    char protocol[32];
    char address[128];
    char edgecore_sock[256];
    char http_port[16];
} CommonConfig;

typedef struct {
    GRPCServerConfig grpc_server;
    CommonConfig common;
} Config;

Config* config_parse(const char *filename);

void config_free(Config *cfg);

#ifdef __cplusplus
}
#endif

#endif