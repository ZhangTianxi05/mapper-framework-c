#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H
#include <string>
#include <memory>

// 你的设备面板类声明
class DevPanel;

// 仿照Go的Config结构体
struct ServerConfig {
    std::string sockPath;
    std::string protocol;
    ServerConfig(const std::string& sock_path, const std::string& protocol);
};

// gRPC Server 封装
class GrpcServer {
public:
    GrpcServer(const ServerConfig& cfg, std::shared_ptr<DevPanel> devPanel);

    int Start();
    void Stop();

private:
    ServerConfig cfg_;
    std::shared_ptr<DevPanel> devPanel_;
};
#endif // GRPC_SERVER_H