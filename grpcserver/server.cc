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
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h> 
#include <fcntl.h>
class DevPanel {
public:
    DevPanel() {}
    ~DevPanel() {}
    
    void processDevice(const std::string& deviceId) {
    }
};

class DeviceMapperServiceImpl final : public v1beta1::DeviceMapperService::Service {
public:
    explicit DeviceMapperServiceImpl(std::shared_ptr<DevPanel> devPanel) : devPanel_(devPanel) {}

    ::grpc::Status RegisterDevice(::grpc::ServerContext* context,
                                  const ::v1beta1::RegisterDeviceRequest* request,
                                  ::v1beta1::RegisterDeviceResponse* response) override {
        log_info("RegisterDevice called");
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


ServerConfig::ServerConfig(const std::string& sock_path, const std::string& protocol)
    : sockPath(sock_path), protocol(protocol) {}


GrpcServer::GrpcServer(const ServerConfig& cfg, std::shared_ptr<DevPanel> devPanel)
    : cfg_(cfg), devPanel_(devPanel) {}

int GrpcServer::Start() {
    log_info("uds socket path: %s", cfg_.sockPath.c_str());
    
    struct stat st;
    if (stat(cfg_.sockPath.c_str(), &st) == 0) {
        if (unlink(cfg_.sockPath.c_str()) != 0) {
            log_error("Failed to remove uds socket: %s", cfg_.sockPath.c_str());
            return -1;
        }
    }
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

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
     for (int i = 0; i < 100; ++i) {
        if (stat(cfg_.sockPath.c_str(), &st) == 0) {
            if (chmod(cfg_.sockPath.c_str(), 0666) != 0) {
                log_warn("chmod %s to 0666 failed", cfg_.sockPath.c_str());
            }
            break;
        }
        usleep(10000);
    }
    
    log_info("start grpc server on %s", server_address.c_str());
    server->Wait();
    return 0;
}

void GrpcServer::Stop() {
    log_info("Stopping gRPC server...");
    
    if (unlink(cfg_.sockPath.c_str()) != 0) {
        log_warn("failed to remove uds socket: %s", cfg_.sockPath.c_str());
    }
    
    log_info("gRPC server stopped");
}

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
            server->Stop(); 
            delete server;
        } catch (const std::exception& e) {
            log_error("Failed to free gRPC server: %s", e.what());
        }
    }
}

} // extern "C"