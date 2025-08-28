#include <stdio.h>
#include <stdlib.h>
#include "grpcclient/register.h"
#include "common/configmaptype.h"

int main() {
    DeviceInstance *deviceList = NULL;
    int deviceCount = 0;
    DeviceModel *modelList = NULL;
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
        printf("Device[%d]: name=%s\n", i, deviceList[i].name);
    }
    for (int i = 0; i < modelCount; ++i) {
        printf("Model[%d]: name=%s\n", i, modelList[i].name);
    }

    // TODO: 根据 RegisterMapper 的释放约定做相应释放
    free(deviceList);
    free(modelList);
    return 0;
}