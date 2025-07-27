#include <grpcpp/grpcpp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include "dmi/v1beta1/api.grpc.pb.h"
#include "log/log.h"
#include "common/configmaptype.h"

// 你的设备面板类声明
class DevPanel {};

// 1. 继承 proto 生成的 Service
class DeviceMapperServiceImpl final : public v1beta1::DeviceMapperService::Service {
public:
    explicit DeviceMapperServiceImpl(std::shared_ptr<DevPanel> devPanel) : devPanel_(devPanel) {}

    // 下面这些方法必须实现，哪怕只写空实现
    ::grpc::Status RegisterDevice(::grpc::ServerContext* context,
                                  const ::v1beta1::RegisterDeviceRequest* request,
                                  ::v1beta1::RegisterDeviceResponse* response) override {
        // TODO: 你的业务逻辑
        return ::grpc::Status::OK;
    }
    ::grpc::Status RemoveDevice(::grpc::ServerContext* context,
                                const ::v1beta1::RemoveDeviceRequest* request,
                                ::v1beta1::RemoveDeviceResponse* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status UpdateDevice(::grpc::ServerContext* context,
                                const ::v1beta1::UpdateDeviceRequest* request,
                                ::v1beta1::UpdateDeviceResponse* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status CreateDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::CreateDeviceModelRequest* request,
                                     ::v1beta1::CreateDeviceModelResponse* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status RemoveDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::RemoveDeviceModelRequest* request,
                                     ::v1beta1::RemoveDeviceModelResponse* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status UpdateDeviceModel(::grpc::ServerContext* context,
                                     const ::v1beta1::UpdateDeviceModelRequest* request,
                                     ::v1beta1::UpdateDeviceModelResponse* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status GetDevice(::grpc::ServerContext* context,
                             const ::v1beta1::GetDeviceRequest* request,
                             ::v1beta1::GetDeviceResponse* response) override {
        return ::grpc::Status::OK;
    }

private:
    std::shared_ptr<DevPanel> devPanel_;
};

// 仿照Go的Config结构体
struct ServerConfig {
    std::string sockPath;
    std::string protocol;
    ServerConfig(const std::string& sock_path, const std::string& protocol)
        : sockPath(sock_path), protocol(protocol) {}
};

class GrpcServer {
public:
    GrpcServer(const ServerConfig& cfg, std::shared_ptr<DevPanel> devPanel)
        : cfg_(cfg), devPanel_(devPanel) {}

    int Start() {
        log_info("uds socket path: %s", cfg_.sockPath.c_str());
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
            log_fatal("failed to start grpc server");
            return -1;
        }
        log_info("start grpc server");
        server->Wait();
        return 0;
    }

    void Stop() {
        // 这里只做 socket 文件清理，实际可扩展
        if (unlink(cfg_.sockPath.c_str()) != 0) {
            log_warn("failed to remove uds socket: %s", cfg_.sockPath.c_str());
        }
    }

private:
    ServerConfig cfg_;
    std::shared_ptr<DevPanel> devPanel_;
};