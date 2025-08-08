#include "mysql_client.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 设置环境变量 PASSWORD，实际部署时可用 export PASSWORD=xxx
    setenv("PASSWORD", "yourpassword", 1);

    // 构造配置
    MySQLClientConfig config = {
        .addr = "127.0.0.1",
        .database = "testdb",
        .userName = "root",
        .password = getenv("PASSWORD")
    };
    MySQLDataBaseConfig db = { .config = config, .conn = NULL };

    // 初始化连接
    if (mysql_init_client(&db) != 0) {
        printf("MySQL connect failed\n");
        return 1;
    }
    printf("MySQL connect success\n");

    // 构造一个简单的 DataModel 数据进行插入测试
    DataModel data = {
        .namespace_ = "default",
        .deviceName = "dev1",
        .propertyName = "prop1",
        .timeStamp = time(NULL),
        .value = "test_value"
    };

    if (mysql_add_data(&db, &data) == 0) {
        printf("Insert data success\n");
    } else {
        printf("Insert data failed\n");
    }

    mysql_close_client(&db);
        return 0;
}