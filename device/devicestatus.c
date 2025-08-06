#include "devicestatus.h"
#include "device.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// 获取当前时间戳（毫秒）
static long long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// 创建设备状态
DeviceStatus *device_status_new(const char *initialStatus) {
    DeviceStatus *status = calloc(1, sizeof(DeviceStatus));
    if (!status) return NULL;
    
    status->status = initialStatus ? strdup(initialStatus) : strdup(DEVICE_STATUS_UNKNOWN);
    status->lastStatus = strdup(DEVICE_STATUS_UNKNOWN);
    status->lastUpdateTime = get_current_time_ms();
    status->healthCheckInterval = 30; // 默认30秒
    status->statusChangeCount = 0;
    
    return status;
}

// 销毁设备状态
void device_status_free(DeviceStatus *status) {
    if (!status) return;
    
    free(status->status);
    free(status->lastStatus);
    free(status);
}

// 更新设备状态
int device_status_update(Device *device, const char *newStatus) {
    if (!device || !newStatus) return -1;
    
    pthread_mutex_lock(&device->mutex);
    
    // 检查状态是否真的改变了
    if (strcmp(device->status, newStatus) == 0) {
        pthread_mutex_unlock(&device->mutex);
        return 0; // 状态没有变化
    }
    
    // 记录状态变化
    char *oldStatus = device->status;
    device->status = strdup(newStatus);
    
    log_info("Device %s status changed: %s -> %s", 
             device->instance.name, oldStatus, newStatus);
    
    // 发送状态变化事件
    device_status_send_event(device, "StatusChanged", 
                            "Device status has been updated");
    
    // 处理特殊状态
    if (strcmp(newStatus, DEVICE_STATUS_OFFLINE) == 0) {
        device_status_handle_offline(device);
    } else if (strcmp(newStatus, DEVICE_STATUS_OK) == 0) {
        device_status_handle_online(device);
    }
    
    free(oldStatus);
    pthread_mutex_unlock(&device->mutex);
    
    return 0;
}

// 检查状态变化
int device_status_check_change(Device *device, const char *currentStatus) {
    if (!device || !currentStatus) return -1;
    
    const char *lastStatus = device_status_get_current(device);
    
    if (strcmp(lastStatus, currentStatus) != 0) {
        return device_status_update(device, currentStatus);
    }
    
    return 0;
}

// 获取当前状态
const char *device_status_get_current(Device *device) {
    if (!device) return DEVICE_STATUS_UNKNOWN;
    
    pthread_mutex_lock(&device->mutex);
    const char *status = device->status;
    pthread_mutex_unlock(&device->mutex);
    
    return status;
}

// 获取上次状态
const char *device_status_get_last(Device *device) {
    if (!device) return DEVICE_STATUS_UNKNOWN;
    
    // TODO: 实现上次状态记录
    return DEVICE_STATUS_UNKNOWN;
}

// 获取上次更新时间
long long device_status_get_last_update_time(Device *device) {
    if (!device) return 0;
    
    // TODO: 实现时间记录
    return get_current_time_ms();
}

// 设备健康检查
int device_status_health_check(Device *device) {
    if (!device) return -1;
    
    log_debug("Performing health check for device: %s", device->instance.name);
    
    // 检查设备客户端状态
    if (!device->client) {
        log_warn("Device %s has no client, marking as offline", device->instance.name);
        device_status_update(device, DEVICE_STATUS_OFFLINE);
        return -1;
    }
    
    // 检查设备响应
    const char *clientStatus = GetDeviceStates(device->client);
    if (!clientStatus || strcmp(clientStatus, DEVICE_STATUS_OK) != 0) {
        log_warn("Device %s health check failed, status: %s", 
                 device->instance.name, clientStatus ? clientStatus : "null");
        device_status_update(device, DEVICE_STATUS_OFFLINE);
        return -1;
    }
    
    // 健康检查通过
    device_status_update(device, DEVICE_STATUS_OK);
    log_debug("Device %s health check passed", device->instance.name);
    
    return 0;
}

// 健康检查线程
static void *health_check_thread(void *arg) {
    Device *device = (Device*)arg;
    
    log_info("Health check thread started for device: %s", device->instance.name);
    
    while (device->dataThreadRunning) {
        device_status_health_check(device);
        
        // 等待健康检查间隔
        sleep(30); // 30秒检查一次
    }
    
    log_info("Health check thread stopped for device: %s", device->instance.name);
    return NULL;
}

// 启动健康监控
int device_status_start_health_monitor(Device *device) {
    if (!device) return -1;
    
    log_info("Starting health monitor for device: %s", device->instance.name);
    
    // 健康检查由设备主线程处理，这里可以设置标志
    return 0;
}

// 停止健康监控
int device_status_stop_health_monitor(Device *device) {
    if (!device) return -1;
    
    log_info("Stopping health monitor for device: %s", device->instance.name);
    
    return 0;
}

// 发送状态事件
int device_status_send_event(Device *device, const char *eventType, const char *message) {
    if (!device || !eventType || !message) return -1;
    
    log_info("Device %s event [%s]: %s", device->instance.name, eventType, message);
    
    // TODO: 实现事件发送到边缘核心
    return 0;
}

// 处理设备离线
int device_status_handle_offline(Device *device) {
    if (!device) return -1;
    
    log_warn("Device %s is now offline", device->instance.name);
    
    // 停止数据采集
    // 发送离线事件
    device_status_send_event(device, "DeviceOffline", "Device has gone offline");
    
    return 0;
}

// 处理设备上线
int device_status_handle_online(Device *device) {
    if (!device) return -1;
    
    log_info("Device %s is now online", device->instance.name);
    
    // 恢复数据采集
    // 发送上线事件
    device_status_send_event(device, "DeviceOnline", "Device has come online");
    
    return 0;
}

// 创建状态管理器
DeviceStatusManager *device_status_manager_new(void) {
    DeviceStatusManager *manager = calloc(1, sizeof(DeviceStatusManager));
    if (!manager) return NULL;
    
    manager->capacity = 10;
    manager->statusList = calloc(manager->capacity, sizeof(DeviceStatus*));
    if (!manager->statusList) {
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->statusMutex, NULL) != 0) {
        free(manager->statusList);
        free(manager);
        return NULL;
    }
    
    return manager;
}

// 销毁状态管理器
void device_status_manager_free(DeviceStatusManager *manager) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->statusMutex);
    for (int i = 0; i < manager->statusCount; i++) {
        device_status_free(manager->statusList[i]);
    }
    free(manager->statusList);
    pthread_mutex_unlock(&manager->statusMutex);
    
    pthread_mutex_destroy(&manager->statusMutex);
    free(manager);
}

// 添加设备到状态管理器
int device_status_manager_add(DeviceStatusManager *manager, Device *device) {
    if (!manager || !device) return -1;
    
    pthread_mutex_lock(&manager->statusMutex);
    
    // 检查容量
    if (manager->statusCount >= manager->capacity) {
        manager->capacity *= 2;
        DeviceStatus **newList = realloc(manager->statusList, 
                                        manager->capacity * sizeof(DeviceStatus*));
        if (!newList) {
            pthread_mutex_unlock(&manager->statusMutex);
            return -1;
        }
        manager->statusList = newList;
    }
    
    DeviceStatus *status = device_status_new(device->status);
    if (!status) {
        pthread_mutex_unlock(&manager->statusMutex);
        return -1;
    }
    
    manager->statusList[manager->statusCount++] = status;
    
    pthread_mutex_unlock(&manager->statusMutex);
    
    return 0;
}

// 从状态管理器移除设备
int device_status_manager_remove(DeviceStatusManager *manager, const char *deviceId) {
    if (!manager || !deviceId) return -1;
    
    pthread_mutex_lock(&manager->statusMutex);
    
    // TODO: 实现移除逻辑（需要设备ID映射）
    
    pthread_mutex_unlock(&manager->statusMutex);
    
    return 0;
}

// 更新所有设备状态
int device_status_manager_update_all(DeviceStatusManager *manager) {
    if (!manager) return -1;
    
    pthread_mutex_lock(&manager->statusMutex);
    
    log_debug("Updating status for %d devices", manager->statusCount);
    
    // TODO: 实现批量状态更新
    
    pthread_mutex_unlock(&manager->statusMutex);
    
    return 0;
}