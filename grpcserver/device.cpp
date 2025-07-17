#include "grpcserver/device.h"
#include "dmi/v1beta1/api.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include "common/configmaptype.h"
#include "util/parse/grpc.h"

// RegisterDevice
grpc::Status DeviceServiceImpl::RegisterDevice(
    grpc::ServerContext* context,
    const v1beta1::RegisterDeviceRequest* request,
    v1beta1::RegisterDeviceResponse* response) {

    const auto& device = request->device();
    if (!device.has_name()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "device is nil");
    }
    std::string deviceID = device.namespace_() + "/" + device.name();

    // 已注册直接返回
    if (dev_panel_->HasDevice(deviceID)) {
        response->set_devicename(device.name());
        return grpc::Status::OK;
    }

    // 查找模型
    std::string modelID = device.namespace_() + "/" + device.spec().devicemodelreference();
    DeviceModel model;
    if (!dev_panel_->GetModel(modelID, &model)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "deviceModel not found");
    }

    // 构建协议
    ProtocolConfig protocol;
    if (build_protocol_from_grpc(&device, &protocol) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse protocol failed");
    }

    // 构建实例
    DeviceInstance instance;
    if (get_device_from_grpc(&device, &model, &instance) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse device instance failed");
    }
    instance.pProtocol = protocol;

    dev_panel_->UpdateDev(&model, &instance);

    response->set_devicename(device.name());
    return grpc::Status::OK;
}

// RemoveDevice
grpc::Status DeviceServiceImpl::RemoveDevice(
    grpc::ServerContext* context,
    const v1beta1::RemoveDeviceRequest* request,
    v1beta1::RemoveDeviceResponse* response) {

    if (request->devicename().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "device name is nil");
    }
    std::string deviceID = request->devicenamespace() + "/" + request->devicename();
    int ret = dev_panel_->RemoveDevice(deviceID);
    if (ret != 0) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "device not found");
    }
    return grpc::Status::OK;
}

// UpdateDevice
grpc::Status DeviceServiceImpl::UpdateDevice(
    grpc::ServerContext* context,
    const v1beta1::UpdateDeviceRequest* request,
    v1beta1::UpdateDeviceResponse* response) {

    const auto& device = request->device();
    if (!device.has_name()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "device is nil");
    }
    std::string modelID = device.namespace_() + "/" + device.spec().devicemodelreference();
    DeviceModel model;
    if (!dev_panel_->GetModel(modelID, &model)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "deviceModel not found");
    }
    ProtocolConfig protocol;
    if (build_protocol_from_grpc(&device, &protocol) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse protocol failed");
    }
    DeviceInstance instance;
    if (get_device_from_grpc(&device, &model, &instance) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse device instance failed");
    }
    instance.pProtocol = protocol;

    dev_panel_->UpdateDev(&model, &instance);

    return grpc::Status::OK;
}

// CreateDeviceModel
grpc::Status DeviceServiceImpl::CreateDeviceModel(
    grpc::ServerContext* context,
    const v1beta1::CreateDeviceModelRequest* request,
    v1beta1::CreateDeviceModelResponse* response) {

    const auto& deviceModel = request->model();
    if (!deviceModel.has_name()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "deviceModel is nil");
    }
    DeviceModel model;
    if (get_device_model_from_grpc(&deviceModel, &model) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse deviceModel failed");
    }
    dev_panel_->UpdateModel(&model);

    response->set_devicemodelname(deviceModel.name());
    return grpc::Status::OK;
}

// UpdateDeviceModel
grpc::Status DeviceServiceImpl::UpdateDeviceModel(
    grpc::ServerContext* context,
    const v1beta1::UpdateDeviceModelRequest* request,
    v1beta1::UpdateDeviceModelResponse* response) {

    const auto& deviceModel = request->model();
    if (!deviceModel.has_name()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "deviceModel is nil");
    }
    std::string modelID = deviceModel.namespace_() + "/" + deviceModel.name();
    DeviceModel model;
    if (!dev_panel_->GetModel(modelID, &model)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "deviceModel not found");
    }
    if (get_device_model_from_grpc(&deviceModel, &model) != 0) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "parse deviceModel failed");
    }
    dev_panel_->UpdateModel(&model);

    return grpc::Status::OK;
}

// RemoveDeviceModel
grpc::Status DeviceServiceImpl::RemoveDeviceModel(
    grpc::ServerContext* context,
    const v1beta1::RemoveDeviceModelRequest* request,
    v1beta1::RemoveDeviceModelResponse* response) {

    std::string modelID = request->modelnamespace() + "/" + request->modelname();
    dev_panel_->RemoveModel(modelID);
    return grpc::Status::OK;
}

// GetDevice
grpc::Status DeviceServiceImpl::GetDevice(
    grpc::ServerContext* context,
    const v1beta1::GetDeviceRequest* request,
    v1beta1::GetDeviceResponse* response) {

    if (request->devicename().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "device name is nil");
    }
    std::string deviceID = request->devicenamespace() + "/" + request->devicename();
    DeviceInstance instance;
    if (!dev_panel_->GetDevice(deviceID, &instance)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "device not found");
    }
    // 这里只简单返回 device 字段，twins/status 可按需补充
    v1beta1::Device* resp_device = response->mutable_device();
    resp_device->set_name(instance.name);
    resp_device->set_namespace_(instance.namespace_);
    // ...可补充其它字段...

    return grpc::Status::OK;
}