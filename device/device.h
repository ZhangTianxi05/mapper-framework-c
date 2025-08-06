#ifndef DEVICE_DEVICE_H
#define DEVICE_DEVICE_H

#include "common/configmaptype.h"
#include "common/eventtype.h"
#include "driver/driver.h"
#include "data/dbmethod/mysql/mysql_client.h"
#include "data/dbmethod/redis/redis_client.h"
#include "data/dbmethod/influxdb2/influxdb2_client.h"
#ifdef TAOS_FOUND
#include "data/dbmethod/tdengine/tdengine_client.h"
#endif
#ifdef ENABLE_STREAM
#include "data/stream/stream.h"
#endif

// 设备核心结构体
typedef struct {
    DeviceInstance instance;           // 设备实例信息
    DeviceModel model;                 // 设备模型信息
    CustomizedClient *client;          // 设备客户端
    char *status;                      // 设备状态
    pthread_mutex_t mutex;             // 线程安全锁
    int stopChan;                      // 停止通道标志
    pthread_t dataThread;              // 数据处理线程
    int dataThreadRunning;             // 数据线程运行标志
} Device;

// 设备管理器结构体
typedef struct {
    Device **devices;                  // 设备数组
    int deviceCount;                   // 设备数量
    int capacity;                      // 容量
    pthread_mutex_t managerMutex;      // 管理器锁
} DeviceManager;

// 设备创建与销毁
Device *device_new(const DeviceInstance *instance, const DeviceModel *model);
void device_free(Device *device);

// 设备操作
int device_start(Device *device);
int device_stop(Device *device);
int device_restart(Device *device);

// 设备数据处理
int device_deal_twin(Device *device, const Twin *twin);
int device_data_process(Device *device, const char *method, const char *config, 
                       const char *propertyName, const void *data);

// 设备状态管理
const char *device_get_status(Device *device);
int device_set_status(Device *device, const char *status);

// 设备管理器
DeviceManager *device_manager_new(void);
void device_manager_free(DeviceManager *manager);
int device_manager_add(DeviceManager *manager, Device *device);
int device_manager_remove(DeviceManager *manager, const char *deviceId);
Device *device_manager_get(DeviceManager *manager, const char *deviceId);
int device_manager_start_all(DeviceManager *manager);
int device_manager_stop_all(DeviceManager *manager);

// 设备初始化和注册
int device_init_from_config(Device *device, const char *configPath);
int device_register_to_edge(Device *device);

#endif // DEVICE_DEVICE_H