#include "devicetwin.h"
#include "device.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <cjson/cJSON.h>

// 获取当前时间戳（毫秒）
static long long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// 处理设备孪生
int devicetwin_deal(Device *device, const Twin *twin) {
    if (!device || !twin || !twin->propertyName) {
        log_error("Invalid parameters for devicetwin_deal");
        return -1;
    }
    
    log_debug("Processing twin for device %s, property: %s", 
              device->instance.name, twin->propertyName);
    
    // 根据 twin 的 property 配置处理
    if (!twin->property) {
        log_error("Twin property is NULL for %s", twin->propertyName);
        return -1;
    }
    
    DeviceProperty *prop = twin->property;
    
    // 检查访问模式
    if (!prop->accessMode) {
        log_warn("No access mode specified for property %s", twin->propertyName);
        return -1;
    }
    
    // 处理只读属性 - 读取设备数据并上报
    if (strcmp(prop->accessMode, "ReadOnly") == 0) {
        TwinResult result = {0};
        if (devicetwin_get(device, twin->propertyName, &result) == 0) {
            // 上报到云端
            devicetwin_report_to_cloud(device, twin->propertyName, result.value);
            free(result.value);
            free(result.error);
        }
        return 0;
    }
    
    // 处理读写属性 - 检查期望值变化
    if (strcmp(prop->accessMode, "ReadWrite") == 0) {
        // 检查是否有期望值变化
        if (twin->observedDesired.value && twin->reported.value) {
            if (strcmp(twin->observedDesired.value, twin->reported.value) != 0) {
                // 期望值与上报值不同，需要设置设备
                log_info("Desired value changed for %s: %s -> %s", 
                         twin->propertyName, twin->reported.value, twin->observedDesired.value);
                
                TwinResult result = {0};
                if (devicetwin_set(device, twin->propertyName, 
                                  twin->observedDesired.value, &result) == 0) {
                    // 设置成功，上报新值
                    devicetwin_report_to_cloud(device, twin->propertyName, result.value);
                }
                free(result.value);
                free(result.error);
            }
        }
        
        // 定期读取并上报当前值
        TwinResult result = {0};
        if (devicetwin_get(device, twin->propertyName, &result) == 0) {
            devicetwin_report_to_cloud(device, twin->propertyName, result.value);
            free(result.value);
            free(result.error);
        }
        
        return 0;
    }
    
    log_warn("Unknown access mode for property %s: %s", 
             twin->propertyName, prop->accessMode);
    return -1;
}

// 获取孪生属性值
int devicetwin_get(Device *device, const char *propertyName, TwinResult *result) {
    if (!device || !propertyName || !result) return -1;
    
    memset(result, 0, sizeof(TwinResult));
    result->timestamp = get_current_time_ms();
    
    log_debug("Getting twin property %s for device %s", propertyName, device->instance.name);
    
    // 查找对应的 twin 配置
    Twin *twin = NULL;
    for (int i = 0; i < device->instance.twinsCount; i++) {
        if (device->instance.twins[i].propertyName &&
            strcmp(device->instance.twins[i].propertyName, propertyName) == 0) {
            twin = &device->instance.twins[i];
            break;
        }
    }
    
    if (!twin || !twin->property) {
        result->error = strdup("Property not found or not configured");
        return -1;
    }
    
    // 构建访问配置
    VisitorConfig visitorConfig = {0};
    visitorConfig.propertyName = (char*)propertyName;
    visitorConfig.protocolName = device->instance.protocolName;
    
    // 从 twin property 中获取访问配置
    if (twin->property->pVisitor && twin->property->pVisitor->configData) {
        visitorConfig.configData = twin->property->pVisitor->configData;
    }
    
    // 从设备读取数据
    void *deviceData = NULL;
    int ret = GetDeviceData(device->client, &visitorConfig, &deviceData);
    if (ret != 0 || !deviceData) {
        result->error = strdup("Failed to read device data");
        return -1;
    }
    
    // 转换数据类型
    char *convertedValue = NULL;
    if (devicetwin_convert_data(twin, (char*)deviceData, &convertedValue) == 0) {
        result->value = convertedValue;
    } else {
        result->value = strdup((char*)deviceData);
    }
    
    result->success = 1;
    free(deviceData);
    
    log_debug("Got twin property %s value: %s", propertyName, result->value);
    return 0;
}

// 设置孪生属性值
int devicetwin_set(Device *device, const char *propertyName, const char *value, TwinResult *result) {
    if (!device || !propertyName || !value || !result) return -1;
    
    memset(result, 0, sizeof(TwinResult));
    result->timestamp = get_current_time_ms();
    
    log_debug("Setting twin property %s for device %s to value: %s", 
              propertyName, device->instance.name, value);
    
    // 查找对应的 twin 配置
    Twin *twin = NULL;
    for (int i = 0; i < device->instance.twinsCount; i++) {
        if (device->instance.twins[i].propertyName &&
            strcmp(device->instance.twins[i].propertyName, propertyName) == 0) {
            twin = &device->instance.twins[i];
            break;
        }
    }
    
    if (!twin || !twin->property) {
        result->error = strdup("Property not found or not configured");
        return -1;
    }
    
    // 检查访问模式
    if (twin->property->accessMode && strcmp(twin->property->accessMode, "ReadOnly") == 0) {
        result->error = strdup("Property is read-only");
        return -1;
    }
    
    // 验证数据
    if (devicetwin_validate_data(twin, value) != 0) {
        result->error = strdup("Invalid data value");
        return -1;
    }
    
    // 构建访问配置
    VisitorConfig visitorConfig = {0};
    visitorConfig.propertyName = (char*)propertyName;
    visitorConfig.protocolName = device->instance.protocolName;
    
    if (twin->property->pVisitor && twin->property->pVisitor->configData) {
        visitorConfig.configData = twin->property->pVisitor->configData;
    }
    
    // 写入设备数据
    int ret = DeviceDataWrite(device->client, &visitorConfig, "SetProperty", propertyName, value);
    if (ret != 0) {
        result->error = strdup("Failed to write device data");
        return -1;
    }
    
    // 验证写入结果 - 重新读取
    void *deviceData = NULL;
    ret = GetDeviceData(device->client, &visitorConfig, &deviceData);
    if (ret == 0 && deviceData) {
        result->value = strdup((char*)deviceData);
        result->success = 1;
        free(deviceData);
    } else {
        result->value = strdup(value); // 假设写入成功
        result->success = 1;
    }
    
    log_debug("Set twin property %s to value: %s", propertyName, result->value);
    return 0;
}

// 处理孪生数据
int devicetwin_process_data(Device *device, const Twin *twin, const void *data) {
    if (!device || !twin || !data) return -1;
    
    log_debug("Processing twin data for property: %s", twin->propertyName);
    
    // 根据 twin 配置处理数据
    // 可以调用相应的数据库存储、流处理等
    
    return 0;
}

// 验证孪生数据
int devicetwin_validate_data(const Twin *twin, const char *value) {
    if (!twin || !twin->property || !value) return -1;
    
    DeviceProperty *prop = twin->property;
    
    // 检查数据类型
    if (prop->dataType) {
        if (strcmp(prop->dataType, "int") == 0) {
            char *endptr;
            strtol(value, &endptr, 10);
            if (*endptr != '\0') return -1; // 不是有效整数
            
        } else if (strcmp(prop->dataType, "float") == 0) {
            char *endptr;
            strtod(value, &endptr);
            if (*endptr != '\0') return -1; // 不是有效浮点数
            
        } else if (strcmp(prop->dataType, "boolean") == 0) {
            if (strcmp(value, "true") != 0 && strcmp(value, "false") != 0 &&
                strcmp(value, "1") != 0 && strcmp(value, "0") != 0) {
                return -1; // 不是有效布尔值
            }
        }
        // string 类型不需要特殊验证
    }
    
    // 检查数值范围
    if (prop->minimum && prop->maximum) {
        if (prop->dataType && (strcmp(prop->dataType, "int") == 0 || strcmp(prop->dataType, "float") == 0)) {
            double val = strtod(value, NULL);
            double min = strtod(prop->minimum, NULL);
            double max = strtod(prop->maximum, NULL);
            
            if (val < min || val > max) {
                log_warn("Value %s out of range [%s, %s] for property %s", 
                         value, prop->minimum, prop->maximum, twin->propertyName);
                return -1;
            }
        }
    }
    
    return 0;
}

// 转换孪生数据
int devicetwin_convert_data(const Twin *twin, const char *rawValue, char **convertedValue) {
    if (!twin || !twin->property || !rawValue || !convertedValue) return -1;
    
    // 简单的数据转换示例
    DeviceProperty *prop = twin->property;
    
    if (prop->dataType) {
        if (strcmp(prop->dataType, "boolean") == 0) {
            // 转换布尔值
            if (strcmp(rawValue, "1") == 0 || strcasecmp(rawValue, "true") == 0) {
                *convertedValue = strdup("true");
            } else {
                *convertedValue = strdup("false");
            }
            return 0;
        }
    }
    
    // 默认不转换
    *convertedValue = strdup(rawValue);
    return 0;
}

// 上报到云端
int devicetwin_report_to_cloud(Device *device, const char *propertyName, const char *value) {
    if (!device || !propertyName || !value) return -1;
    
    log_debug("Reporting twin property %s=%s for device %s", 
              propertyName, value, device->instance.name);
    
    // 构建上报数据
    char *reportData = devicetwin_build_report_data(propertyName, value, get_current_time_ms());
    if (!reportData) return -1;
    
    // TODO: 发送到边缘核心
    log_info("Twin report data: %s", reportData);
    
    free(reportData);
    return 0;
}

// 孪生上报线程
static void *twin_report_thread(void *arg) {
    TwinProcessor *processor = (TwinProcessor*)arg;
    
    log_info("Twin report thread started for property: %s", processor->propertyName);
    
    while (processor->reportThreadRunning) {
        // TODO: 定期上报逻辑
        
        usleep(processor->reportCycle * 1000); // 转换为微秒
    }
    
    log_info("Twin report thread stopped for property: %s", processor->propertyName);
    return NULL;
}

// 启动自动上报
int devicetwin_start_auto_report(Device *device, const Twin *twin) {
    if (!device || !twin) return -1;
    
    log_info("Starting auto report for twin property: %s", twin->propertyName);
    
    // TODO: 创建并启动上报线程
    return 0;
}

// 停止自动上报
int devicetwin_stop_auto_report(Device *device, const char *propertyName) {
    if (!device || !propertyName) return -1;
    
    log_info("Stopping auto report for twin property: %s", propertyName);
    
    // TODO: 停止上报线程
    return 0;
}

// 处理期望值变化
int devicetwin_handle_desired_change(Device *device, const Twin *twin, const char *newValue) {
    if (!device || !twin || !newValue) return -1;
    
    log_info("Handling desired change for %s: new value = %s", twin->propertyName, newValue);
    
    TwinResult result = {0};
    if (devicetwin_set(device, twin->propertyName, newValue, &result) == 0) {
        // 设置成功，上报新值
        devicetwin_report_to_cloud(device, twin->propertyName, result.value);
    }
    
    free(result.value);
    free(result.error);
    return 0;
}

// 处理上报值更新
int devicetwin_handle_reported_update(Device *device, const Twin *twin, const char *newValue) {
    if (!device || !twin || !newValue) return -1;
    
    log_debug("Handling reported update for %s: new value = %s", twin->propertyName, newValue);
    
    // 上报到云端
    return devicetwin_report_to_cloud(device, twin->propertyName, newValue);
}

// 解析访问配置
int devicetwin_parse_visitor_config(const char *configData, VisitorConfig *config) {
    if (!configData || !config) return -1;
    
    cJSON *root = cJSON_Parse(configData);
    if (!root) return -1;
    
    cJSON *protocol = cJSON_GetObjectItem(root, "protocolName");
    if (protocol && cJSON_IsString(protocol)) {
        config->protocolName = strdup(protocol->valuestring);
    }
    
    config->configData = strdup(configData);
    
    cJSON_Delete(root);
    return 0;
}

// 构建上报数据
char *devicetwin_build_report_data(const char *propertyName, const char *value, long long timestamp) {
    if (!propertyName || !value) return NULL;
    
    cJSON *root = cJSON_CreateObject();
    cJSON *twin = cJSON_CreateObject();
    cJSON *reported = cJSON_CreateObject();
    
    cJSON_AddStringToObject(reported, propertyName, value);
    cJSON_AddNumberToObject(reported, "timestamp", timestamp);
    
    cJSON_AddItemToObject(twin, "reported", reported);
    cJSON_AddItemToObject(root, "twin", twin);
    
    char *jsonString = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return jsonString;
}

// 创建孪生处理器
TwinProcessor *devicetwin_processor_new(const Twin *twin) {
    if (!twin) return NULL;
    
    TwinProcessor *processor = calloc(1, sizeof(TwinProcessor));
    if (!processor) return NULL;
    
    processor->propertyName = twin->propertyName ? strdup(twin->propertyName) : NULL;
    if (twin->property) {
        processor->dataType = twin->property->dataType ? strdup(twin->property->dataType) : NULL;
        processor->accessMode = twin->property->accessMode ? strdup(twin->property->accessMode) : NULL;
    }
    processor->reportCycle = 10000; // 默认10秒
    processor->reportThreadRunning = 0;
    
    return processor;
}

// 销毁孪生处理器
void devicetwin_processor_free(TwinProcessor *processor) {
    if (!processor) return;
    
    processor->reportThreadRunning = 0;
    
    free(processor->propertyName);
    free(processor->dataType);
    free(processor->accessMode);
    free(processor);
}

// 创建孪生管理器
TwinManager *devicetwin_manager_new(void) {
    TwinManager *manager = calloc(1, sizeof(TwinManager));
    if (!manager) return NULL;
    
    manager->capacity = 10;
    manager->processors = calloc(manager->capacity, sizeof(TwinProcessor*));
    if (!manager->processors) {
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->twinMutex, NULL) != 0) {
        free(manager->processors);
        free(manager);
        return NULL;
    }
    
    return manager;
}

// 销毁孪生管理器
void devicetwin_manager_free(TwinManager *manager) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->twinMutex);
    for (int i = 0; i < manager->processorCount; i++) {
        devicetwin_processor_free(manager->processors[i]);
    }
    free(manager->processors);
    pthread_mutex_unlock(&manager->twinMutex);
    
    pthread_mutex_destroy(&manager->twinMutex);
    free(manager);
}