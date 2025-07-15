#include "config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

Config *config_parse(const char *filename)
{
    FILE *fh = fopen(filename, "r");
    if (!fh)
        return NULL;

    Config *cfg = (Config *)calloc(1, sizeof(Config));
    if (!cfg)
    {
        fclose(fh);
        return NULL;
    }

    yaml_parser_t parser;
    yaml_token_t token;
    char key[128] = {0};
    int in_grpc_server = 0, in_common = 0;

    if (!yaml_parser_initialize(&parser))
    {
        fclose(fh);
        free(cfg);
        return NULL;
    }
    yaml_parser_set_input_file(&parser, fh);

    while (1)
    {
        yaml_parser_scan(&parser, &token);
        if (token.type == YAML_STREAM_END_TOKEN)
            break;

        if (token.type == YAML_KEY_TOKEN)
        {
            yaml_token_delete(&token);
            yaml_parser_scan(&parser, &token);
            if (token.type == YAML_SCALAR_TOKEN)
            {
                if (strcmp((char *)token.data.scalar.value, "grpc_server") == 0)
                {
                    in_grpc_server = 1;
                    in_common = 0;
                    key[0] = '\0';
                }
                else if (strcmp((char *)token.data.scalar.value, "common") == 0)
                {
                    in_grpc_server = 0;
                    in_common = 1;
                    key[0] = '\0';
                }
                else
                {
                    strncpy(key, (char *)token.data.scalar.value, sizeof(key) - 1);
                    key[sizeof(key) - 1] = '\0';
                }
            }
        }
        else if (token.type == YAML_VALUE_TOKEN)
        {
            yaml_token_delete(&token);
            yaml_parser_scan(&parser, &token);
            if (token.type == YAML_SCALAR_TOKEN)
            {
                if (in_grpc_server)
                {
                    if (strcmp(key, "socket_path") == 0)
                        strncpy(cfg->grpc_server.socket_path, (char *)token.data.scalar.value, sizeof(cfg->grpc_server.socket_path) - 1);
                }
                else if (in_common)
                {
                    if (strcmp(key, "name") == 0)
                        strncpy(cfg->common.name, (char *)token.data.scalar.value, sizeof(cfg->common.name) - 1);
                    else if (strcmp(key, "version") == 0)
                        strncpy(cfg->common.version, (char *)token.data.scalar.value, sizeof(cfg->common.version) - 1);
                    else if (strcmp(key, "api_version") == 0)
                        strncpy(cfg->common.api_version, (char *)token.data.scalar.value, sizeof(cfg->common.api_version) - 1);
                    else if (strcmp(key, "protocol") == 0)
                        strncpy(cfg->common.protocol, (char *)token.data.scalar.value, sizeof(cfg->common.protocol) - 1);
                    else if (strcmp(key, "address") == 0)
                        strncpy(cfg->common.address, (char *)token.data.scalar.value, sizeof(cfg->common.address) - 1);
                    else if (strcmp(key, "edgecore_sock") == 0)
                        strncpy(cfg->common.edgecore_sock, (char *)token.data.scalar.value, sizeof(cfg->common.edgecore_sock) - 1);
                    else if (strcmp(key, "http_port") == 0)
                        strncpy(cfg->common.http_port, (char *)token.data.scalar.value, sizeof(cfg->common.http_port) - 1);
                }
            }
        }
        yaml_token_delete(&token);
    }

    yaml_parser_delete(&parser);
    fclose(fh);
    return cfg;
}

void config_free(Config *cfg)
{
    if (cfg)
        free(cfg);
}