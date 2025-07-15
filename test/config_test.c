#include <stdio.h>
#include <stdlib.h>
#include "config/config.h"

int main() {
    Config *cfg = config_parse("config/config.yaml");
    if (!cfg) {
        printf("Failed to parse config\n");
        return 1;
    }

    printf("GRPC socket_path: %s\n", cfg->grpc_server.socket_path);
    printf("Common name: %s\n", cfg->common.name);
    printf("Common version: %s\n", cfg->common.version);
    printf("Common api_version: %s\n", cfg->common.api_version);
    printf("Common protocol: %s\n", cfg->common.protocol);
    printf("Common address: %s\n", cfg->common.address);
    printf("Common edgecore_sock: %s\n", cfg->common.edgecore_sock);
    printf("Common http_port: %s\n", cfg->common.http_port);

    config_free(cfg);
    return 0;
}