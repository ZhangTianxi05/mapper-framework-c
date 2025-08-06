#ifndef HTTP_PUBLISHER_H
#define HTTP_PUBLISHER_H

#include "common/datamodel.h"
#include <curl/curl.h>

// HTTP 发布配置
typedef struct {
    char *endpoint;      // HTTP 端点 URL
    char *method;        // HTTP 方法 (POST/PUT)
    char *auth_token;    // 认证令牌
    char *content_type;  // 内容类型
    int timeout_ms;      // 超时时间
    int retry_count;     // 重试次数
} HttpPublishConfig;

// HTTP 发布客户端
typedef struct {
    HttpPublishConfig config;
    CURL *curl;
    struct curl_slist *headers;
} HttpPublisher;

// 函数声明
int http_parse_config(const char *json, HttpPublishConfig *config);
void http_free_config(HttpPublishConfig *config);

HttpPublisher *http_publisher_new(const char *config_json);
void http_publisher_free(HttpPublisher *publisher);
int http_publisher_publish(HttpPublisher *publisher, const DataModel *data);

#endif // HTTP_PUBLISHER_H