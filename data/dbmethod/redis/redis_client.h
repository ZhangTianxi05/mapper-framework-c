#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include "common/datamodel.h"
#include <hiredis/hiredis.h>

typedef struct {
    char *addr;
    int db;
    int poolSize;
    int minIdleConns;
    char *password; // 从环境变量读取
} RedisClientConfig;

typedef struct {
    RedisClientConfig config;
    redisContext *conn;
} DataBaseConfig;

int redis_parse_client_config(const char *json, RedisClientConfig *out);
int redis_init_client(DataBaseConfig *db);
void redis_close_client(DataBaseConfig *db);
int redis_add_data(DataBaseConfig *db, const DataModel *data);

#endif