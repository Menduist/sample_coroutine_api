#ifndef SAMPLE_CONFIG_H
#define SAMPLE_CONFIG_H

struct config_db {
        char *host;
        char *port;
        char *user;
        char *passwd;
        char *database;

        int pool_size;
};

struct config_api {
        int port;
};

struct config {
        int argc;
        char **argv;
        char *config_filename;

        struct config_db db;
        struct config_api *api;
};

void dump_config_db(struct config_db *);
void dump_config_api(struct config_api *);
void dump_config(struct config *);

int config_init(struct config *target, int ac, char **av);
int config_read_yaml(struct config *target);

#endif

