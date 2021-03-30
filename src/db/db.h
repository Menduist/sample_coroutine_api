#ifndef DB_H
#define DB_H

#include "../config.h"
#include <stdbool.h>

struct main;
struct db;

struct db *db_init(struct main *main, struct config_db *conf);

char *db_get_echo(struct db *db, char *number);

#endif
