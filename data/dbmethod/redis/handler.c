#include "redis_client.h"
#include "log/log.h"
#include "common/datamodel.h"
#include "driver/driver.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void *redis_data_handler_thread(void *arg) {
    RedisDataHandlerArgs *args = (RedisDataHandlerArgs*)arg;
    
    while (args->running) {
        // 采集数据
        void *deviceData = NULL;
        int ret = GetDeviceData(args->customizedClient, args->visitorConfig, &deviceData);
        if (ret != 0) {
            log_error("GetDeviceData failed");
            usleep(args->reportCycleMs * 1000);
            continue;
        }
        
        if (deviceData) {
            // 更新数据模型的值
            if (args->dataModel->value) {
                free(args->dataModel->value);
            }
            // 假设返回的是字符串数据，进行类型转换
            args->dataModel->value = strdup((char*)deviceData);
            free(deviceData);
        }
        
        // 设置时间戳
        args->dataModel->timeStamp = (int64_t)time(NULL);
        
        // 写入 Redis 数据库
        if (redis_add_data(&args->dbConfig, args->dataModel) != 0) {
            log_error("redis database add data error");
            break;
        }
        
        // 等待下一个周期
        usleep(args->reportCycleMs * 1000);
    }
    
    // 清理资源
    redis_close_client(&args->dbConfig);
    return NULL;
}

// 启动 Redis 数据处理
int StartRedisDataHandler(const char *clientConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs) {
    if (!clientConfigJson || !dataModel || !customizedClient || !visitorConfig) {
        log_error("Invalid parameters for Redis data handler");
        return -1;
    }
    
    // 解析 Redis 客户端配置
    RedisClientConfig clientCfg;
    if (redis_parse_client_config(clientConfigJson, &clientCfg) != 0) {
        log_error("Failed to parse Redis client config");
        return -1;
    }
    
    // 创建数据处理参数结构
    RedisDataHandlerArgs *args = calloc(1, sizeof(RedisDataHandlerArgs));
    if (!args) {
        log_error("Failed to allocate memory for Redis data handler args");
        free(clientCfg.addr);
        free(clientCfg.password);
        return -1;
    }
    
    // 设置数据库配置
    args->dbConfig.config = clientCfg;
    
    // 初始化 Redis 客户端
    if (redis_init_client(&args->dbConfig) != 0) {
        log_error("Failed to initialize Redis client");
        free(args);
        free(clientCfg.addr);
        free(clientCfg.password);
        return -1;
    }
    
    // 设置其他参数
    args->dataModel = dataModel;
    args->customizedClient = customizedClient;
    args->visitorConfig = visitorConfig;
    args->reportCycleMs = reportCycleMs > 0 ? reportCycleMs : 1000; // 默认1秒
    args->running = 1;
    
    // 创建数据处理线程
    pthread_t tid;
    if (pthread_create(&tid, NULL, redis_data_handler_thread, args) != 0) {
        log_error("Failed to create Redis data handler thread");
        redis_close_client(&args->dbConfig);
        free(args);
        free(clientCfg.addr);
        free(clientCfg.password);
        return -1;
    }
    
    // 分离线程，让它独立运行
    pthread_detach(tid);
    log_info("Redis data handler started successfully");
    return 0;
}

// 停止 Redis 数据处理
int StopRedisDataHandler(RedisDataHandlerArgs *args) {
    if (!args) return -1;
    
    args->running = 0;
    log_info("Redis data handler stopped");
    return 0;
}

// 创建 Redis 数据库客户端
DataBaseConfig* NewRedisDataBaseClient(const char *configJson) {
    if (!configJson) {
        log_error("Redis config JSON is null");
        return NULL;
    }
    
    RedisClientConfig clientCfg;
    if (redis_parse_client_config(configJson, &clientCfg) != 0) {
        log_error("Failed to parse Redis client config");
        return NULL;
    }
    
    DataBaseConfig *dbConfig = calloc(1, sizeof(DataBaseConfig));
    if (!dbConfig) {
        log_error("Failed to allocate memory for Redis database config");
        free(clientCfg.addr);
        free(clientCfg.password);
        return NULL;
    }
    
    dbConfig->config = clientCfg;
    
    if (redis_init_client(dbConfig) != 0) {
        log_error("Failed to initialize Redis database client");
        free(dbConfig);
        free(clientCfg.addr);
        free(clientCfg.password);
        return NULL;
    }
    
    return dbConfig;
}

// 释放 Redis 数据库客户端
void FreeRedisDataBaseClient(DataBaseConfig *dbConfig) {
    if (!dbConfig) return;
    
    redis_close_client(dbConfig);
    free(dbConfig->config.addr);
    free(dbConfig->config.password);
    free(dbConfig);
}