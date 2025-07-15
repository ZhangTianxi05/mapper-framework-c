#ifndef GRPCCLIENT_REGISTER_H
#define GRPCCLIENT_REGISTER_H

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明 - 使用你已有的protobuf-c结构体
typedef struct V1beta1__Device V1beta1__Device;
typedef struct V1beta1__DeviceModel V1beta1__DeviceModel;

/**
 * RegisterMapper - 注册Mapper到EdgeCore
 * 
 * 如果 withData 为 true，edgecore 会发送设备和模型列表
 * 
 * @param withData: 是否请求设备和模型数据
 * @param deviceList: 输出参数，设备列表指针数组
 * @param deviceListCount: 输出参数，设备列表数量
 * @param modelList: 输出参数，模型列表指针数组
 * @param modelListCount: 输出参数，模型列表数量
 * 
 * @return 0=成功，-1=失败
 */
int RegisterMapper(
    int withData,
    V1beta1__Device ***deviceList,
    int *deviceListCount,
    V1beta1__DeviceModel ***modelList,
    int *modelListCount
);

/**
 * 释放RegisterMapper返回的设备列表内存
 * @param deviceList: 设备列表
 * @param deviceListCount: 设备数量
 */
void free_device_list(V1beta1__Device **deviceList, int deviceListCount);

/**
 * 释放RegisterMapper返回的模型列表内存
 * @param modelList: 模型列表
 * @param modelListCount: 模型数量
 */
void free_model_list(V1beta1__DeviceModel **modelList, int modelListCount);

#ifdef __cplusplus
}
#endif

#endif // GRPCCLIENT_REGISTER_H