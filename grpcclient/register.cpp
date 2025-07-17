#include <grpcpp/grpcpp.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include "config/config.h"
#include "common/const.h"
#include "dmi/v1beta1/api.grpc.pb.h"

using namespace std;

// 仿照Go: RegisterMapper
int RegisterMapper(
    bool withData,
    std::vector<v1beta1::Device> &deviceList,
    std::vector<v1beta1::DeviceModel> &modelList
) {
    Config *cfg = config_parse("config.yaml");
    if (!cfg) return -1;

    // 构造 Unix 域套接字地址
    std::string sock_addr = "unix://" + std::string(cfg->common.edgecore_sock);

    // 创建 gRPC channel
    auto channel = grpc::CreateChannel(sock_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<v1beta1::DeviceManagerService::Stub> stub = v1beta1::DeviceManagerService::NewStub(channel);

    // 构造 MapperInfo
    v1beta1::MapperInfo mapper;
    mapper.set_name(cfg->common.name);
    mapper.set_version(cfg->common.version);
    mapper.set_api_version(cfg->common.api_version);
    mapper.set_protocol(cfg->common.protocol);
    mapper.set_address(cfg->common.address, strlen(cfg->common.address));
    mapper.set_state(DEVICE_STATUS_OK);

    // 构造请求
    v1beta1::MapperRegisterRequest req;
    req.set_withdata(withData);
    *req.mutable_mapper() = mapper;

    // 发起RPC
    v1beta1::MapperRegisterResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub->MapperRegister(&ctx, req, &resp);

    if (!status.ok()) {
        config_free(cfg);
        return -1;
    }

    // 拷贝结果
    deviceList.assign(resp.devicelist().begin(), resp.devicelist().end());
    modelList.assign(resp.modellist().begin(), resp.modellist().end());

    config_free(cfg);
    return 0;
}