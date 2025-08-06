#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include "common/datamodel.h"
#include "driver/driver.h"
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

typedef struct {
    DataBaseConfig dbConfig;
    DataModel *dataModel;
    int reportCycleMs;
    CustomizedClient *customizedClient;
    VisitorConfig *visitorConfig;
    int running;
} RedisDataHandlerArgs;

// Redis 客户端函数
int redis_parse_client_config(const char *json, RedisClientConfig *out);
int redis_init_client(DataBaseConfig *db);
void redis_close_client(DataBaseConfig *db);
int redis_add_data(DataBaseConfig *db, const DataModel *data);
int redis_get_data_by_device_id(DataBaseConfig *db, const char *deviceID, DataModel ***dataModels, int *count);

// Redis 数据处理函数
int StartRedisDataHandler(const char *clientConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs);
int StopRedisDataHandler(RedisDataHandlerArgs *args);
DataBaseConfig* NewRedisDataBaseClient(const char *configJson);
void FreeRedisDataBaseClient(DataBaseConfig *dbConfig);

#endif