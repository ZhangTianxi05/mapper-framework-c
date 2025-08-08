#ifndef DEVICE_DEVICETWIN_H
#define DEVICE_DEVICETWIN_H

#include "common/configmaptype.h"
#include "common/datamodel.h"
#include "driver/driver.h"
#include <pthread.h>

// 前置声明 Device 结构体，不重复定义
struct Device;
typedef struct Device Device;

// 孪生数据处理结果
typedef struct {
    int success;                       // 处理是否成功
    char *value;                       // 处理后的值
    char *error;                       // 错误信息
    long long timestamp;               // 时间戳
} TwinResult;

// 孪生处理器
typedef struct {
    char *propertyName;                // 属性名
    char *dataType;                    // 数据类型
    char *accessMode;                  // 访问模式 (ReadOnly/ReadWrite)
    VisitorConfig *visitorConfig;      // 访问配置
    int reportCycle;                   // 上报周期(毫秒)
    pthread_t reportThread;            // 上报线程
    int reportThreadRunning;           // 上报线程运行标志
} TwinProcessor;

// 孪生管理器
typedef struct {
    TwinProcessor **processors;        // 处理器数组
    int processorCount;                // 处理器数量
    int capacity;                      // 容量
    pthread_mutex_t twinMutex;         // 孪生锁
} TwinManager;

// 孪生处理函数
int devicetwin_deal(Device *device, const Twin *twin);
int devicetwin_get(Device *device, const char *propertyName, TwinResult *result);
int devicetwin_set(Device *device, const char *propertyName, const char *value, TwinResult *result);

// 孪生数据处理
int devicetwin_process_data(Device *device, const Twin *twin, const void *data);
int devicetwin_validate_data(const Twin *twin, const char *value);
int devicetwin_convert_data(const Twin *twin, const char *rawValue, char **convertedValue);

// 孪生上报
int devicetwin_report_to_cloud(Device *device, const char *propertyName, const char *value);
int devicetwin_start_auto_report(Device *device, const Twin *twin);
int devicetwin_stop_auto_report(Device *device, const char *propertyName);

// 孪生事件处理
int devicetwin_handle_desired_change(Device *device, const Twin *twin, const char *newValue);
int devicetwin_handle_reported_update(Device *device, const Twin *twin, const char *newValue);

// 孪生处理器管理
TwinProcessor *devicetwin_processor_new(const Twin *twin);
void devicetwin_processor_free(TwinProcessor *processor);

// 孪生管理器
TwinManager *devicetwin_manager_new(void);
void devicetwin_manager_free(TwinManager *manager);
int devicetwin_manager_add(TwinManager *manager, const Twin *twin);
int devicetwin_manager_remove(TwinManager *manager, const char *propertyName);
TwinProcessor *devicetwin_manager_get(TwinManager *manager, const char *propertyName);

// 工具函数
int devicetwin_parse_visitor_config(const char *configData, VisitorConfig *config);
char *devicetwin_build_report_data(const char *propertyName, const char *value, long long timestamp);

#endif // DEVICE_DEVICETWIN_H