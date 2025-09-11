#ifndef MYSQL_RECORDER_H
#define MYSQL_RECORDER_H

#include "data/dbmethod/mysql/mysql_client.h"

#ifdef __cplusplus
extern "C" {
#endif

// 注入全局 MySQL 连接（可为 NULL，NULL 时 record 无操作）
void mysql_recorder_set_db(MySQLDataBaseConfig *db);

// 记录一条时间序列数据；ts_ms 毫秒时间戳（内部转秒）
int mysql_recorder_record(const char *ns,
                          const char *deviceName,
                          const char *propertyName,
                          const char *value,
                          long long ts_ms);

#ifdef __cplusplus
}
#endif

#endif