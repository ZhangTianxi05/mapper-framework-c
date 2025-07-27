#ifndef GRPCSERVER_DEVICE_H
#define GRPCSERVER_DEVICE_H

#include "dmi/v1beta1/api.pb-c.h"

// 前向声明
typedef struct DevPanel DevPanel;

// 设备服务结构体
typedef struct {
    DevPanel *dev_panel;
} DeviceService;

// 创建/销毁服务
DeviceService *device_service_new(DevPanel *panel);
void device_service_free(DeviceService *svc);

// gRPC接口
int device_register(DeviceService *svc, const V1beta1__RegisterDeviceRequest *req, V1beta1__RegisterDeviceResponse *resp);
int device_remove(DeviceService *svc, const V1beta1__RemoveDeviceRequest *req, V1beta1__RemoveDeviceResponse *resp);
int device_update(DeviceService *svc, const V1beta1__UpdateDeviceRequest *req, V1beta1__UpdateDeviceResponse *resp);
int device_create_model(DeviceService *svc, const V1beta1__CreateDeviceModelRequest *req, V1beta1__CreateDeviceModelResponse *resp);
int device_update_model(DeviceService *svc, const V1beta1__UpdateDeviceModelRequest *req, V1beta1__UpdateDeviceModelResponse *resp);
int device_remove_model(DeviceService *svc, const V1beta1__RemoveDeviceModelRequest *req, V1beta1__RemoveDeviceModelResponse *resp);
int device_get(DeviceService *svc, const V1beta1__GetDeviceRequest *req, V1beta1__GetDeviceResponse *resp);

#endif // GRPCSERVER_DEVICE_H