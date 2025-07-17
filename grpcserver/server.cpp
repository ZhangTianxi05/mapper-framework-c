#include <grpcpp/grpcpp.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <iostream>
#include <filesystem>
#include "dmi/v1beta1/api.grpc.pb.h"
#include "grpcserver/device.h"

class Server final {
public:
    Server(const std::string& sock_path, std::shared_ptr<DevicePanel> dev_panel)
        : sock_path_(sock_path), dev_panel_(dev_panel) {}

    void Start() {
        // Remove old socket if exists
        std::filesystem::remove(sock_path_);

        grpc::ServerBuilder builder;
        builder.AddListeningPort("unix://" + sock_path_, grpc::InsecureServerCredentials());
        device_service_ = std::make_unique<DeviceServiceImpl>(dev_panel_);
        builder.RegisterService(device_service_.get());

        server_ = builder.BuildAndStart();
        std::cout << "gRPC server listening on unix://" << sock_path_ << std::endl;
        server_->Wait();
    }

    void Stop() {
        if (server_) server_->Shutdown();
        std::filesystem::remove(sock_path_);
    }

private:
    std::string sock_path_;
    std::shared_ptr<DevicePanel> dev_panel_;
    std::unique_ptr<DeviceServiceImpl> device_service_;
    std::unique_ptr<grpc::Server> server_;
};