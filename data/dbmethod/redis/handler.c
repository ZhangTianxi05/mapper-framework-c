#include "redis_client.h"
#include "log/log.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 线程参数结构体
typedef struct {
    DataBaseConfig dbConfig;
    DataModel *dataModel;
    int reportCycleMs;
    void *customizedClient; // 你的 driver/driver.h 类型
    void *visitorConfig;
    int running;
} DataHandlerArgs;

extern int GetDeviceData(void *client, void *visitor, char **out_value);

static void *data_handler_thread(void *arg) {
    DataHandlerArgs *args = (DataHandlerArgs*)arg;
    while (args->running) {
        char *deviceData = NULL;
        int ret = GetDeviceData(args->customizedClient, args->visitorConfig, &deviceData);
        if (ret != 0) {
            log_error("publish error");
            usleep(args->reportCycleMs * 1000);
            continue;
        }
        if (deviceData) {
            if (args->dataModel->value) free(args->dataModel->value);
            args->dataModel->value = strdup(deviceData);
            free(deviceData);
        }
        args->dataModel->timeStamp = (int64_t)time(NULL);

        if (redis_add_data(&args->dbConfig, args->dataModel) != 0) {
            log_error("redis database add data error");
            break;
        }
        usleep(args->reportCycleMs * 1000);
    }
    redis_close_client(&args->dbConfig);
    return NULL;
}

int StartDataHandler(const char *clientConfigJson, DataModel *dataModel, void *customizedClient, void *visitorConfig, int reportCycleMs) {
    RedisClientConfig clientCfg;
    if (redis_parse_client_config(clientConfigJson, &clientCfg) != 0) return -1;
    DataBaseConfig db;
    db.config = clientCfg;
    if (redis_init_client(&db) != 0) return -1;
    DataHandlerArgs *args = calloc(1, sizeof(DataHandlerArgs));
    args->dbConfig = db;
    args->dataModel = dataModel;
    args->customizedClient = customizedClient;
    args->visitorConfig = visitorConfig;
    args->reportCycleMs = reportCycleMs > 0 ? reportCycleMs : 1000;
    args->running = 1;
    pthread_t tid;
    pthread_create(&tid, NULL, data_handler_thread, args);
    pthread_detach(tid);
    return 0;
}