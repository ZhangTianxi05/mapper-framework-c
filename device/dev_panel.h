#ifndef DEVICE_DEV_PANEL_H
#define DEVICE_DEV_PANEL_H

// 直接包含需要的头文件，而不是重复定义
#include "common/configmaptype.h"
#include "device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

// 设备面板接口函数

// 获取设备孪生结果
int dev_panel_get_twin_result(DeviceManager *manager, const char *deviceId, 
                             const char *propertyName, char **value, char **datatype);

// 写入设备数据
int dev_panel_write_device(DeviceManager *manager, const char *method, 
                          const char *deviceId, const char *propertyName, const char *data);

// 获取设备方法
int dev_panel_get_device_method(DeviceManager *manager, const char *deviceId,
                               char ***method_map, int *method_count,
                               char ***property_map, int *property_count);

// 获取设备信息
int dev_panel_get_device(DeviceManager *manager, const char *deviceId, DeviceInstance *instance);

// 获取设备模型
int dev_panel_get_model(DeviceManager *manager, const char *modelId, DeviceModel *model);

// 检查设备是否存在
int dev_panel_has_device(DeviceManager *manager, const char *deviceId);

// 更新设备
int dev_panel_update_dev(DeviceManager *manager, const DeviceModel *model, const DeviceInstance *instance);

// 更新模型
int dev_panel_update_model(DeviceManager *manager, const DeviceModel *model);

// 移除模型
int dev_panel_remove_model(DeviceManager *manager, const char *modelId);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_DEV_PANEL_H