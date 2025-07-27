#ifndef REGISTER_H
#define REGISTER_H
#include <vector>
#include <string>
#include "dmi/v1beta1/api.grpc.pb.h"

// 注册 Mapper 到 EdgeCore，仿照 Go 版本
// 返回值：0=成功，非0=失败
int RegisterMapper(
    bool withData,
    std::vector<v1beta1::Device> &deviceList,
    std::vector<v1beta1::DeviceModel> &modelList
);
#endif // REGISTER_H