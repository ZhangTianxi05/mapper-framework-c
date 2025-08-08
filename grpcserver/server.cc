#include <grpcpp/grpcpp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include "dmi/v1beta1/api.grpc.pb.h"
#include "log/log.h"
#include "common/configmaptype.h"
#include "server.h"
#include "device/device.h"

// 你的设备面板类实现
class DevPanel {
public:
    DevPanel() {}
    ~DevPanel() {}
    
    // TODO: 添加实际的设备面板功能
    void processDevice(const std::string& deviceId) {
        // 占位实现
    }
};

// 1. 继承 proto 生成的 Service
class DeviceMapperServiceImpl final : public v1beta1::DeviceMapperService::Service {
public:
    explicit DeviceMapperServiceImpl(std::shared_ptr<DevPanel> devPanel) : devPanel_(devPanel) {}

    // 实现各种 gRPC 服务方法
    ::grpc::Status RegisterDevice(::grpc::ServerContext* context,
                                  const ::v1beta1::RegisterDeviceRequest* request,
                                  ::v1beta1::RegisterDeviceResponse* response) override {
        log_info("RegisterDevice called");
        // TODO: 你的业务逻辑
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status RemoveDevice(::grpc::ServerContext* context,
                                const ::v1beta1::RemoveDeviceRequest* request,
                                ::v1beta1::RemoveDeviceResponse* response) override {
        log_info("RemoveDevice called");
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status UpdateDevice(::grpc::ServerContext* context,
                                const ::v1beta1::UpdateDeviceRequest* request,
                                ::v1beta1::UpdateDeviceResponse* response) override {
        log_info("UpdateDevice called");
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status CreateDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::CreateDeviceModelRequest* request,
                                     ::v1beta1::CreateDeviceModelResponse* response) override {
        log_info("CreateDeviceModel called");
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status RemoveDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::RemoveDeviceModelRequest* request,
                                     ::v1beta1::RemoveDeviceModelResponse* response) override {
        log_info("RemoveDeviceModel called");
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status UpdateDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::UpdateDeviceModelRequest* request,
                                     ::v1beta1::UpdateDeviceModelResponse* response) override {
        log_info("UpdateDeviceModel called");
        return ::grpc::Status::OK;
    }
    
    ::grpc::Status GetDevice(::grpc::ServerContext* context,
                             const ::v1beta1::GetDeviceRequest* request,
                             ::v1beta1::GetDeviceResponse* response) override {
        log_info("GetDevice called");
        return ::grpc::Status::OK;
    }

private:
    std::shared_ptr<DevPanel> devPanel_;
};

// ❌ 删除这些重复定义，它们已经在 server.h 中定义了
// struct ServerConfig { ... }
// class GrpcServer { ... }

// ✅ 只保留 ServerConfig 构造函数的实现
ServerConfig::ServerConfig(const std::string& sock_path, const std::string& protocol)
    : sockPath(sock_path), protocol(protocol) {}

// ✅ 只保留 GrpcServer 方法的实现
GrpcServer::GrpcServer(const ServerConfig& cfg, std::shared_ptr<DevPanel> devPanel)
    : cfg_(cfg), devPanel_(devPanel) {}

int GrpcServer::Start() {
    log_info("uds socket path: %s", cfg_.sockPath.c_str());
    
    // 清理已存在的 socket 文件
    struct stat st;
    if (stat(cfg_.sockPath.c_str(), &st) == 0) {
        if (unlink(cfg_.sockPath.c_str()) != 0) {
            log_error("Failed to remove uds socket: %s", cfg_.sockPath.c_str());
            return -1;
        }
    }

    std::string server_address = "unix://" + cfg_.sockPath;
    DeviceMapperServiceImpl service(devPanel_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        log_error("failed to start grpc server");
        return -1;
    }
    
    log_info("start grpc server on %s", server_address.c_str());
    server->Wait();
    return 0;
}

void GrpcServer::Stop() {
    log_info("Stopping gRPC server...");
    
    // 清理 socket 文件
    if (unlink(cfg_.sockPath.c_str()) != 0) {
        log_warn("failed to remove uds socket: %s", cfg_.sockPath.c_str());
    }
    
    log_info("gRPC server stopped");
}

// C 接口实现
extern "C" {

ServerConfig *server_config_new(const char *sock_path, const char *protocol) {
    if (!sock_path || !protocol) {
        log_error("Invalid parameters for server config creation");
        return nullptr;
    }
    
    try {
        return new ServerConfig(std::string(sock_path), std::string(protocol));
    } catch (const std::exception& e) {
        log_error("Failed to create server config: %s", e.what());
        return nullptr;
    }
}

void server_config_free(ServerConfig *config) {
    if (config) {
        delete config;
    }
}

GrpcServer *grpcserver_new(ServerConfig *config, DeviceManager *device_manager) {
    if (!config || !device_manager) {
        log_error("Invalid parameters for gRPC server creation");
        return nullptr;
    }
    
    try {
        // 创建 DevPanel 适配器
        auto devPanel = std::make_shared<DevPanel>();
        
        return new GrpcServer(*config, devPanel);
    } catch (const std::exception& e) {
        log_error("Failed to create gRPC server: %s", e.what());
        return nullptr;
    }
}

int grpcserver_start(GrpcServer *server) {
    if (!server) {
        log_error("Invalid gRPC server pointer");
        return -1;
    }
    
    try {
        return server->Start();
    } catch (const std::exception& e) {
        log_error("Failed to start gRPC server: %s", e.what());
        return -1;
    }
}

void grpcserver_stop(GrpcServer *server) {
    if (!server) {
        log_warn("Trying to stop NULL gRPC server");
        return;
    }
    
    try {
        server->Stop();
    } catch (const std::exception& e) {
        log_error("Failed to stop gRPC server: %s", e.what());
    }
}

void grpcserver_free(GrpcServer *server) {
    if (server) {
        try {
            server->Stop(); // 确保先停止服务器
            delete server;
        } catch (const std::exception& e) {
            log_error("Failed to free gRPC server: %s", e.what());
        }
    }
}

} // extern "C"