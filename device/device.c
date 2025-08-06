#include "device.h"
#include "devicestatus.h"
#include "devicetwin.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// 设备数据处理线程
static void *device_data_thread(void *arg) {
    Device *device = (Device*)arg;
    
    log_info("Device data thread started for device: %s", 
             device->instance.name ? device->instance.name : "unknown");
    
    while (device->dataThreadRunning) {
        pthread_mutex_lock(&device->mutex);
        
        // 检查设备状态
        if (strcmp(device->status, DEVICE_STATUS_OK) != 0) {
            pthread_mutex_unlock(&device->mutex);
            usleep(1000000); // 1秒
            continue;
        }
        
        // 处理设备 twins 数据
        for (int i = 0; i < device->instance.twinsCount; i++) {
            Twin *twin = &device->instance.twins[i];
            if (twin && twin->propertyName) {
                device_deal_twin(device, twin);
            }
        }
        
        pthread_mutex_unlock(&device->mutex);
        
        // 等待下个周期（可配置）
        usleep(5000000); // 5秒
    }
    
    log_info("Device data thread stopped for device: %s", 
             device->instance.name ? device->instance.name : "unknown");
    return NULL;
}

// 创建设备
Device *device_new(const DeviceInstance *instance, const DeviceModel *model) {
    if (!instance || !model) {
        log_error("Invalid parameters for device creation");
        return NULL;
    }
    
    Device *device = calloc(1, sizeof(Device));
    if (!device) {
        log_error("Failed to allocate memory for device");
        return NULL;
    }
    
    // 复制设备实例信息
    device->instance = *instance; // 浅拷贝，注意深拷贝字符串
    if (instance->name) device->instance.name = strdup(instance->name);
    if (instance->namespace_) device->instance.namespace_ = strdup(instance->namespace_);
    if (instance->model) device->instance.model = strdup(instance->model);
    if (instance->protocolName) device->instance.protocolName = strdup(instance->protocolName);
    
    // 复制设备模型信息
    device->model = *model; // 浅拷贝
    if (model->name) device->model.name = strdup(model->name);
    if (model->namespace_) device->model.namespace_ = strdup(model->namespace_);
    
    // 初始化设备状态
    device->status = strdup(DEVICE_STATUS_UNKNOWN);
    device->stopChan = 0;
    device->dataThreadRunning = 0;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&device->mutex, NULL) != 0) {
        log_error("Failed to initialize device mutex");
        device_free(device);
        return NULL;
    }
    
    // 创建设备客户端
    if (instance->pProtocol) {
        device->client = NewClient(instance->pProtocol);
        if (!device->client) {
            log_error("Failed to create device client");
            device_free(device);
            return NULL;
        }
    }
    
    log_info("Device created successfully: %s", device->instance.name);
    return device;
}

// 销毁设备
void device_free(Device *device) {
    if (!device) return;
    
    // 停止设备
    device_stop(device);
    
    // 释放客户端
    if (device->client) {
        FreeClient(device->client);
    }
    
    // 释放字符串
    free(device->instance.name);
    free(device->instance.namespace_);
    free(device->instance.model);
    free(device->instance.protocolName);
    free(device->model.name);
    free(device->model.namespace_);
    free(device->status);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&device->mutex);
    
    free(device);
}

// 启动设备
int device_start(Device *device) {
    if (!device) return -1;
    
    pthread_mutex_lock(&device->mutex);
    
    log_info("Starting device: %s", device->instance.name);
    
    // 检查设备是否已启动
    if (device->dataThreadRunning) {
        log_warn("Device %s is already running", device->instance.name);
        pthread_mutex_unlock(&device->mutex);
        return 0;
    }
    
    // 初始化设备客户端
    if (device->client) {
        if (InitDevice(device->client) != 0) {
            log_error("Failed to initialize device client for %s", device->instance.name);
            device_set_status(device, DEVICE_STATUS_OFFLINE);
            pthread_mutex_unlock(&device->mutex);
            return -1;
        }
    }
    
    // 设置设备状态
    device_set_status(device, DEVICE_STATUS_OK);
    
    // 启动数据处理线程
    device->dataThreadRunning = 1;
    if (pthread_create(&device->dataThread, NULL, device_data_thread, device) != 0) {
        log_error("Failed to create data thread for device %s", device->instance.name);
        device->dataThreadRunning = 0;
        device_set_status(device, DEVICE_STATUS_OFFLINE);
        pthread_mutex_unlock(&device->mutex);
        return -1;
    }
    
    pthread_detach(device->dataThread);
    
    pthread_mutex_unlock(&device->mutex);
    
    log_info("Device %s started successfully", device->instance.name);
    return 0;
}

// 停止设备
int device_stop(Device *device) {
    if (!device) return -1;
    
    pthread_mutex_lock(&device->mutex);
    
    log_info("Stopping device: %s", device->instance.name);
    
    // 设置停止标志
    device->stopChan = 1;
    device->dataThreadRunning = 0;
    
    // 停止设备客户端
    if (device->client) {
        StopDevice(device->client);
    }
    
    // 设置设备状态
    device_set_status(device, DEVICE_STATUS_OFFLINE);
    
    pthread_mutex_unlock(&device->mutex);
    
    // 等待数据线程结束
    usleep(100000); // 100ms
    
    log_info("Device %s stopped successfully", device->instance.name);
    return 0;
}

// 重启设备
int device_restart(Device *device) {
    if (!device) return -1;
    
    log_info("Restarting device: %s", device->instance.name);
    
    if (device_stop(device) != 0) {
        log_error("Failed to stop device %s during restart", device->instance.name);
        return -1;
    }
    
    // 等待一段时间
    usleep(1000000); // 1秒
    
    if (device_start(device) != 0) {
        log_error("Failed to start device %s during restart", device->instance.name);
        return -1;
    }
    
    log_info("Device %s restarted successfully", device->instance.name);
    return 0;
}

// 处理设备 Twin 数据
int device_deal_twin(Device *device, const Twin *twin) {
    if (!device || !twin) return -1;
    
    // 使用 devicetwin 模块处理
    return devicetwin_deal(device, twin);
}

// 处理设备数据（根据方法类型分发）
int device_data_process(Device *device, const char *method, const char *config, 
                       const char *propertyName, const void *data) {
    if (!device || !method) return -1;
    
    log_debug("Processing device data: method=%s, property=%s", method, propertyName);
    
    // 根据方法类型分发处理
    if (strcmp(method, "mysql") == 0) {
        // MySQL 数据库处理
        return StartMySQLDataHandler(config, NULL, device->client, NULL, 10000);
        
    } else if (strcmp(method, "redis") == 0) {
        // Redis 数据库处理
        return StartRedisDataHandler(config, NULL, device->client, NULL, 10000);
        
    } else if (strcmp(method, "influxdb2") == 0) {
        // InfluxDB2 数据库处理
        return StartInfluxDB2DataHandler(config, NULL, device->client, NULL, 10000);
        
#ifdef TAOS_FOUND
    } else if (strcmp(method, "tdengine") == 0) {
        // TDengine 数据库处理
        return StartTDEngineDataHandler(config, NULL, device->client, NULL, 10000);
#endif
        
#ifdef ENABLE_STREAM
    } else if (strcmp(method, "stream") == 0) {
        // 流处理
        VisitorConfig visitorConfig = {0};
        visitorConfig.configData = (char*)config;
        return stream_handler(twin, device->client, &visitorConfig);
#endif
        
    } else {
        log_error("Unsupported data processing method: %s", method);
        return -1;
    }
}

// 获取设备状态
const char *device_get_status(Device *device) {
    if (!device) return DEVICE_STATUS_UNKNOWN;
    
    pthread_mutex_lock(&device->mutex);
    const char *status = device->status;
    pthread_mutex_unlock(&device->mutex);
    
    return status;
}

// 设置设备状态
int device_set_status(Device *device, const char *status) {
    if (!device || !status) return -1;
    
    pthread_mutex_lock(&device->mutex);
    free(device->status);
    device->status = strdup(status);
    pthread_mutex_unlock(&device->mutex);
    
    log_debug("Device %s status changed to: %s", device->instance.name, status);
    return 0;
}

// 创建设备管理器
DeviceManager *device_manager_new(void) {
    DeviceManager *manager = calloc(1, sizeof(DeviceManager));
    if (!manager) return NULL;
    
    manager->capacity = 10; // 初始容量
    manager->devices = calloc(manager->capacity, sizeof(Device*));
    if (!manager->devices) {
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->managerMutex, NULL) != 0) {
        free(manager->devices);
        free(manager);
        return NULL;
    }
    
    return manager;
}

// 销毁设备管理器
void device_manager_free(DeviceManager *manager) {
    if (!manager) return;
    
    device_manager_stop_all(manager);
    
    pthread_mutex_lock(&manager->managerMutex);
    for (int i = 0; i < manager->deviceCount; i++) {
        device_free(manager->devices[i]);
    }
    free(manager->devices);
    pthread_mutex_unlock(&manager->managerMutex);
    
    pthread_mutex_destroy(&manager->managerMutex);
    free(manager);
}

// 添加设备到管理器
int device_manager_add(DeviceManager *manager, Device *device) {
    if (!manager || !device) return -1;
    
    pthread_mutex_lock(&manager->managerMutex);
    
    // 检查容量
    if (manager->deviceCount >= manager->capacity) {
        manager->capacity *= 2;
        Device **newDevices = realloc(manager->devices, 
                                     manager->capacity * sizeof(Device*));
        if (!newDevices) {
            pthread_mutex_unlock(&manager->managerMutex);
            return -1;
        }
        manager->devices = newDevices;
    }
    
    manager->devices[manager->deviceCount++] = device;
    
    pthread_mutex_unlock(&manager->managerMutex);
    
    log_info("Device %s added to manager", device->instance.name);
    return 0;
}

// 从管理器移除设备
int device_manager_remove(DeviceManager *manager, const char *deviceId) {
    if (!manager || !deviceId) return -1;
    
    pthread_mutex_lock(&manager->managerMutex);
    
    for (int i = 0; i < manager->deviceCount; i++) {
        if (manager->devices[i] && manager->devices[i]->instance.name &&
            strcmp(manager->devices[i]->instance.name, deviceId) == 0) {
            
            device_free(manager->devices[i]);
            
            // 移动数组元素
            for (int j = i; j < manager->deviceCount - 1; j++) {
                manager->devices[j] = manager->devices[j + 1];
            }
            manager->deviceCount--;
            
            pthread_mutex_unlock(&manager->managerMutex);
            log_info("Device %s removed from manager", deviceId);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&manager->managerMutex);
    log_warn("Device %s not found in manager", deviceId);
    return -1;
}

// 从管理器获取设备
Device *device_manager_get(DeviceManager *manager, const char *deviceId) {
    if (!manager || !deviceId) return NULL;
    
    pthread_mutex_lock(&manager->managerMutex);
    
    for (int i = 0; i < manager->deviceCount; i++) {
        if (manager->devices[i] && manager->devices[i]->instance.name &&
            strcmp(manager->devices[i]->instance.name, deviceId) == 0) {
            Device *device = manager->devices[i];
            pthread_mutex_unlock(&manager->managerMutex);
            return device;
        }
    }
    
    pthread_mutex_unlock(&manager->managerMutex);
    return NULL;
}

// 启动所有设备
int device_manager_start_all(DeviceManager *manager) {
    if (!manager) return -1;
    
    pthread_mutex_lock(&manager->managerMutex);
    
    int success = 0;
    for (int i = 0; i < manager->deviceCount; i++) {
        if (device_start(manager->devices[i]) == 0) {
            success++;
        }
    }
    
    pthread_mutex_unlock(&manager->managerMutex);
    
    log_info("Started %d/%d devices", success, manager->deviceCount);
    return success == manager->deviceCount ? 0 : -1;
}

// 停止所有设备
int device_manager_stop_all(DeviceManager *manager) {
    if (!manager) return -1;
    
    pthread_mutex_lock(&manager->managerMutex);
    
    for (int i = 0; i < manager->deviceCount; i++) {
        device_stop(manager->devices[i]);
    }
    
    pthread_mutex_unlock(&manager->managerMutex);
    
    log_info("Stopped all devices");
    return 0;
}

// 从配置初始化设备
int device_init_from_config(Device *device, const char *configPath) {
    if (!device || !configPath) return -1;
    
    // TODO: 实现从配置文件加载设备信息
    log_info("Device %s initialized from config: %s", device->instance.name, configPath);
    return 0;
}

// 注册设备到边缘核心
int device_register_to_edge(Device *device) {
    if (!device) return -1;
    
    // TODO: 实现设备注册逻辑
    log_info("Device %s registered to edge core", device->instance.name);
    return 0;
}