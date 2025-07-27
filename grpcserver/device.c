#include "grpcserver/device.h"
#include "util/parse/grpc.h"
#include "log/log.h"
#include <string.h>
#include <stdlib.h>

DeviceService *device_service_new(DevPanel *panel) {
    DeviceService *svc = calloc(1, sizeof(DeviceService));
    svc->dev_panel = panel;
    return svc;
}

void device_service_free(DeviceService *svc) {
    free(svc);
}

int device_register(DeviceService *svc, const V1beta1__RegisterDeviceRequest *req, V1beta1__RegisterDeviceResponse *resp) {
    log_info("RegisterDevice");
    const V1beta1__Device *device = req->device;
    if (!device) return -1;
    char deviceID[256];
    get_resource_id(device->namespace_, device->name, deviceID, sizeof(deviceID));
    if (dev_panel_has_device(svc->dev_panel, deviceID)) {
        resp->devicename = strdup(device->name);
        return 0;
    }
    char modelID[256];
    get_resource_id(device->namespace_, device->spec->devicemodelreference, modelID, sizeof(modelID));
    DeviceModel model;
    if (dev_panel_get_model(svc->dev_panel, modelID, &model) != 0) {
        log_error("deviceModel %s not found", device->spec->devicemodelreference);
        return -2;
    }
    ProtocolConfig protocol;
    if (build_protocol_from_grpc(device, &protocol) != 0) {
        log_error("parse device %s protocol failed", device->name);
        return -3;
    }
    DeviceInstance instance;
    if (get_device_from_grpc(device, &model, &instance) != 0) {
        log_error("parse device %s instance failed", device->name);
        return -4;
    }
    instance.pProtocol = protocol;
    dev_panel_update_dev(svc->dev_panel, &model, &instance);
    resp->devicename = strdup(device->name);
    return 0;
}

int device_remove(DeviceService *svc, const V1beta1__RemoveDeviceRequest *req, V1beta1__RemoveDeviceResponse *resp) {
    if (!req->devicename) return -1;
    char deviceID[256];
    get_resource_id(req->devicenamespace, req->devicename, deviceID, sizeof(deviceID));
    return dev_panel_remove_device(svc->dev_panel, deviceID);
}

int device_update(DeviceService *svc, const V1beta1__UpdateDeviceRequest *req, V1beta1__UpdateDeviceResponse *resp) {
    log_info("UpdateDevice");
    const V1beta1__Device *device = req->device;
    if (!device) return -1;
    char modelID[256];
    get_resource_id(device->namespace_, device->spec->devicemodelreference, modelID, sizeof(modelID));
    DeviceModel model;
    if (dev_panel_get_model(svc->dev_panel, modelID, &model) != 0) {
        log_error("deviceModel %s not found", device->spec->devicemodelreference);
        return -2;
    }
    ProtocolConfig protocol;
    if (build_protocol_from_grpc(device, &protocol) != 0) {
        log_error("parse device %s protocol failed", device->name);
        return -3;
    }
    DeviceInstance instance;
    if (get_device_from_grpc(device, &model, &instance) != 0) {
        log_error("parse device %s instance failed", device->name);
        return -4;
    }
    instance.pProtocol = protocol;
    dev_panel_update_dev(svc->dev_panel, &model, &instance);
    return 0;
}

int device_create_model(DeviceService *svc, const V1beta1__CreateDeviceModelRequest *req, V1beta1__CreateDeviceModelResponse *resp) {
    const V1beta1__DeviceModel *deviceModel = req->model;
    if (!deviceModel) return -1;
    DeviceModel model;
    get_device_model_from_grpc(deviceModel, &model);
    dev_panel_update_model(svc->dev_panel, &model);
    resp->devicemodelname = strdup(deviceModel->name);
    return 0;
}

int device_update_model(DeviceService *svc, const V1beta1__UpdateDeviceModelRequest *req, V1beta1__UpdateDeviceModelResponse *resp) {
    const V1beta1__DeviceModel *deviceModel = req->model;
    if (!deviceModel) return -1;
    char modelID[256];
    get_resource_id(deviceModel->namespace_, deviceModel->name, modelID, sizeof(modelID));
    DeviceModel model;
    if (dev_panel_get_model(svc->dev_panel, modelID, &model) != 0) {
        log_error("update deviceModel %s failed, not existed", deviceModel->name);
        return -2;
    }
    get_device_model_from_grpc(deviceModel, &model);
    dev_panel_update_model(svc->dev_panel, &model);
    return 0;
}

int device_remove_model(DeviceService *svc, const V1beta1__RemoveDeviceModelRequest *req, V1beta1__RemoveDeviceModelResponse *resp) {
    char modelID[256];
    get_resource_id(req->modelnamespace, req->modelname, modelID, sizeof(modelID));
    dev_panel_remove_model(svc->dev_panel, modelID);
    return 0;
}

int device_get(DeviceService *svc, const V1beta1__GetDeviceRequest *req, V1beta1__GetDeviceResponse *resp) {
    if (!req->devicename) return -1;
    char deviceID[256];
    get_resource_id(req->devicenamespace, req->devicename, deviceID, sizeof(deviceID));
    DeviceInstance instance;
    if (dev_panel_get_device(svc->dev_panel, deviceID, &instance) != 0) {
        return -2;
    }
    // 填充 resp->device/status/twins 等字段，需根据你的结构体实现
    resp->device = malloc(sizeof(V1beta1__Device));
    v1beta1__device__init(resp->device);
    resp->device->status = malloc(sizeof(V1beta1__DeviceStatus));
    v1beta1__device_status__init(resp->device->status);
    // twins 需要你自己实现转换
    // resp->device->status->n_twins = ...;
    // resp->device->status->twins = ...;
    return 0;
}