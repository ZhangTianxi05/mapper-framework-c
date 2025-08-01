#ifndef DRIVER_DEVICETYPE_H
#define DRIVER_DEVICETYPE_H

#include <pthread.h>
#include "common/configmaptype.h"

// CustomizedDev: 设备配置和客户端信息
typedef struct {
    DeviceInstance instance;           // 设备实例（common/configmaptype.h 已定义）
    struct CustomizedClient *client;   // 指向客户端
} CustomizedDev;

// 协议配置结构体
typedef struct {
    char *protocolName;   // 协议名
    char *configData;     // 协议配置(JSON字符串)
} ProtocolConfig;

// 访问器配置结构体
typedef struct {
    char *protocolName;   // 协议名
    char *configData;     // 访问器配置(JSON字符串)
    char *dataType;       // 数据类型
} VisitorConfig;

// CustomizedClient: 设备驱动客户端
typedef struct CustomizedClient {
    pthread_mutex_t deviceMutex;   // 互斥锁
    ProtocolConfig protocolConfig; // 协议配置
    // 你可以在这里添加更多成员变量
} CustomizedClient;

#endif // DRIVER_DEVICETYPE_H