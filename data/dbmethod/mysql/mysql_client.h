#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include "common/datamodel.h"
#include <mysql.h>

typedef struct {
    char *addr;
    char *database;
    char *userName;
    char *password; // 从环境变量读取
} MySQLClientConfig;

typedef struct {
    MySQLClientConfig config;
    MYSQL *conn;
} DataBaseConfig;

int mysql_parse_client_config(const char *json, MySQLClientConfig *out);
int mysql_init_client(DataBaseConfig *db);
void mysql_close_client(DataBaseConfig *db);
int mysql_add_data(DataBaseConfig *db, const DataModel *data);

#endif