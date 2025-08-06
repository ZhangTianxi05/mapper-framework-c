#ifndef OTEL_PUBLISHER_H
#define OTEL_PUBLISHER_H

#include "common/datamodel.h"
#include <curl/curl.h>

// OpenTelemetry 发布配置
typedef struct {
    char *endpoint;      // OTLP 端点
    char *service_name;  // 服务名称
    char *service_version; // 服务版本
    int timeout_ms;      // 超时时间
} OtelPublishConfig;

// OpenTelemetry 发布器
typedef struct {
    OtelPublishConfig config;
    CURL *curl;
    struct curl_slist *headers;
} OtelPublisher;

// 函数声明
int otel_parse_config(const char *json, OtelPublishConfig *config);
void otel_free_config(OtelPublishConfig *config);

OtelPublisher *otel_publisher_new(const char *config_json);
void otel_publisher_free(OtelPublisher *publisher);
int otel_publisher_publish(OtelPublisher *publisher, const DataModel *data);

#endif // OTEL_PUBLISHER_H