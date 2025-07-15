#include "util/parse/grpc.h"
#include "dmi/v1beta1/api.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // 初始化 device 结构体及其成员
    V1beta1__Device device = V1BETA1__DEVICE__INIT;
    device.name = strdup("test-device");
    device.namespace_ = strdup("default");

    device.spec = malloc(sizeof(V1beta1__DeviceSpec));
    v1beta1__device_spec__init(device.spec);

    device.spec->protocol = malloc(sizeof(V1beta1__ProtocolConfig));
    v1beta1__protocol_config__init(device.spec->protocol);
    device.spec->protocol->protocolname = strdup("test-protocol");

    device.spec->devicemodelreference = strdup("test-model");

    // 可根据需要继续初始化 properties/methods 等

    DeviceModel model = {0};
    model.name = strdup("test-model");
    model.namespace_ = strdup("default");

    DeviceInstance instance = {0};
    get_device_from_grpc(&device, &model, &instance);

    // 打印 instance 字段检查
    printf("DeviceInstance name: %s\n", instance.name ? instance.name : "(null)");
    printf("DeviceInstance protocolName: %s\n", instance.protocolName ? instance.protocolName : "(null)");
    printf("DeviceInstance model: %s\n", instance.model ? instance.model : "(null)");

    // 释放 device 相关内存
    free(device.name);
    free(device.namespace_);
    free(device.spec->protocol->protocolname);
    free(device.spec->protocol);
    free(device.spec->devicemodelreference);
    free(device.spec);

    // 释放 model 相关内存
    free(model.name);

    // 释放 instance 相关内存（如有分配）

    return 0;
}