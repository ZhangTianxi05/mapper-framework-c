#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include "common/datamodel.h"
#include <mosquitto.h>

// MQTT 发布配置
typedef struct {
    char *broker_url;    // MQTT 代理地址
    int port;            // 端口号
    char *client_id;     // 客户端 ID
    char *username;      // 用户名
    char *password;      // 密码
    char *topic_prefix;  // 主题前缀
    int qos;            // 服务质量等级
    int keep_alive;     // 保活时间
} MqttPublishConfig;

// MQTT 发布器
typedef struct {
    MqttPublishConfig config;
    struct mosquitto *mosq;
    int connected;
} MqttPublisher;

// 函数声明
int mqtt_parse_config(const char *json, MqttPublishConfig *config);
void mqtt_free_config(MqttPublishConfig *config);

MqttPublisher *mqtt_publisher_new(const char *config_json);
void mqtt_publisher_free(MqttPublisher *publisher);
int mqtt_publisher_publish(MqttPublisher *publisher, const DataModel *data);

#endif // MQTT_PUBLISHER_H