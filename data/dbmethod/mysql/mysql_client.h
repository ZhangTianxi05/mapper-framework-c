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
} MySQLDataBaseConfig;  // 重命名：DataBaseConfig -> MySQLDataBaseConfig

typedef struct {
    MySQLDataBaseConfig dbConfig;  // 更新类型
    DataModel *dataModel;
    int reportCycleMs;
    CustomizedClient *customizedClient;
    VisitorConfig *visitorConfig;
    int running;
} MySQLDataHandlerArgs;

// MySQL 客户端函数
int mysql_parse_client_config(const char *json, MySQLClientConfig *out);
int mysql_init_client(MySQLDataBaseConfig *db);         // 更新参数类型
void mysql_close_client(MySQLDataBaseConfig *db);       // 更新参数类型
int mysql_add_data(MySQLDataBaseConfig *db, const DataModel *data);  // 更新参数类型

// MySQL 数据处理函数
int StartMySQLDataHandler(const char *clientConfigJson, DataModel *dataModel, CustomizedClient *customizedClient, VisitorConfig *visitorConfig, int reportCycleMs);
int StopMySQLDataHandler(MySQLDataHandlerArgs *args);
MySQLDataBaseConfig* NewMySQLDataBaseClient(const char *configJson);
void FreeMySQLDataBaseClient(MySQLDataBaseConfig *dbConfig);

#endif