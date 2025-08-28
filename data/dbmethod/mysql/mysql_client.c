#include "mysql_client.h"
#include "log/log.h"

#include <mysql.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <cjson/cJSON.h>

static const char* getenv_def(const char *k, const char *defv) {
    const char *v = getenv(k);
    return (v && *v) ? v : defv;
}

int mysql_parse_client_config(const char *json, MySQLClientConfig *out) {
    if (!out) return -1;

    // 先用环境变量初始化默认值
    const char *env_addr = getenv_def("MYSQL_HOST", "127.0.0.1");
    const char *env_db   = getenv_def("MYSQL_DB",   "testdb");
    const char *env_user = getenv_def("MYSQL_USER", "mapper");
    const char *env_pwd  = getenv("MYSQL_PASSWORD");
    if (!env_pwd || !*env_pwd) env_pwd = getenv("PASSWORD"); // 兼容 PASSWORD

    out->addr     = strdup(env_addr);
    out->database = strdup(env_db);
    out->userName = strdup(env_user);
    out->password = env_pwd ? strdup(env_pwd) : NULL;

    // 如提供了 JSON，则覆盖对应字段
    if (json && *json) {
        cJSON *root = cJSON_Parse(json);
        if (root) {
            cJSON *addr = cJSON_GetObjectItem(root, "addr");
            cJSON *database = cJSON_GetObjectItem(root, "database");
            cJSON *userName = cJSON_GetObjectItem(root, "userName");
            cJSON *password = cJSON_GetObjectItem(root, "password");

            if (addr && cJSON_IsString(addr) && addr->valuestring) {
                free(out->addr); out->addr = strdup(addr->valuestring);
            }
            if (database && cJSON_IsString(database) && database->valuestring) {
                free(out->database); out->database = strdup(database->valuestring);
            }
            if (userName && cJSON_IsString(userName) && userName->valuestring) {
                free(out->userName); out->userName = strdup(userName->valuestring);
            }
            if (password && cJSON_IsString(password) && password->valuestring) {
                if (out->password) free(out->password);
                out->password = strdup(password->valuestring);
            }
            cJSON_Delete(root);
        }
    }
    return 0;
}

// ...existing code...
int mysql_init_client(MySQLDataBaseConfig *db) {
    if (!db) return -1;

    db->conn = mysql_init(NULL);
    if (!db->conn) {
        log_error("mysql_init failed");
        return -1;
    }

    // 最简化：只设置超时，其他都用默认
    unsigned int timeout = 10;
    mysql_options(db->conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    unsigned int port = 3306;
    const char *portEnv = getenv("MYSQL_PORT");
    if (portEnv && *portEnv) port = (unsigned int)atoi(portEnv);

    const char *host = db->config.addr ? db->config.addr : "127.0.0.1";
    const char *user = db->config.userName ? db->config.userName : "mapper";
    const char *pass = db->config.password ? db->config. password : "";
    const char *dbnm = db->config.database ? db->config.database : "testdb";

    // 完全不传 client_flag，让库自动协商
    if (!mysql_real_connect(db->conn, host, user, pass, dbnm, port, NULL, 0)) {
        log_error("mysql_real_connect failed: %s", mysql_error(db->conn));
        mysql_close(db->conn);
        db->conn = NULL;
        return -1;
    }

    return 0;
}
// ...existing code...
void mysql_close_client(MySQLDataBaseConfig *db) {
    if (db && db->conn) {
        mysql_close(db->conn);
        db->conn = NULL;
    }
}

int mysql_add_data(MySQLDataBaseConfig *db, const DataModel *data) {
    if (!db || !db->conn || !data) return -1;

    // 组合表名（包含 / 需要使用反引号）
    char tableName[256];
    snprintf(tableName, sizeof(tableName), "%s/%s/%s",
             data->namespace_ ? data->namespace_ : "default",
             data->deviceName ? data->deviceName : "device",
             data->propertyName ? data->propertyName : "property");

    // 建表
    char createTable[512];
    snprintf(createTable, sizeof(createTable),
        "CREATE TABLE IF NOT EXISTS `%s` ("
        "  id INT AUTO_INCREMENT PRIMARY KEY,"
        "  ts DATETIME NOT NULL,"
        "  field TEXT"
        ")", tableName);

    if (mysql_query(db->conn, createTable)) {
        log_error("create table failed: %s", mysql_error(db->conn));
        return -1;
    }

    // 插入语句（预处理）
    char insertSql[512];
    snprintf(insertSql, sizeof(insertSql),
             "INSERT INTO `%s` (ts, field) VALUES (?, ?)", tableName);

    MYSQL_STMT *stmt = mysql_stmt_init(db->conn);
    if (!stmt) {
        log_error("mysql_stmt_init failed: %s", mysql_error(db->conn));
        return -1;
    }
    if (mysql_stmt_prepare(stmt, insertSql, (unsigned long)strlen(insertSql))) {
        log_error("mysql_stmt_prepare failed: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return -1;
    }

    // 准备绑定参数
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    // 时间戳 -> 日期时间字符串
    char datetime[32];
    time_t ts = data->timeStamp;
    struct tm tm_info;
    localtime_r(&ts, &tm_info);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &tm_info);

    unsigned long lengths[2];
    lengths[0] = (unsigned long)strlen(datetime);
    lengths[1] = (unsigned long)(data->value ? strlen(data->value) : 0);

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = (void*)datetime;
    bind[0].buffer_length = lengths[0];
    bind[0].length        = &lengths[0];

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = (void*)(data->value ? data->value : "");
    bind[1].buffer_length = lengths[1];
    bind[1].length        = &lengths[1];

    if (mysql_stmt_bind_param(stmt, bind)) {
        log_error("mysql_stmt_bind_param failed: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return -1;
    }

    if (mysql_stmt_execute(stmt)) {
        log_error("mysql_stmt_execute failed: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return -1;
    }

    mysql_stmt_close(stmt);
    return 0;
}