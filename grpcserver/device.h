#pragma once

#include "dmi/v1beta1/api.grpc.pb.h"
#include <memory>

// 前置声明
class DevicePanel;
class DeviceModel;
class DeviceInstance;
class ProtocolConfig;

// 设备服务实现，继承 proto 生成的 Service
class DeviceServiceImpl : public v1beta1::DeviceMapperService::Service {
public:
    // 构造函数
    explicit DeviceServiceImpl(std::shared_ptr<DevicePanel> dev_panel)
        : dev_panel_(dev_panel) {}

    // gRPC 方法实现
    grpc::Status RegisterDevice(grpc::ServerContext* context,
                               const v1beta1::RegisterDeviceRequest* request,
                               v1beta1::RegisterDeviceResponse* response) override;

    grpc::Status RemoveDevice(grpc::ServerContext* context,
                              const v1beta1::RemoveDeviceRequest* request,
                              v1beta1::RemoveDeviceResponse* response) override;

    grpc::Status UpdateDevice(grpc::ServerContext* context,
                              const v1beta1::UpdateDeviceRequest* request,
                              v1beta1::UpdateDeviceResponse* response) override;

    grpc::Status CreateDeviceModel(grpc::ServerContext* context,
                                   const v1beta1::CreateDeviceModelRequest* request,
                                   v1beta1::CreateDeviceModelResponse* response) override;

    grpc::Status UpdateDeviceModel(grpc::ServerContext* context,
                                   const v1beta1::UpdateDeviceModelRequest* request,
                                   v1beta1::UpdateDeviceModelResponse* response) override;

    grpc::Status RemoveDeviceModel(grpc::ServerContext* context,
                                   const v1beta1::RemoveDeviceModelRequest* request,
                                   v1beta1::RemoveDeviceModelResponse* response) override;

    grpc::Status GetDevice(grpc::ServerContext* context,
                           const v1beta1::GetDeviceRequest* request,
                           v1beta1::GetDeviceResponse* response) override;

private:
    std::shared_ptr<DevicePanel> dev_panel_;
};