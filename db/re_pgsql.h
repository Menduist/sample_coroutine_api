#ifndef RE_PGSQL_H
# define RE_PGSQL_H

#include <stdint.h>
#include <re/re.h>
#include <libpq-fe.h>

enum re_pgsql_state {
	RE_PGSQL_NOT_CONNECTED,
	RE_PGSQL_TRY_CONNECT,
	RE_PGSQL_TRY_RECONNECT,
	RE_PGSQL_PREPARE_START,
	RE_PGSQL_PREPARE_CONT,
	RE_PGSQL_CONNECTED,
	RE_PGSQL_RECV,
	RE_PGSQL_WAITING
};

struct re_pgsql_bind {
	char **values;
	int *length;
	int *format;
	int current;
};

struct re_pgsql_queue {
	struct re_pgsql_queue *next;
	int index;
	void *arg;
	void (*bind_handler)(struct re_pgsql_bind *, void *);
	void (*result_handler)(PGresult *, void *);
};

struct re_pgsql_prepared {
	char **keys;
	char **query;
	int *nb_params;
	int current;
	int max;
	int prep_index;
};

struct re_pgsql_connect {
	struct re_pgsql_queue *queue;
	struct re_pgsql_queue *end;
	int queue_length;
	PGconn *conn;
	enum re_pgsql_state state;
	struct re_pgsql_prepared prepared;
	char *host;
	char *user;
	char *password;
	char *port;
	char *dbname;
	struct tmr tmr;
	uint64_t delay;
};

struct re_pgsql {
	struct re_pgsql_connect *pool;
	int pool_length;
};

int re_pgsql_init(struct re_pgsql *pgsql, const char **prepared, int *nb_params, int len);
int re_pgsql_connect(struct re_pgsql *pgsql, char *host, char *user, char *password, char *port, char *dbname);
void re_pgsql_execute_stmt(struct re_pgsql *pgsql, int index_stmt, void (*bind_handler)(struct re_pgsql_bind *, void *), void (*result_handler)(PGresult *, void *), void *arg);

void re_pgsql_bind_null(struct re_pgsql_bind *bind);
void re_pgsql_bind_string(struct re_pgsql_bind *bind, char *value);
void re_pgsql_bind_int(struct re_pgsql_bind *bind, int value);
void re_pgsql_bind_bigint(struct re_pgsql_bind *bind, long long value);
void re_pgsql_bind_blob(struct re_pgsql_bind *bind, void *value, int length);

#endif
