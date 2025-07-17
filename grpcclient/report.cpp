#include <grpcpp/grpcpp.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <string>
#include "config/config.h"
#include "dmi/v1beta1/api.grpc.pb.h"

using namespace std;

// 仿照Go: ReportDeviceStatus
int ReportDeviceStatus(const v1beta1::ReportDeviceStatusRequest &request) {
    Config *cfg = config_parse("config.yaml");
    if (!cfg) return -1;

    std::string sock_addr = "unix://" + std::string(cfg->common.edgecore_sock);
    auto channel = grpc::CreateChannel(sock_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<v1beta1::DeviceManagerService::Stub> stub = v1beta1::DeviceManagerService::NewStub(channel);

    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(1));
    v1beta1::ReportDeviceStatusResponse resp;
    grpc::Status status = stub->ReportDeviceStatus(&ctx, request, &resp);

    config_free(cfg);
    return status.ok() ? 0 : -1;
}

// 仿照Go: ReportDeviceStates
int ReportDeviceStates(const v1beta1::ReportDeviceStatesRequest &request) {
    Config *cfg = config_parse("config.yaml");
    if (!cfg) return -1;

    std::string sock_addr = "unix://" + std::string(cfg->common.edgecore_sock);
    auto channel = grpc::CreateChannel(sock_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<v1beta1::DeviceManagerService::Stub> stub = v1beta1::DeviceManagerService::NewStub(channel);

    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(1));
    v1beta1::ReportDeviceStatesResponse resp;
    grpc::Status status = stub->ReportDeviceStates(&ctx, request, &resp);

    config_free(cfg);
    return status.ok() ? 0 : -1;
}