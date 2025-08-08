#include "device.h"
#include "log/log.h"
#include "common/const.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

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
    
    // 深拷贝设备实例信息
    memset(&device->instance, 0, sizeof(DeviceInstance)); // 初始化为0
    
    // 复制基本字符串字段（只复制存在的字段）
    if (instance->id) device->instance.id = strdup(instance->id);
    if (instance->name) device->instance.name = strdup(instance->name);
    if (instance->namespace_) device->instance.namespace_ = strdup(instance->namespace_);
    if (instance->model) device->instance.model = strdup(instance->model);
    if (instance->protocolName) device->instance.protocolName = strdup(instance->protocolName);
    
    // 复制协议配置（根据实际定义修正）
    if (instance->pProtocol.protocolName) {
        device->instance.pProtocol.protocolName = strdup(instance->pProtocol.protocolName);
    }
    if (instance->pProtocol.configData) {
        device->instance.pProtocol.configData = strdup(instance->pProtocol.configData);
    }
    
    // 深拷贝 twins 数组
    if (instance->twins && instance->twinsCount > 0) {
        device->instance.twinsCount = instance->twinsCount;
        device->instance.twins = calloc(instance->twinsCount, sizeof(Twin));
        
        for (int i = 0; i < instance->twinsCount; i++) {
            Twin *srcTwin = &instance->twins[i];
            Twin *dstTwin = &device->instance.twins[i];
            
            if (srcTwin->propertyName) {
                dstTwin->propertyName = strdup(srcTwin->propertyName);
            }
            
            // 复制 observedDesired
            if (srcTwin->observedDesired.value) {
                dstTwin->observedDesired.value = strdup(srcTwin->observedDesired.value);
            }
            if (srcTwin->observedDesired.metadata.timestamp) {
                dstTwin->observedDesired.metadata.timestamp = strdup(srcTwin->observedDesired.metadata.timestamp);
            }
            if (srcTwin->observedDesired.metadata.type) {
                dstTwin->observedDesired.metadata.type = strdup(srcTwin->observedDesired.metadata.type);
            }
            
            // 复制 reported
            if (srcTwin->reported.value) {
                dstTwin->reported.value = strdup(srcTwin->reported.value);
            }
            if (srcTwin->reported.metadata.timestamp) {
                dstTwin->reported.metadata.timestamp = strdup(srcTwin->reported.metadata.timestamp);
            }
            if (srcTwin->reported.metadata.type) {
                dstTwin->reported.metadata.type = strdup(srcTwin->reported.metadata.type);
            }
            
            // 如果有 property 指针，也需要复制
            if (srcTwin->property) {
                dstTwin->property = malloc(sizeof(DeviceProperty));
                if (dstTwin->property) {
                    memcpy(dstTwin->property, srcTwin->property, sizeof(DeviceProperty));
                    // 深拷贝 property 中的字符串字段（只复制存在的字段）
                    if (srcTwin->property->name) {
                        dstTwin->property->name = strdup(srcTwin->property->name);
                    }
                    // 注意：根据错误信息，DeviceProperty 可能没有这些字段，需要检查实际定义
                    // if (srcTwin->property->dataType) {
                    //     dstTwin->property->dataType = strdup(srcTwin->property->dataType);
                    // }
                    // if (srcTwin->property->accessMode) {
                    //     dstTwin->property->accessMode = strdup(srcTwin->property->accessMode);
                    // }
                }
            }
        }
    }
    
    // 深拷贝 properties 数组
    if (instance->properties && instance->propertiesCount > 0) {
        device->instance.propertiesCount = instance->propertiesCount;
        device->instance.properties = calloc(instance->propertiesCount, sizeof(DeviceProperty));
        
        for (int i = 0; i < instance->propertiesCount; i++) {
            DeviceProperty *srcProp = &instance->properties[i];
            DeviceProperty *dstProp = &device->instance.properties[i];
            
            // 只复制确实存在的字段
            if (srcProp->name) dstProp->name = strdup(srcProp->name);
            if (srcProp->propertyName) dstProp->propertyName = strdup(srcProp->propertyName);
            if (srcProp->modelName) dstProp->modelName = strdup(srcProp->modelName);
            if (srcProp->protocol) dstProp->protocol = strdup(srcProp->protocol);
            
            // 复制数值字段
            dstProp->collectCycle = srcProp->collectCycle;
            dstProp->reportCycle = srcProp->reportCycle;
            dstProp->reportToCloud = srcProp->reportToCloud;
            
            // 注意：根据错误信息，这些字段可能不存在，需要检查实际定义
            // if (srcProp->dataType) dstProp->dataType = strdup(srcProp->dataType);
            // if (srcProp->accessMode) dstProp->accessMode = strdup(srcProp->accessMode);
            // if (srcProp->description) dstProp->description = strdup(srcProp->description);
            // if (srcProp->minimum) dstProp->minimum = strdup(srcProp->minimum);
            // if (srcProp->maximum) dstProp->maximum = strdup(srcProp->maximum);
            // if (srcProp->unit) dstProp->unit = strdup(srcProp->unit);
            
            // 复制 visitor 配置（可能是不同的字段名）
            // if (srcProp->pVisitor) {
            //     dstProp->pVisitor = malloc(sizeof(VisitorConfig));
            //     // ... 复制 visitor 配置
            // }
        }
    }
    
    // 深拷贝 methods 数组
    if (instance->methods && instance->methodsCount > 0) {
        device->instance.methodsCount = instance->methodsCount;
        device->instance.methods = calloc(instance->methodsCount, sizeof(DeviceMethod));
        
        for (int i = 0; i < instance->methodsCount; i++) {
            DeviceMethod *srcMethod = &instance->methods[i];
            DeviceMethod *dstMethod = &device->instance.methods[i];
            
            if (srcMethod->name) dstMethod->name = strdup(srcMethod->name);
            if (srcMethod->description) dstMethod->description = strdup(srcMethod->description);
            
            // 复制 propertyNames 数组
            if (srcMethod->propertyNames && srcMethod->propertyNamesCount > 0) {
                dstMethod->propertyNamesCount = srcMethod->propertyNamesCount;
                dstMethod->propertyNames = calloc(srcMethod->propertyNamesCount, sizeof(char*));
                
                for (int j = 0; j < srcMethod->propertyNamesCount; j++) {
                    if (srcMethod->propertyNames[j]) {
                        dstMethod->propertyNames[j] = strdup(srcMethod->propertyNames[j]);
                    }
                }
            }
        }
    }
    
    // 复制设备状态（如果存在）
    // device->instance.status = instance->status; // 枚举类型直接赋值
    
    // 深拷贝设备模型信息
    memset(&device->model, 0, sizeof(DeviceModel));
    if (model->id) device->model.id = strdup(model->id);
    if (model->name) device->model.name = strdup(model->name);
    if (model->namespace_) device->model.namespace_ = strdup(model->namespace_);
    if (model->description) device->model.description = strdup(model->description);
    
    // 复制模型属性
    if (model->properties && model->propertiesCount > 0) {
        device->model.propertiesCount = model->propertiesCount;
        device->model.properties = calloc(model->propertiesCount, sizeof(ModelProperty));
        
        for (int i = 0; i < model->propertiesCount; i++) {
            ModelProperty *srcProp = &model->properties[i];
            ModelProperty *dstProp = &device->model.properties[i];
            
            if (srcProp->name) dstProp->name = strdup(srcProp->name);
            if (srcProp->dataType) dstProp->dataType = strdup(srcProp->dataType);
            if (srcProp->description) dstProp->description = strdup(srcProp->description);
            if (srcProp->accessMode) dstProp->accessMode = strdup(srcProp->accessMode);
            if (srcProp->minimum) dstProp->minimum = strdup(srcProp->minimum);
            if (srcProp->maximum) dstProp->maximum = strdup(srcProp->maximum);
            if (srcProp->unit) dstProp->unit = strdup(srcProp->unit);
        }
    }
    
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
    
    // 创建设备客户端（修正指针传递）
    if (device->instance.pProtocol.protocolName) {
        device->client = NewClient(&device->instance.pProtocol);
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
    
    // 释放基本字符串字段
    free(device->instance.name);
    free(device->instance.namespace_);
    free(device->instance.model);
    free(device->instance.protocolName);
    
    // 释放协议配置（修正为结构体字段）
    free(device->instance.pProtocol.protocolName);
    free(device->instance.pProtocol.configData);
    
    // 释放 twins 数组
    if (device->instance.twins) {
        for (int i = 0; i < device->instance.twinsCount; i++) {
            Twin *twin = &device->instance.twins[i];
            free(twin->propertyName);
            free(twin->observedDesired.value);
            free(twin->observedDesired.metadata.timestamp);
            free(twin->observedDesired.metadata.type);
            free(twin->reported.value);
            free(twin->reported.metadata.timestamp);
            free(twin->reported.metadata.type);
            if (twin->property) {
                free(twin->property->name);
                // 释放其他 property 字段
                free(twin->property);
            }
        }
        free(device->instance.twins);
    }
    
    // 释放 properties 数组
    if (device->instance.properties) {
        for (int i = 0; i < device->instance.propertiesCount; i++) {
            DeviceProperty *prop = &device->instance.properties[i];
            free(prop->name);
            free(prop->propertyName);
            free(prop->modelName);
            free(prop->protocol);
            // 释放其他字段...
        }
        free(device->instance.properties);
    }
    
    // 释放 methods 数组
    if (device->instance.methods) {
        for (int i = 0; i < device->instance.methodsCount; i++) {
            DeviceMethod *method = &device->instance.methods[i];
            free(method->name);
            free(method->description);
            if (method->propertyNames) {
                for (int j = 0; j < method->propertyNamesCount; j++) {
                    free(method->propertyNames[j]);
                }
                free(method->propertyNames);
            }
        }
        free(device->instance.methods);
    }
    
    // 释放模型字段
    free(device->model.name);
    free(device->model.namespace_);
    free(device->model.description);
    
    // 释放模型属性
    if (device->model.properties) {
        for (int i = 0; i < device->model.propertiesCount; i++) {
            ModelProperty *prop = &device->model.properties[i];
            free(prop->name);
            free(prop->dataType);
            free(prop->description);
            free(prop->accessMode);
            free(prop->minimum);
            free(prop->maximum);
            free(prop->unit);
        }
        free(device->model.properties);
    }
    
    // 释放设备状态
    free(device->status);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&device->mutex);
    
    free(device);
}

// 其余函数保持不变...
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

// 处理设备 Twin 数据（暂时简单实现）
int device_deal_twin(Device *device, const Twin *twin) {
    if (!device || !twin) return -1;
    
    // TODO: 实现具体的 twin 处理逻辑
    log_debug("Processing twin for device %s: %s", 
              device->instance.name, twin->propertyName);
    return 0;
}

// 处理设备数据
int device_data_process(Device *device, const char *method, const char *config, 
                       const char *propertyName, const void *data) {
    if (!device || !method) return -1;
    
    log_debug("Processing device data: method=%s, property=%s", method, propertyName);
    
    // TODO: 根据方法类型分发处理
    return 0;
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

// 从配置初始化设备（占位实现）
int device_init_from_config(Device *device, const char *configPath) {
    if (!device || !configPath) return -1;
    
    // TODO: 实现从配置文件加载设备信息
    log_info("Device %s initialized from config: %s", device->instance.name, configPath);
    return 0;
}

// 注册设备到边缘核心（占位实现）
int device_register_to_edge(Device *device) {
    if (!device) return -1;
    
    // TODO: 实现设备注册逻辑
    log_info("Device %s registered to edge core", device->instance.name);
    return 0;
}