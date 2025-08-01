#ifndef UTIL_PARSE_GRPC_H
#define UTIL_PARSE_GRPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dmi/v1beta1/api.pb-c.h"
#include "common/configmaptype.h"
#include "common/datamodel.h"
#include "common/datamethod.h"
#include "common/dataconverter.h"
#include "log/log.h"

// 获取协议名
int get_protocol_name_from_grpc(const V1beta1__Device *device, char **out);

// 构建 ProtocolConfig
int build_protocol_from_grpc(const V1beta1__Device *device, ProtocolConfig *out);

// 构建 twins 数组
int build_twins_from_grpc(const V1beta1__Device *device, Twin **out, int *out_count);

// 构建 properties 数组
int build_properties_from_grpc(const V1beta1__Device *device, DeviceProperty **out, int *out_count);

// 构建 methods 数组
int build_methods_from_grpc(const V1beta1__Device *device, DeviceMethod **out, int *out_count);

// 构建 DeviceModel
int get_device_model_from_grpc(const V1beta1__DeviceModel *model, DeviceModel *out);

// 构建 DeviceInstance
int get_device_from_grpc(const V1beta1__Device *device, const DeviceModel *commonModel, DeviceInstance *out);

void get_resource_id(const char *ns, const char *name, char *out, size_t outlen);

#ifdef __cplusplus
}
#endif

#endif // UTIL_PARSE_GRPC_H