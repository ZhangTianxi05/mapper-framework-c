#ifndef DEVICE_DEVICESTATUS_H
#define DEVICE_DEVICESTATUS_H

#include "common/const.h"
#include "common/eventtype.h"

// 前置声明
typedef struct Device Device;

// 设备状态结构体
typedef struct {
    char *status;                      // 当前状态
    char *lastStatus;                  // 上次状态
    long long lastUpdateTime;          // 上次更新时间
    int healthCheckInterval;           // 健康检查间隔(秒)
    int statusChangeCount;             // 状态变更次数
} DeviceStatus;

// 设备状态管理器
typedef struct {
    DeviceStatus **statusList;         // 状态列表
    int statusCount;                   // 状态数量
    int capacity;                      // 容量
    pthread_mutex_t statusMutex;       // 状态锁
    pthread_t healthCheckThread;       // 健康检查线程
    int healthCheckRunning;            // 健康检查运行标志
} DeviceStatusManager;

// 设备状态操作
DeviceStatus *device_status_new(const char *initialStatus);
void device_status_free(DeviceStatus *status);

// 状态更新
int device_status_update(Device *device, const char *newStatus);
int device_status_check_change(Device *device, const char *currentStatus);

// 状态查询
const char *device_status_get_current(Device *device);
const char *device_status_get_last(Device *device);
long long device_status_get_last_update_time(Device *device);

// 健康检查
int device_status_health_check(Device *device);
int device_status_start_health_monitor(Device *device);
int device_status_stop_health_monitor(Device *device);

// 状态管理器
DeviceStatusManager *device_status_manager_new(void);
void device_status_manager_free(DeviceStatusManager *manager);
int device_status_manager_add(DeviceStatusManager *manager, Device *device);
int device_status_manager_remove(DeviceStatusManager *manager, const char *deviceId);
int device_status_manager_update_all(DeviceStatusManager *manager);

// 状态事件处理
int device_status_send_event(Device *device, const char *eventType, const char *message);
int device_status_handle_offline(Device *device);
int device_status_handle_online(Device *device);

#endif // DEVICE_DEVICESTATUS_H