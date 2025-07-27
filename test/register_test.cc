#include <iostream>
#include <vector>
#include "dmi/v1beta1/api.grpc.pb.h"

// 声明 RegisterMapper
int RegisterMapper(
    bool withData,
    std::vector<v1beta1::Device> &deviceList,
    std::vector<v1beta1::DeviceModel> &modelList
);

int main() {
    std::vector<v1beta1::Device> deviceList;
    std::vector<v1beta1::DeviceModel> modelList;

    int ret = RegisterMapper(true, deviceList, modelList);
    if (ret != 0) {
        std::cerr << "RegisterMapper failed!" << std::endl;
        return 1;
    }

    std::cout << "RegisterMapper success!" << std::endl;
    std::cout << "Device count: " << deviceList.size() << std::endl;
    std::cout << "Model count: " << modelList.size() << std::endl;

    // 打印部分内容
    for (size_t i = 0; i < deviceList.size(); ++i) {
        std::cout << "Device[" << i << "]: name=" << deviceList[i].name() << std::endl;
    }
    for (size_t i = 0; i < modelList.size(); ++i) {
        std::cout << "Model[" << i << "]: name=" << modelList[i].name() << std::endl;
    }
    return 0;
}