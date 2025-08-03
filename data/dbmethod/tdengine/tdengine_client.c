#include "tdengine_client.h"
#include "log/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <taos.h>

int tdengine_parse_client_config(const char *json, TDEngineClientConfig *out) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;
    cJSON *addr = cJSON_GetObjectItem(root, "addr");
    cJSON *dbName = cJSON_GetObjectItem(root, "dbName");
    out->addr = addr ? strdup(addr->valuestring) : NULL;
    out->dbName = dbName ? strdup(dbName->valuestring) : NULL;
    char *user = getenv("USERNAME");
    char *pwd = getenv("PASSWORD");
    out->user = user ? strdup(user) : NULL;
    out->password = pwd ? strdup(pwd) : NULL;
    cJSON_Delete(root);
    return 0;
}

int tdengine_init_client(DataBaseConfig *db) {
    taos_init();
    db->conn = taos_connect(db->config.addr, db->config.user, db->config.password, db->config.dbName, 0);
    if (!db->conn) {
        log_error("TDengine connect failed: %s", taos_errstr(NULL));
        return -1;
    }
    log_info("TDengine connected successfully");
    return 0;
}

void tdengine_close_client(DataBaseConfig *db) {
    if (db->conn) {
        taos_close(db->conn);
        db->conn = NULL;
    }
    taos_cleanup();
}

int tdengine_add_data(DataBaseConfig *db, const DataModel *data) {
    if (!db || !db->conn || !data) return -1;
    char tableName[256];
    snprintf(tableName, sizeof(tableName), "%s_%s", data->namespace_, data->deviceName);
    char legalTable[256];
    for (int i = 0; tableName[i]; ++i)
        legalTable[i] = (tableName[i] == '-') ? '_' : tableName[i];
    legalTable[strlen(tableName)] = '\0';

    char legalTag[128];
    snprintf(legalTag, sizeof(legalTag), "%s", data->propertyName);
    for (int i = 0; legalTag[i]; ++i)
        if (legalTag[i] == '-') legalTag[i] = '_';

    char stableName[256];
    snprintf(stableName, sizeof(stableName), "SHOW STABLES LIKE '%s'", legalTable);

    char stabel[512];
    snprintf(stabel, sizeof(stabel),
        "CREATE STABLE IF NOT EXISTS %s (ts timestamp, deviceid binary(64), propertyname binary(64), data binary(64), type binary(64)) TAGS (location binary(64));",
        legalTable);

    char datatime[32];
    time_t ts = data->timeStamp;
    struct tm *tm_info = localtime(&ts);
    strftime(datatime, sizeof(datatime), "%Y-%m-%d %H:%M:%S", tm_info);

    char insertSQL[1024];
    snprintf(insertSQL, sizeof(insertSQL),
        "INSERT INTO %s USING %s TAGS ('%s') VALUES('%s','%s', '%s', '%s', '%s');",
        legalTag, legalTable, legalTag, datatime, tableName, data->propertyName, data->value ? data->value : "", data->type ? data->type : "");

    TAOS_RES *res = taos_query(db->conn, stableName);
    if (!res || taos_errno(res) != 0) {
        log_error("query stable failed: %s", taos_errstr(res));
        taos_free_result(res);
        return -1;
    }
    int exist = taos_num_rows(res) > 0;
    taos_free_result(res);

    if (!exist) {
        res = taos_query(db->conn, stabel);
        if (taos_errno(res) != 0) {
            log_error("create stable failed: %s", taos_errstr(res));
            taos_free_result(res);
            return -1;
        }
        taos_free_result(res);
    }

    res = taos_query(db->conn, insertSQL);
    if (taos_errno(res) != 0) {
        log_error("failed add data to TDengine: %s", taos_errstr(res));
        taos_free_result(res);
        return -1;
    }
    taos_free_result(res);
    return 0;
}