#ifndef TDENGINE_CLIENT_H
#define TDENGINE_CLIENT_H

#include "common/datamodel.h"
#include <taos.h>

typedef struct {
    char *addr;
    char *dbName;
    char *user;
    char *password;
} TDEngineClientConfig;

typedef struct {
    TDEngineClientConfig config;
    TAOS *conn;
} DataBaseConfig;

int tdengine_parse_client_config(const char *json, TDEngineClientConfig *out);
int tdengine_init_client(DataBaseConfig *db);
void tdengine_close_client(DataBaseConfig *db);
int tdengine_add_data(DataBaseConfig *db, const DataModel *data);

#endif