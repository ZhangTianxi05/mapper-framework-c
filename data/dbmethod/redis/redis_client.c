#include "redis_client.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

int redis_parse_client_config(const char *json, RedisClientConfig *out) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;
    cJSON *addr = cJSON_GetObjectItem(root, "addr");
    cJSON *db = cJSON_GetObjectItem(root, "db");
    cJSON *poolSize = cJSON_GetObjectItem(root, "poolSize");
    cJSON *minIdleConns = cJSON_GetObjectItem(root, "minIdleConns");
    out->addr = addr ? strdup(addr->valuestring) : NULL;
    out->db = db ? db->valueint : 0;
    out->poolSize = poolSize ? poolSize->valueint : 0;
    out->minIdleConns = minIdleConns ? minIdleConns->valueint : 0;
    char *pwd = getenv("PASSWORD");
    out->password = pwd ? strdup(pwd) : NULL;
    cJSON_Delete(root);
    return 0;
}

int redis_init_client(DataBaseConfig *db) {
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    db->conn = redisConnectWithTimeout(db->config.addr, 6379, timeout);
    if (db->conn == NULL || db->conn->err) {
        log_error("redisConnect failed: %s", db->conn ? db->conn->errstr : "NULL");
        return -1;
    }
    if (db->config.password) {
        redisReply *reply = redisCommand(db->conn, "AUTH %s", db->config.password);
        if (!reply || db->conn->err) {
            log_error("redis AUTH failed: %s", db->conn->errstr);
            if (reply) freeReplyObject(reply);
            return -1;
        }
        freeReplyObject(reply);
    }
    if (db->config.db > 0) {
        redisReply *reply = redisCommand(db->conn, "SELECT %d", db->config.db);
        if (!reply || db->conn->err) {
            log_error("redis SELECT failed: %s", db->conn->errstr);
            if (reply) freeReplyObject(reply);
            return -1;
        }
        freeReplyObject(reply);
    }
    return 0;
}

void redis_close_client(DataBaseConfig *db) {
    if (db->conn) {
        redisFree(db->conn);
        db->conn = NULL;
    }
}

int redis_add_data(DataBaseConfig *db, const DataModel *data) {
    if (!db || !db->conn || !data) return -1;
    char key[256];
    snprintf(key, sizeof(key), "%s/%s", data->namespace_, data->deviceName);

    char member[512];
    snprintf(member, sizeof(member), "TimeStamp: %lld PropertyName: %s data: %s",
             (long long)data->timeStamp, data->propertyName, data->value ? data->value : "");

    redisReply *reply = redisCommand(db->conn, "ZADD %s %lld \"%s\"",
                                     data->deviceName, (long long)data->timeStamp, member);
    if (!reply || db->conn->err) {
        log_error("redis ZADD failed: %s", db->conn->errstr);
        if (reply) freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    return 0;
}