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
    int  enabled;
    char addr[128];
    char database[64];
    char username[64];
    int  port;
    char ssl_mode[16];   // 新增: ssl_mode (DISABLED / PREFERRED / REQUIRED...)
    char password[64];
} DatabaseMySQLConfig;

typedef struct {
    DatabaseMySQLConfig mysql;
} DatabaseConfigGroup;

typedef struct Config {
    GRPCServerConfig grpc_server;
    CommonConfig common;
    DatabaseConfigGroup database;  // 新增
} Config;

Config* config_parse(const char *filename);

void config_free(Config *cfg);

#ifdef __cplusplus
}
#endif

#endif