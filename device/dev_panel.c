#include "dev_panel.h"
#include "device.h"
#include "devicetwin.h"
#include "log/log.h"
#include <string.h>
#include <stdlib.h>

// 从 DeviceManager 中获取设备孪生结果
int dev_panel_get_twin_result(DeviceManager *manager, const char *deviceId, 
                             const char *propertyName, char **value, char **datatype) {
    if (!manager || !deviceId || !propertyName || !value || !datatype) return -1;
    
    *value = NULL;
    *datatype = NULL;
    
    // 查找设备
    Device *device = device_manager_get(manager, deviceId);
    if (!device) {
        log_warn("Device %s not found", deviceId);
        return -1;
    }
    
    // 获取孪生属性值
    TwinResult result = {0};
    if (devicetwin_get(device, propertyName, &result) != 0) {
        log_error("Failed to get twin property %s for device %s", propertyName, deviceId);
        return -1;
    }
    
    if (result.success && result.value) {
        *value = strdup(result.value);
        *datatype = strdup("string"); // 默认类型
        
        // 清理结果
        free(result.value);
        free(result.error);
        return 0;
    }
    
    // 清理结果
    free(result.value);
    free(result.error);
    return -1;
}

// 写入设备数据
int dev_panel_write_device(DeviceManager *manager, const char *method, 
                          const char *deviceId, const char *propertyName, const char *data) {
    if (!manager || !deviceId || !propertyName || !data) return -1;
    
    // 查找设备
    Device *device = device_manager_get(manager, deviceId);
    if (!device) {
        log_warn("Device %s not found", deviceId);
        return -1;
    }
    
    // 设置孪生属性值
    TwinResult result = {0};
    if (devicetwin_set(device, propertyName, data, &result) != 0) {
        log_error("Failed to set twin property %s for device %s", propertyName, deviceId);
        free(result.value);
        free(result.error);
        return -1;
    }
    
    log_info("Successfully wrote property %s=%s to device %s", propertyName, data, deviceId);
    
    // 清理结果
    free(result.value);
    free(result.error);
    return 0;
}

// 获取设备方法
int dev_panel_get_device_method(DeviceManager *manager, const char *deviceId,
                               char ***method_map, int *method_count,
                               char ***property_map, int *property_count) {
    if (!manager || !deviceId || !method_map || !method_count || 
        !property_map || !property_count) return -1;
    
    // 查找设备
    Device *device = device_manager_get(manager, deviceId);
    if (!device) {
        log_warn("Device %s not found", deviceId);
        return -1;
    }
    
    *method_count = device->instance.methodsCount;
    *property_count = device->instance.methodsCount;
    
    if (*method_count == 0) {
        *method_map = NULL;
        *property_map = NULL;
        return 0;
    }
    
    // 分配方法映射数组
    *method_map = calloc(*method_count, sizeof(char*));
    *property_map = calloc(*property_count, sizeof(char*));
    
    for (int i = 0; i < *method_count; i++) {
        DeviceMethod *method = &device->instance.methods[i];
        
        // 分配方法名数组（简化为只包含方法名）
        (*method_map)[i] = calloc(2, sizeof(char*));
        (*method_map)[i][0] = strdup(method->name ? method->name : "unknown");
        (*method_map)[i][1] = NULL; // 结束标记
        
        // 分配属性名数组
        int propCount = method->propertyNamesCount;
        (*property_map)[i] = calloc(propCount + 1, sizeof(char*));
        
        for (int j = 0; j < propCount; j++) {
            (*property_map)[i][j] = strdup(method->propertyNames[j] ? 
                                         method->propertyNames[j] : "unknown");
        }
        (*property_map)[i][propCount] = NULL; // 结束标记
    }
    
    return 0;
}

// 获取设备信息
int dev_panel_get_device(DeviceManager *manager, const char *deviceId, DeviceInstance *instance) {
    if (!manager || !deviceId || !instance) return -1;
    
    // 查找设备
    Device *device = device_manager_get(manager, deviceId);
    if (!device) {
        log_warn("Device %s not found", deviceId);
        return -1;
    }
    
    // 复制设备实例信息（浅拷贝，注意内存管理）
    *instance = device->instance;
    return 0;
}

// 获取设备模型
int dev_panel_get_model(DeviceManager *manager, const char *modelId, DeviceModel *model) {
    if (!manager || !modelId || !model) return -1;
    
    // 简化实现：遍历所有设备，找到匹配的模型
    pthread_mutex_lock(&manager->managerMutex);
    
    for (int i = 0; i < manager->deviceCount; i++) {
        Device *device = manager->devices[i];
        if (device && device->model.name) {
            // 构建模型ID进行比较（简化版本）
            char deviceModelId[256];
            snprintf(deviceModelId, sizeof(deviceModelId), "%s/%s", 
                     device->model.namespace_ ? device->model.namespace_ : "default",
                     device->model.name);
            
            if (strcmp(deviceModelId, modelId) == 0) {
                // 找到匹配的模型，复制信息（浅拷贝）
                *model = device->model;
                pthread_mutex_unlock(&manager->managerMutex);
                return 0;
            }
        }
    }
    
    pthread_mutex_unlock(&manager->managerMutex);
    log_warn("Model %s not found", modelId);
    return -1;
}

// 检查设备是否存在
int dev_panel_has_device(DeviceManager *manager, const char *deviceId) {
    if (!manager || !deviceId) return 0;
    
    Device *device = device_manager_get(manager, deviceId);
    return device != NULL ? 1 : 0;
}

// 更新设备（占位实现）
int dev_panel_update_dev(DeviceManager *manager, const DeviceModel *model, const DeviceInstance *instance) {
    if (!manager || !model || !instance) return -1;
    
    log_info("Updating device: %s", instance->name ? instance->name : "unknown");
    
    // TODO: 实现设备更新逻辑
    // 1. 查找现有设备
    // 2. 更新设备配置
    // 3. 重启设备（如果需要）
    
    return 0;
}

// 更新模型（占位实现）
int dev_panel_update_model(DeviceManager *manager, const DeviceModel *model) {
    if (!manager || !model) return -1;
    
    log_info("Updating model: %s", model->name ? model->name : "unknown");
    
    // TODO: 实现模型更新逻辑
    
    return 0;
}

// 移除模型（占位实现）
int dev_panel_remove_model(DeviceManager *manager, const char *modelId) {
    if (!manager || !modelId) return -1;
    
    log_info("Removing model: %s", modelId);
    
    // TODO: 实现模型移除逻辑
    
    return 0;
}