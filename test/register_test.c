#include <stdio.h>
#include <stdlib.h>
#include "grpcclient/register.h"
#include "dmi/v1beta1/api.pb-c.h"

int main() {
    V1beta1__Device **deviceList = NULL;
    int deviceCount = 0;
    V1beta1__DeviceModel **modelList = NULL;
    int modelCount = 0;

    int ret = RegisterMapper(1, &deviceList, &deviceCount, &modelList, &modelCount);
    if (ret != 0) {
        printf("RegisterMapper failed!\n");
        return 1;
    }

    printf("RegisterMapper success!\n");
    printf("Device count: %d\n", deviceCount);
    printf("Model count: %d\n", modelCount);

    for (int i = 0; i < deviceCount; ++i) {
        printf("Device[%d]: name=%s\n", i, deviceList[i]->name);
    }
    for (int i = 0; i < modelCount; ++i) {
        printf("Model[%d]: name=%s\n", i, modelList[i]->name);
    }

    // 释放内存
    for (int i = 0; i < deviceCount; ++i) {
        v1beta1__device__free_unpacked(deviceList[i], NULL);
    }
    free(deviceList);
    for (int i = 0; i < modelCount; ++i) {
        v1beta1__device_model__free_unpacked(modelList[i], NULL);
    }
    free(modelList);

    return 0;
}