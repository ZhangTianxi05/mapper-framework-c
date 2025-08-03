#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include "common/datamodel.h"
#include "driver/driver.h"
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

typedef struct {
    DataBaseConfig dbConfig;
    DataModel *dataModel;
    int reportCycleMs;
    CustomizedClient *customizedClient;
    VisitorConfig *visitorConfig;
    int running;
} MySQLDataHandlerArgs;

// MySQL 客户端函数
int mysql_parse_client_config(const char *json, MySQLClientConfig *out);
int mysql_init_client(DataBaseConfig *db);
void mysql_close_client(DataBaseConfig *db);
int mysql_add_data(DataBaseConfig *db, const DataModel *data);

// MySQL 数据处理函数
int StartMySQLDataHandler(const char *clientConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs);
int StopMySQLDataHandler(MySQLDataHandlerArgs *args);
DataBaseConfig* NewMySQLDataBaseClient(const char *configJson);
void FreeMySQLDataBaseClient(DataBaseConfig *dbConfig);

#endif