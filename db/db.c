#include "db.h"
#include "coro.h"
#include "re_pgsql.h"
#include <string.h>
#include <stdlib.h>


struct db {
        struct re_pgsql pgsql;
};

/*
** STATEMENTS
*/

enum stmts {
        /* 0 */ STMT_ECHO,
};

const char *prepared_stmt[] = {
        /* 0 */ "SELECT $1",
	NULL
};

int nb_param_pg[] = {
        /* 0 */  1,
};

/*
** UTILS
*/
static PGresult *db_execute(struct re_pgsql *pgsql, int index_stmt) {
        PGresult *res;
        cre_current->target = &res;
        re_pgsql_execute_stmt(pgsql, index_stmt, NULL, (void (*)(PGresult *, void *))store_result_and_switch, (void *)cre_current);
        switch_to_main();
        return res;
}

static struct re_pgsql_bind *db_prepare(struct re_pgsql *pgsql, int index_stmt) {
        struct re_pgsql_bind *res;
        cre_current->target = &res;
        re_pgsql_execute_stmt(pgsql, index_stmt, (void (*)(struct re_pgsql_bind *, void *))store_result_and_switch, (void (*)(PGresult *, void *))store_result_and_switch, (void *)cre_current);
        switch_to_main();
        return res;
}

static PGresult *db_finish(void) {
        PGresult *res;
        cre_current->target = &res;
        switch_to_main();
        return res;
}

/*
** STATEMENTS CODE
*/
char *db_get_echo(struct db *db, char *echo) {
        struct re_pgsql_bind *bind = db_prepare(&db->pgsql, STMT_ECHO);
        re_pgsql_bind_string(bind, echo);
        PGresult *res = db_finish();
        if (PQntuples(res) > 0) {
                if (PQgetisnull(res, 0, 0)) return 0;
                return strdup(PQgetvalue(res, 0, 0));
        }
        return 0;
}


/*
** INIT
*/
struct db *db_init(struct main *main, struct config_db *conf) {
        struct db *result = calloc(1, sizeof(struct db));

        re_pgsql_init(&result->pgsql, prepared_stmt, nb_param_pg, conf->pool_size);
        re_pgsql_connect(&result->pgsql, conf->host, conf->user, conf->passwd, conf->port, conf->database);
        return (result);
}

