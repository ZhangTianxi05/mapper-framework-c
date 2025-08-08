#include "tdengine_client.h"
#include "log/log.h"
#include "common/datamodel.h"
#include "driver/driver.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void *tdengine_data_handler_thread(void *arg) {
    TDEngineDataHandlerArgs *args = (TDEngineDataHandlerArgs*)arg;
    
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
        args->dataModel->timeStamp = (int64_t)time(NULL) * 1000; // 毫秒
        
        // 写入 TDengine 数据库
        if (tdengine_add_data(&args->dbConfig, args->dataModel) != 0) {
            log_error("tdengine database add data error");
            break;
        }
        
        // 等待下一个周期
        usleep(args->reportCycleMs * 1000);
    }
    
    // 清理资源
    tdengine_close_client(&args->dbConfig);
    return NULL;
}

// 启动 TDengine 数据处理
int StartTDEngineDataHandler(const char *clientConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs) {
    if (!clientConfigJson || !dataModel || !customizedClient || !visitorConfig) {
        log_error("Invalid parameters for TDengine data handler");
        return -1;
    }
    
    // 解析 TDengine 客户端配置
    TDEngineClientConfig clientCfg;
    if (tdengine_parse_client_config(clientConfigJson, &clientCfg) != 0) {
        log_error("Failed to parse TDengine client config");
        return -1;
    }
    
    // 创建数据处理参数结构
    TDEngineDataHandlerArgs *args = calloc(1, sizeof(TDEngineDataHandlerArgs));
    if (!args) {
        log_error("Failed to allocate memory for TDengine data handler args");
        free(clientCfg.addr);
        free(clientCfg.dbName);
        free(clientCfg.username);
        free(clientCfg.password);
        return -1;
    }
    
    // 设置数据库配置
    args->dbConfig.config = clientCfg;
    
    // 初始化 TDengine 客户端
    if (tdengine_init_client(&args->dbConfig) != 0) {
        log_error("Failed to initialize TDengine client");
        free(args);
        free(clientCfg.addr);
        free(clientCfg.dbName);
        free(clientCfg.username);
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
    if (pthread_create(&tid, NULL, tdengine_data_handler_thread, args) != 0) {
        log_error("Failed to create TDengine data handler thread");
        tdengine_close_client(&args->dbConfig);
        free(args);
        free(clientCfg.addr);
        free(clientCfg.dbName);
        free(clientCfg.username);
        free(clientCfg.password);
        return -1;
    }
    
    // 分离线程，让它独立运行
    pthread_detach(tid);
    log_info("TDengine data handler started successfully");
    return 0;
}

// 停止 TDengine 数据处理
int StopTDEngineDataHandler(TDEngineDataHandlerArgs *args) {
    if (!args) return -1;
    
    args->running = 0;
    log_info("TDengine data handler stopped");
    return 0;
}

// 创建 TDengine 数据库客户端
TDEngineDataBaseConfig* NewTDEngineDataBaseClient(const char *configJson) {
    if (!configJson) {
        log_error("TDengine config JSON is null");
        return NULL;
    }
    
    TDEngineClientConfig clientCfg;
    if (tdengine_parse_client_config(configJson, &clientCfg) != 0) {
        log_error("Failed to parse TDengine client config");
        return NULL;
    }
    
    TDEngineDataBaseConfig *dbConfig = calloc(1, sizeof(TDEngineDataBaseConfig));
    if (!dbConfig) {
        log_error("Failed to allocate memory for TDengine database config");
        free(clientCfg.addr);
        free(clientCfg.dbName);
        free(clientCfg.username);
        free(clientCfg.password);
        return NULL;
    }
    
    dbConfig->config = clientCfg;
    
    if (tdengine_init_client(dbConfig) != 0) {
        log_error("Failed to initialize TDengine database client");
        free(dbConfig);
        free(clientCfg.addr);
        free(clientCfg.dbName);
        free(clientCfg.username);
        free(clientCfg.password);
        return NULL;
    }
    
    return dbConfig;
}

// 释放 TDengine 数据库客户端
void FreeTDEngineDataBaseClient(TDEngineDataBaseConfig *dbConfig) {
    if (!dbConfig) return;
    
    tdengine_close_client(dbConfig);
    free(dbConfig->config.addr);
    free(dbConfig->config.dbName);
    free(dbConfig->config.username);
    free(dbConfig->config.password);
    free(dbConfig);
}