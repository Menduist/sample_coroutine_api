#ifndef SAMPLE_MAIN_H
#define SAMPLE_MAIN_H

#include "config.h"
#include <stdint.h>
#include <re/re.h>

struct main {
        struct config config;
        struct api *api;
        struct db *db;
        struct http_cli *http_client;
};

char *get_database_echo(struct main *m, char *str);
char *get_url_value(struct main *main, char *url);

#endif
