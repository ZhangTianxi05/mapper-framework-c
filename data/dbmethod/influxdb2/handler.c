#include "influxdb2_client.h"
#include "log/log.h"
#include "common/datamodel.h"
#include "driver/driver.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    Influxdb2Client client;
    Influxdb2ClientConfig clientConfig;
    Influxdb2DataConfig dataConfig;
    DataModel *dataModel;
    int reportCycleMs;
    CustomizedClient *customizedClient;  // 使用具体类型而不是 void*
    VisitorConfig *visitorConfig;        // 使用具体类型而不是 void*
    int running;
} DataHandlerArgs;

static void *data_handler_thread(void *arg) {
    DataHandlerArgs *args = (DataHandlerArgs*)arg;
    while (args->running) {
        // 采集数据 - 修复为 void* 类型以匹配函数签名
        void *deviceData = NULL;
        int ret = GetDeviceData(args->customizedClient, args->visitorConfig, &deviceData);
        if (ret != 0) {
            log_error("GetDeviceData failed");
            usleep(args->reportCycleMs * 1000);
            continue;
        }
        
        if (deviceData) {
            if (args->dataModel->value) free(args->dataModel->value);
            // 假设返回的是字符串数据，进行类型转换
            args->dataModel->value = strdup((char*)deviceData);
            free(deviceData);
        }
        
        args->dataModel->timeStamp = (int64_t)time(NULL);
        
        if (influxdb2_add_data(&args->clientConfig, &args->dataConfig, &args->client, args->dataModel) != 0) {
            log_error("influx database add data error");
            break;
        }
        
        usleep(args->reportCycleMs * 1000);
    }
    
    influxdb2_close_client(&args->client);
    return NULL;
}

// 启动数据处理
int StartDataHandler(const char *clientConfigJson, const char *dataConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs) {
    if (!clientConfigJson || !dataConfigJson || !dataModel || !customizedClient || !visitorConfig) {
        return -1;
    }
    
    Influxdb2ClientConfig clientCfg;
    Influxdb2DataConfig dataCfg;
    
    if (influxdb2_parse_client_config(clientConfigJson, &clientCfg) != 0) {
        log_error("Failed to parse client config");
        return -1;
    }
    
    if (influxdb2_parse_data_config(dataConfigJson, &dataCfg) != 0) {
        log_error("Failed to parse data config");
        return -1;
    }
    
    Influxdb2Client client;
    if (influxdb2_init_client(&clientCfg, &client) != 0) {
        log_error("Failed to initialize InfluxDB client");
        return -1;
    }
    
    DataHandlerArgs *args = calloc(1, sizeof(DataHandlerArgs));
    if (!args) {
        influxdb2_close_client(&client);
        return -1;
    }
    
    args->client = client;
    args->clientConfig = clientCfg;
    args->dataConfig = dataCfg;
    args->dataModel = dataModel;
    args->customizedClient = customizedClient;
    args->visitorConfig = visitorConfig;
    args->reportCycleMs = reportCycleMs > 0 ? reportCycleMs : 1000;
    args->running = 1;
    
    pthread_t tid;
    if (pthread_create(&tid, NULL, data_handler_thread, args) != 0) {
        log_error("Failed to create data handler thread");
        influxdb2_close_client(&client);
        free(args);
        return -1;
    }
    
    pthread_detach(tid);
    return 0;
}