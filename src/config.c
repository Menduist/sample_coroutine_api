#include "config.h"
#include "myaml.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_config_api(struct config_api *api) {
        printf("API on port %d\n", api->port);
}

void dump_config_db(struct config_db *db) {
        printf("BDD (pool size %d): %s:%s@%s:%s/%s\n", db->pool_size, db->user, db->passwd, db->host,
                db->port, db->database);
}

void dump_config(struct config *conf) {
        printf("Config:\n");
        dump_config_db(&conf->db);
        if (conf->api) dump_config_api(conf->api);
}

int config_init(struct config *target, int ac, char **av) {
        memset(target, 0, sizeof(struct config));
        target->argc = ac;
        target->argv = av;

        target->config_filename = strdup("conf.yaml");
        int opt;
        while ((opt = getopt(ac, av, "c:")) != -1) {
                switch (opt) {
                case 'c':
                        free(target->config_filename);
                        target->config_filename = strdup(optarg);
                default:
                        printf("Usage: %s [-c config file]\n", av[0]);
                        return -1;
                }

        }
        return 0;
}

int config_read_yaml(struct config *target) {
        FILE *conf = fopen(target->config_filename, "r");
        struct myaml_t *root = myaml_loadf(conf);

        struct myaml_t *db = 0;
        struct myaml_t *api = 0;
        myaml_unpack(root, "{sr,s?r}",
                                "db", &db,
                                "api", &api
                        );

        target->db.host = strdup("127.0.0.1");
        target->db.port = strdup("5432");
	target->db.pool_size = 8;
        if (db) {
                myaml_unpack(db, "{s?s,s?s,ss,s?s,ss,s?i}",
                        "host", &target->db.host, "port", &target->db.port,
                        "user", &target->db.user, "passwd", &target->db.passwd,
                        "database", &target->db.database, "pool_size", &target->db.pool_size
                        );
        } else printf("missing db config\n");

        if (api) {
                target->api = calloc(1, sizeof(struct config_api));
                target->api->port = 8080;
                myaml_unpack(api, "{s?i}", "port", &target->api->port);
        }
        dump_config(target);
        myaml_free(root);
        return 0;
}
