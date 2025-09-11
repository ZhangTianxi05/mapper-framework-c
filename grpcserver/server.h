#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#ifdef __cplusplus
// C++ 部分
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
    std::unique_ptr<grpc::Server> server_;
    bool stopped_ = false;   // 防止重复 Stop
};

#else
// C 部分 - 为 C 代码提供接口
typedef struct ServerConfig ServerConfig;
typedef struct GrpcServer GrpcServer;

// C 接口函数
ServerConfig *server_config_new(const char *sock_path, const char *protocol);
void server_config_free(ServerConfig *config);

GrpcServer *grpcserver_new(ServerConfig *config, DeviceManager *device_manager);
int grpcserver_start(GrpcServer *server);
void grpcserver_stop(GrpcServer *server);
void grpcserver_free(GrpcServer *server);

#endif // __cplusplus

#endif // GRPC_SERVER_H