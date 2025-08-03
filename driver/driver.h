#ifndef DRIVER_DRIVER_H
#define DRIVER_DRIVER_H

#include "common/configmaptype.h"
#include <pthread.h>


// 访问信息结构体（可根据你的 VisitorConfig 定义调整）
typedef struct {
    char *propertyName;
    char *protocolName;
    char *configData; // 推荐用 JSON 字符串
} VisitorConfig;

// 驱动客户端结构体
typedef struct {
    ProtocolConfig protocolConfig;
    pthread_mutex_t deviceMutex;
    // 你可以在这里添加更多成员变量
} CustomizedClient;

// 构造与析构
CustomizedClient *NewClient(const ProtocolConfig *protocol);
void FreeClient(CustomizedClient *client);

// 设备操作接口
int InitDevice(CustomizedClient *client);
int GetDeviceData(CustomizedClient *client, const VisitorConfig *visitor, void **out_data);
int DeviceDataWrite(CustomizedClient *client, const VisitorConfig *visitor, const char *deviceMethodName, const char *propertyName, const void *data);
int SetDeviceData(CustomizedClient *client, const void *data, const VisitorConfig *visitor);
int StopDevice(CustomizedClient *client);
const char *GetDeviceStates(CustomizedClient *client);

#endif // DRIVER_DRIVER_H