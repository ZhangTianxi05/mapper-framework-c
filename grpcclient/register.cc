#include "grpcclient/register.h"
#include <grpcpp/grpcpp.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include "config/config.h"
#include "common/const.h"
#include "dmi/v1beta1/api.grpc.pb.h"
#include "dmi/v1beta1/api.pb-c.h"
#include "util/parse/grpc.h"
#include "common/datamodel.h"
#include "log/log.h"  // 新增

using namespace std;

// 原始 C++ 实现，返回 protobuf 对象
static int RegisterMapperCpp(
    bool withData,
    std::vector<v1beta1::Device> &deviceList,
    std::vector<v1beta1::DeviceModel> &modelList
) {
    const char* env_sock = getenv("EDGECORE_SOCK");
    std::string sock_path;
    Config *cfg = nullptr;

    if (env_sock && *env_sock) {
        sock_path = env_sock;
    } else {
        // 优先 ./config.yaml，回退 ../config.yaml（从 build 目录运行的常见情况）
        cfg = config_parse("config.yaml");
        if (!cfg) cfg = config_parse("../config.yaml");
        if (!cfg) {
            log_error("RegisterMapper: config.yaml not found (tried ./ and ../)");
            return -1;
        }
        if (cfg->common.edgecore_sock[0]) {
            sock_path = cfg->common.edgecore_sock;
        } else {
            sock_path = "/var/lib/kubeedge/kubeedge.sock";
        }
    }

    std::string sock_addr = "unix://" + sock_path;
    auto channel = grpc::CreateChannel(sock_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<v1beta1::DeviceManagerService::Stub> stub = v1beta1::DeviceManagerService::NewStub(channel);

    v1beta1::MapperInfo mapper;
    if (!cfg) {
        mapper.set_name("arduino-mapper");
        mapper.set_version("v1.13.0");
        mapper.set_api_version("v1.0.0");
        mapper.set_protocol("modbus-tcp");
        mapper.set_address("/tmp/mapper_dmi.sock");
    } else {
        mapper.set_name(cfg->common.name);
        mapper.set_version(cfg->common.version);
        mapper.set_api_version(cfg->common.api_version);
        mapper.set_protocol(cfg->common.protocol);
        // 关键：告知 EdgeCore 回连到我们的 UDS
        mapper.set_address(cfg->grpc_server.socket_path);
    }
    mapper.set_state(DEVICE_STATUS_OK);

    v1beta1::MapperRegisterRequest req;
    req.set_withdata(withData);
    *req.mutable_mapper() = mapper;

    v1beta1::MapperRegisterResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub->MapperRegister(&ctx, req, &resp);

    if (!status.ok()) {
        log_error("MapperRegister RPC failed: code=%d msg=%s",
                  (int)status.error_code(), status.error_message().c_str());
        if (cfg) config_free(cfg);
        return -1;
    }

    deviceList.assign(resp.devicelist().begin(), resp.devicelist().end());
    modelList.assign(resp.modellist().begin(), resp.modellist().end());
    if (cfg) config_free(cfg);
    return 0;
}

// C 风格 wrapper，返回 C 结构体数组
extern "C" int RegisterMapper(
    int withData,
    DeviceInstance **outDeviceList, int *outDeviceCount,
    DeviceModel **outModelList, int *outModelCount
) {
    std::vector<v1beta1::Device> devList;
    std::vector<v1beta1::DeviceModel> mdlList;
    int ret = RegisterMapperCpp(withData != 0, devList, mdlList);
    if (ret != 0) return ret;

    // 分配 C 结构体数组
    if (outDeviceList && outDeviceCount) {
        *outDeviceCount = devList.size();
        *outDeviceList = (DeviceInstance*)calloc(*outDeviceCount, sizeof(DeviceInstance));
        for (int i = 0; i < *outDeviceCount; ++i) {
            // 转换 C++ Device 到 protobuf-c Device
            V1beta1__Device pb_dev = V1BETA1__DEVICE__INIT;
            pb_dev.name = strdup(devList[i].name().c_str());
            pb_dev.namespace_ = strdup(devList[i].namespace_().c_str());
            // ...如有 spec 等字段可补充...
            get_device_from_grpc(&pb_dev, NULL, &((*outDeviceList)[i]));
            free(pb_dev.name);
            free(pb_dev.namespace_);
        }
    }
    if (outModelList && outModelCount) {
        *outModelCount = mdlList.size();
        *outModelList = (DeviceModel*)calloc(*outModelCount, sizeof(DeviceModel));
        for (int i = 0; i < *outModelCount; ++i) {
            V1beta1__DeviceModel pb_model = V1BETA1__DEVICE_MODEL__INIT;
            pb_model.name = strdup(mdlList[i].name().c_str());
            pb_model.namespace_ = strdup(mdlList[i].namespace_().c_str());
            // ...如有 spec 等字段可补充...
            get_device_model_from_grpc(&pb_model, &((*outModelList)[i]));
            free(pb_model.name);
            free(pb_model.namespace_);
        }
    }
    return 0;
}