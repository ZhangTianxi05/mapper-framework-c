#ifndef PUBLISHER_H
#define PUBLISHER_H

#include "common/datamodel.h"
#include "driver/driver.h"

// 发布方法类型枚举
typedef enum {
    PUBLISH_METHOD_HTTP = 0,
    PUBLISH_METHOD_MQTT,
    PUBLISH_METHOD_OTEL,
    PUBLISH_METHOD_UNKNOWN
} PublishMethodType;

// 通用发布接口
typedef struct {
    PublishMethodType type;
    char *config_json;      // 发布配置 JSON
    void *client_handle;    // 具体客户端句柄
} Publisher;

// 发布接口函数
Publisher *publisher_new(PublishMethodType type, const char *config_json);
void publisher_free(Publisher *publisher);
int publisher_publish_data(Publisher *publisher, const DataModel *data);

// 辅助函数
PublishMethodType publisher_get_type_from_string(const char *method_name);
const char *publisher_get_type_string(PublishMethodType type);

#endif // PUBLISHER_H