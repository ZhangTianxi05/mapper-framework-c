#ifndef GLOBAL_GLOBAL_H
#define GLOBAL_GLOBAL_H

#include "common/configmaptype.h"
#include "common/eventtype.h"
#include "common/datamodel.h"
#include "common/datamethod.h"

// 设备面板结构体
typedef struct DevPanel DevPanel;

// 创建/销毁
DevPanel *dev_panel_new(void);
void dev_panel_free(DevPanel *panel);

// 启动设备采集/推送
void dev_panel_start(DevPanel *panel);

// 初始化设备面板（注册/配置模式）
int dev_panel_init(DevPanel *panel, DeviceInstance *deviceList, int deviceCount, DeviceModel *modelList, int modelCount);

// 更新设备配置并重启
void dev_panel_update_dev(DevPanel *panel, const DeviceModel *model, const DeviceInstance *device);

// 更新设备Twin配置并重启
int dev_panel_update_dev_twins(DevPanel *panel, const char *deviceID, const Twin *twins, int twinCount);

// 获取设备Twin数据
int dev_panel_deal_device_twin_get(DevPanel *panel, const char *deviceID, const char *twinName, char **out_value);

// 获取设备实例信息
int dev_panel_get_device(DevPanel *panel, const char *deviceID, DeviceInstance *out);

// 移除设备
int dev_panel_remove_device(DevPanel *panel, const char *deviceID);

// 写设备属性
int dev_panel_write_device(DevPanel *panel, const char *deviceMethodName, const char *deviceID, const char *propertyName, const char *data);

// 获取模型信息
int dev_panel_get_model(DevPanel *panel, const char *modelID, DeviceModel *out);

// 只更新模型
void dev_panel_update_model(DevPanel *panel, const DeviceModel *model);

// 只移除模型
void dev_panel_remove_model(DevPanel *panel, const char *modelID);

// 获取Twin结果
int dev_panel_get_twin_result(DevPanel *panel, const char *deviceID, const char *twinName, char **out_value, char **out_datatype);

// 获取设备方法
int dev_panel_get_device_method(DevPanel *panel, const char *deviceID, /* out */ char ***out_method_map, int *out_method_count, char ***out_property_map, int *out_property_count);

#endif // GLOBAL_GLOBAL_H