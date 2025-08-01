#include "mysql_client.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

int mysql_parse_client_config(const char *json, MySQLClientConfig *out) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;
    cJSON *addr = cJSON_GetObjectItem(root, "addr");
    cJSON *database = cJSON_GetObjectItem(root, "database");
    cJSON *userName = cJSON_GetObjectItem(root, "userName");
    out->addr = addr ? strdup(addr->valuestring) : NULL;
    out->database = database ? strdup(database->valuestring) : NULL;
    out->userName = userName ? strdup(userName->valuestring) : NULL;
    char *pwd = getenv("PASSWORD");
    out->password = pwd ? strdup(pwd) : NULL;
    cJSON_Delete(root);
    return 0;
}

int mysql_init_client(DataBaseConfig *db) {
    db->conn = mysql_init(NULL);
    if (!db->conn) {
        log_error("mysql_init failed");
        return -1;
    }
    if (!mysql_real_connect(db->conn, db->config.addr, db->config.userName, db->config.password, db->config.database, 3306, NULL, 0)) {
        log_error("mysql_real_connect failed: %s", mysql_error(db->conn));
        mysql_close(db->conn);
        db->conn = NULL;
        return -1;
    }
    return 0;
}

void mysql_close_client(DataBaseConfig *db) {
    if (db->conn) {
        mysql_close(db->conn);
        db->conn = NULL;
    }
}

int mysql_add_data(DataBaseConfig *db, const DataModel *data) {
    if (!db || !db->conn || !data) return -1;
    char tableName[256];
    snprintf(tableName, sizeof(tableName), "%s/%s/%s", data->namespace_, data->deviceName, data->propertyName);

    char createTable[512];
    snprintf(createTable, sizeof(createTable),
        "CREATE TABLE IF NOT EXISTS `%s` (id INT AUTO_INCREMENT PRIMARY KEY, ts DATETIME NOT NULL, field TEXT)", tableName);
    if (mysql_query(db->conn, createTable)) {
        log_error("create table failed: %s", mysql_error(db->conn));
        return -1;
    }

    char insertSql[512];
    snprintf(insertSql, sizeof(insertSql), "INSERT INTO `%s` (ts,field) VALUES (?,?)", tableName);

    MYSQL_STMT *stmt = mysql_stmt_init(db->conn);
    if (!stmt) {
        log_error("mysql_stmt_init failed: %s", mysql_error(db->conn));
        return -1;
    }
    if (mysql_stmt_prepare(stmt, insertSql, strlen(insertSql))) {
        log_error("mysql_stmt_prepare failed: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    // 时间戳转字符串
    char datetime[32];
    time_t ts = data->timeStamp;
    struct tm *tm_info = localtime(&ts);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = datetime;
    bind[0].buffer_length = strlen(datetime);

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char*)data->value;
    bind[1].buffer_length = data->value ? strlen(data->value) : 0;

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