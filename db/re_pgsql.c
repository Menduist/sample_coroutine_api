#include "re_pgsql.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define REPG_DEBUG 1
#ifdef REPG_DEBUG
#include "coro.h"
#endif

static void re_pgsql_handler(int flags, void *arg);
static void re_pgsql_reconnect(void *arg);

void re_pgsql_bind_string(struct re_pgsql_bind *bind, char *value) {
	bind->values[bind->current] = value ? strdup(value) : NULL;
	bind->length[bind->current] = value ? strlen(value) : 0;
	bind->format[bind->current] = 0;
	bind->current += 1;
}

void re_pgsql_bind_int(struct re_pgsql_bind *bind, int value) {
	char buf[32];

	snprintf(buf, sizeof(buf), "%i", value);
	bind->values[bind->current] = strdup(buf);
	bind->length[bind->current] = strlen(buf);
	bind->format[bind->current] = 0;
	bind->current += 1;
}

void re_pgsql_bind_bigint(struct re_pgsql_bind *bind, long long value) {
	char buf[32];

	snprintf(buf, sizeof(buf), "%lli", value);
	bind->values[bind->current] = strdup(buf);
	bind->length[bind->current] = strlen(buf);
	bind->format[bind->current] = 0;
	bind->current += 1;
}

void re_pgsql_bind_blob(struct re_pgsql_bind *bind, void *value, int length) {
	bind->values[bind->current] = malloc(length);
	memcpy(bind->values[bind->current], value, length);
	bind->length[bind->current] = length;
	bind->format[bind->current] = 1;
	bind->current += 1;
}

void re_pgsql_bind_null(struct re_pgsql_bind *bind) {
	bind->values[bind->current] = NULL;
	bind->length[bind->current] = 0;
	bind->format[bind->current] = 0;
	bind->current += 1;
}

static char *enum_to_str(enum re_pgsql_state state) {
	if (state == RE_PGSQL_NOT_CONNECTED) {
		return ("RE_PGSQL_NOT_CONNECTED");
	} else if (state == RE_PGSQL_TRY_CONNECT) {
		return ("RE_PGSQL_TRY_CONNECT");
	} else if (state == RE_PGSQL_TRY_RECONNECT) {
		return ("RE_PGSQL_TRY_RECONNECT");
	} else if (state == RE_PGSQL_PREPARE_START) {
		return ("RE_PGSQL_PREPARE_START");
	} else if (state == RE_PGSQL_PREPARE_CONT) {
		return ("RE_PGSQL_PREPARE_CONT");
	} else if (state == RE_PGSQL_CONNECTED) {
		return ("RE_PGSQL_CONNECTED");
	} else if (state == RE_PGSQL_RECV) {
		return ("RE_PGSQL_RECV");
	} else if (state == RE_PGSQL_WAITING) {
		return ("RE_PGSQL_WAITING");
	}
}

static void re_pgsql_error_try_reconnect(struct re_pgsql_connect *rpgc) {
	char *error;

	error = PQerrorMessage(rpgc->conn);
	if (!error) {
		dprintf(2, "\033[31;1m... Error? %i\033[0m\n", PQsocket(rpgc->conn));
	} else {
		dprintf(2, "\033[31;1mError %i: %s\033[0m\n", PQsocket(rpgc->conn), error);
	}
	rpgc->state = RE_PGSQL_NOT_CONNECTED;
	if (rpgc->delay == 0) {
		rpgc->delay = 15000;
	} else if (rpgc->delay < 60000) {
		rpgc->delay *= 2;
	} else {
		rpgc->delay = 60000;
	}
	tmr_init(&rpgc->tmr);
	tmr_start(&rpgc->tmr, rpgc->delay, re_pgsql_reconnect, rpgc);
}

static void re_pgsql_prepared_init(struct re_pgsql_prepared *rpgp) {
	rpgp->keys = malloc(sizeof(char *) * 4);
	rpgp->query = malloc(sizeof(char *) * 4);
	rpgp->nb_params = malloc(sizeof(int) * 4);
	rpgp->current = 0;
	rpgp->max = 4;
}

static void re_pgsql_prepared_add(struct re_pgsql_prepared *rpgp, const char *query, int nb_params) {
	char buf[16];

	snprintf(buf, sizeof(buf), "%.4s %i", query, rpgp->current);
	rpgp->keys[rpgp->current] = strdup(buf);
	rpgp->query[rpgp->current] = strdup(query);
	rpgp->nb_params[rpgp->current] = nb_params;
	rpgp->current += 1;
	if (rpgp->current + 1 == rpgp->max) {
		rpgp->max *= 2;
		rpgp->keys = realloc(rpgp->keys, sizeof(char *) * rpgp->max);
		rpgp->query = realloc(rpgp->query, sizeof(char *) * rpgp->max);
		rpgp->nb_params = realloc(rpgp->nb_params, sizeof(int) * rpgp->max);
	}
}

int re_pgsql_init(struct re_pgsql *pgsql, const char **prepared, int *nb_params, int len) {
	int i;
	int j;

	memset(pgsql, 0, sizeof(struct re_pgsql));
	if (len <= 0) {
		return (0);
	}
	if ((pgsql->pool = malloc(sizeof(struct re_pgsql_connect) * len)) == NULL) {
		return (0);
	}
	pgsql->pool_length = len;
	for (i = 0; i < len; i += 1) {
		pgsql->pool[i].queue = NULL;
		pgsql->pool[i].end = NULL;
		pgsql->pool[i].queue_length = 0;
		pgsql->pool[i].state = RE_PGSQL_NOT_CONNECTED;
		re_pgsql_prepared_init(&pgsql->pool[i].prepared);
		if (prepared != NULL) {
			for (j = 0; prepared[j]; j += 1) {
				re_pgsql_prepared_add(&pgsql->pool[i].prepared, prepared[j], nb_params[j]);
			}
		}
	}
	return (1);
}

static void re_pgsql_reconnect(void *arg) {
	PostgresPollingStatusType type;
	struct re_pgsql_connect *rpgc;

	rpgc = arg;
	rpgc->state = RE_PGSQL_TRY_RECONNECT;
	PQresetStart(rpgc->conn);
	type = PQresetPoll(rpgc->conn);
	if (type == PGRES_POLLING_READING) {
		fd_listen(PQsocket(rpgc->conn), FD_READ, re_pgsql_handler, rpgc);
	} else if (type == PGRES_POLLING_WRITING) {
		fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
	} else if (type == PGRES_POLLING_FAILED) {
		re_pgsql_error_try_reconnect(rpgc);
	}
}

static void re_pgsql_handler_connect(struct re_pgsql_connect *rpgc, int flags) {
	PostgresPollingStatusType type;

	if (rpgc->state == RE_PGSQL_TRY_CONNECT) {
		type = PQconnectPoll(rpgc->conn);
	} else if (rpgc->state == RE_PGSQL_TRY_RECONNECT) {
		type = PQresetPoll(rpgc->conn);
	}
	if (type == PGRES_POLLING_READING) {
		fd_listen(PQsocket(rpgc->conn), FD_READ, re_pgsql_handler, rpgc);
	} else if (type == PGRES_POLLING_WRITING) {
		fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
	} else if (type == PGRES_POLLING_OK) {
		rpgc->delay = 0;
		rpgc->prepared.prep_index = 0;
		rpgc->state = RE_PGSQL_PREPARE_START;
		tmr_init(&rpgc->tmr);
		tmr_start(&rpgc->tmr, 60 * 60 * 1000, re_pgsql_reconnect, rpgc);
		fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
	} else if (type == PGRES_POLLING_FAILED) {
		re_pgsql_error_try_reconnect(rpgc);
	}
}

static void re_pgsql_handler_prepare(struct re_pgsql_connect *rpgc, int flags) {
	PGresult *res;

	if (rpgc->prepared.prep_index >= rpgc->prepared.current) {
		if (rpgc->queue) {
			rpgc->state = RE_PGSQL_CONNECTED;
			fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
		} else {
			rpgc->state = RE_PGSQL_WAITING;
		}
	} else if (rpgc->state == RE_PGSQL_PREPARE_START) {
		rpgc->state = RE_PGSQL_PREPARE_CONT;
		PQsendPrepare(rpgc->conn, rpgc->prepared.keys[rpgc->prepared.prep_index], rpgc->prepared.query[rpgc->prepared.prep_index], 1, NULL);
		fd_listen(PQsocket(rpgc->conn), FD_READ, re_pgsql_handler, rpgc);
	} else if (rpgc->state == RE_PGSQL_PREPARE_CONT) {
		res = PQgetResult(rpgc->conn);
		if (res == NULL) {
			rpgc->prepared.prep_index += 1;
			rpgc->state = RE_PGSQL_PREPARE_START;
			fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
		} else if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
			dprintf(2, "\033[31;1mError %i: %s\033[m\n", PQsocket(rpgc->conn), PQresultErrorMessage(res));
			fd_listen(PQsocket(rpgc->conn), FD_READ, re_pgsql_handler, rpgc);
			PQclear(res);
		} else if (PQresultStatus(res) == PGRES_COMMAND_OK) {
			printf("\033[32;1m => %s prepared!\033[m\n", rpgc->prepared.query[rpgc->prepared.prep_index]);
			fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
			PQclear(res);
		}
	}
}

static void re_pgsql_handler_exec_prepared_request(struct re_pgsql_connect *rpgc, int flags) {
	struct re_pgsql_bind bind;
	int nb_params;

	//printf("Execute: %s\n", rpgc->prepared.query[rpgc->queue->index]);
	nb_params = rpgc->prepared.nb_params[rpgc->queue->index];
	if (nb_params > 0) {
		bind.values = calloc(nb_params, sizeof(char *));
		bind.length = calloc(nb_params, sizeof(int));
		bind.format = calloc(nb_params, sizeof(int));
		bind.current = 0;
		rpgc->queue->bind_handler(&bind, rpgc->queue->arg);
#ifdef REPG_DEBUG
		struct creroutine *cre = rpgc->queue->arg;
		int i = 0;
		printf("[%d] Executing ", cre->id);
		while (i < nb_params) {
			printf("{$%d: %.*s} ", i + 1, bind.length[i], bind.values[i]);
			i++;
		}
		printf(": %s\n", rpgc->prepared.query[rpgc->queue->index]);
#endif
		PQsendQueryPrepared(rpgc->conn, rpgc->prepared.keys[rpgc->queue->index], nb_params, (const char **)bind.values, bind.length, bind.format, 0);
		while (bind.current) {
			bind.current -= 1;
			if (bind.values[bind.current]) {
				free(bind.values[bind.current]);
			}
		}
		free(bind.values);
		free(bind.length);
		free(bind.format);
	} else {
#ifdef REPG_DEBUG
		struct creroutine *cre = rpgc->queue->arg;
		int i = 0;
		printf("[%d] Executing %s\n", cre->id, rpgc->prepared.query[rpgc->queue->index]);
#endif
		PQsendQueryPrepared(rpgc->conn, rpgc->prepared.keys[rpgc->queue->index], 0, NULL, NULL, NULL, 0);
	}
	rpgc->state = RE_PGSQL_RECV;
	fd_listen(PQsocket(rpgc->conn), FD_READ | FD_EXCEPT, re_pgsql_handler, rpgc);
}

static void re_pgsql_pop_queue(struct re_pgsql_connect *rpgc) {
	struct re_pgsql_queue *queue;

	queue = rpgc->queue;
	rpgc->queue = rpgc->queue->next;
	rpgc->queue_length -= 1;
	free(queue);
}

static void re_pgsql_handler_recv_prepared_request(struct re_pgsql_connect *rpgc, int flags) {
	PGresult *res;
	struct re_pgsql_bind bind;
	int nb_params;
	ExecStatusType type;

	PQconsumeInput(rpgc->conn);
	while (PQisBusy(rpgc->conn) == 0) {
		res = PQgetResult(rpgc->conn);
		if (!res) {
			re_pgsql_pop_queue(rpgc);
			if (rpgc->queue) {
				rpgc->state = RE_PGSQL_CONNECTED;
				fd_listen(PQsocket(rpgc->conn), FD_WRITE, re_pgsql_handler, rpgc);
			} else {
				rpgc->state = RE_PGSQL_WAITING;
			}
			return ;
		}
		type = PQresultStatus(res);
		//printf("%i Result status: %s\n", PQsocket(rpgc->conn), PQresStatus(type));
		if (rpgc->queue->result_handler && (type == PGRES_COMMAND_OK || type == PGRES_TUPLES_OK || type == PGRES_COPY_IN)) {
#ifdef REPG_DEBUG
			struct creroutine *cre = rpgc->queue->arg;
			printf("[%d] Res: [", cre->id);
			int i = 0;
			while (i < PQntuples(res)) {
				printf("[");
				int y = 0;
				while (y < PQnfields(res)) {
					if (PQgetisnull(res, i, y)) {
						printf("null");
					} else {
						printf("\"%s\"", PQgetvalue(res, i, y));
					}
					y++;
					if (y != PQnfields(res)) {
						printf(",");
					}
				}
				i++;
				printf("]");
				if (i != PQntuples(res)) printf(",");
			}
			printf("] %s\n", PQcmdTuples(res));
#endif
			rpgc->queue->result_handler(res, rpgc->queue->arg);
		} else if (type == PGRES_NONFATAL_ERROR) {
			printf("\033[33;1m%s\033[0m\n", PQerrorMessage(rpgc->conn));
		} else if (type == PGRES_FATAL_ERROR) {
			printf("\033[31;1m%s\033[0m\n", PQerrorMessage(rpgc->conn));
		//	re_pgsql_reconnect(rpgc);
		//	PQclear(res);
		//	return ;
		}
		if (type == PGRES_COPY_IN) {
			return ;
		}
		PQclear(res);
	}
	fd_listen(PQsocket(rpgc->conn), FD_READ, re_pgsql_handler, rpgc);
}

static void re_pgsql_handler(int flags, void *arg) {
	struct re_pgsql_connect *rpgc;

	rpgc = arg;
	fd_close(PQsocket(rpgc->conn));
	//printf("%i => %s\n", PQsocket(rpgc->conn), enum_to_str(rpgc->state));
	switch (rpgc->state) {
		case RE_PGSQL_NOT_CONNECTED:
		case RE_PGSQL_WAITING:
			break ;
		case RE_PGSQL_TRY_CONNECT:
		case RE_PGSQL_TRY_RECONNECT:
			re_pgsql_handler_connect(rpgc, flags);
			break ;
		case RE_PGSQL_PREPARE_START:
		case RE_PGSQL_PREPARE_CONT:
			re_pgsql_handler_prepare(rpgc, flags);
			break ;
		case RE_PGSQL_CONNECTED:
			re_pgsql_handler_exec_prepared_request(rpgc, flags);
			break ;
		case RE_PGSQL_RECV:
			re_pgsql_handler_recv_prepared_request(rpgc, flags);
			break ;
		default:
			puts("Wuut ?");
			break ;
	}
}

int re_pgsql_connect(struct re_pgsql *pgsql, char *host, char *user, char *password, char *port, char *dbname) {
	PostgresPollingStatusType type;
	const char *keywords[] = { "host", "user", "password", "port", "dbname", NULL };
	const char *values[] = { host, user, password, port, dbname, NULL };
	int i;
	int j;

	for (i = 0; i < pgsql->pool_length; i += 1) {
		pgsql->pool[i].host = host ? strdup(host) : NULL;
		pgsql->pool[i].user = user ? strdup(user) : NULL;
		pgsql->pool[i].password = password ? strdup(password) : NULL;
		pgsql->pool[i].port = port ? strdup(port) : NULL;
		pgsql->pool[i].dbname = dbname ? strdup(dbname) : NULL;
		pgsql->pool[i].conn = PQconnectStartParams(keywords, values, 0);
		pgsql->pool[i].delay = 0;
		pgsql->pool[i].state = RE_PGSQL_TRY_CONNECT;
		type = PQconnectPoll(pgsql->pool[i].conn);
		if (type == PGRES_POLLING_READING) {
			fd_listen(PQsocket(pgsql->pool[i].conn), FD_READ, re_pgsql_handler, &pgsql->pool[i]);
		} else if (type == PGRES_POLLING_WRITING) {
			fd_listen(PQsocket(pgsql->pool[i].conn), FD_WRITE, re_pgsql_handler, &pgsql->pool[i]);
		} else if (type == PGRES_POLLING_FAILED) {
			re_pgsql_error_try_reconnect(&pgsql->pool[i]);
		}
	}
}

static int get_lower_pool(struct re_pgsql *pgsql) {
	int index;
	int ret;
	int min;

	min = pgsql->pool[0].queue_length;
	index = 0;
	ret = 0;
	while (index < pgsql->pool_length && min != 0) {
		if (pgsql->pool[index].queue_length < min) {
			min = pgsql->pool[index].queue_length;
			ret = index;
		}
		index += 1;
	}
	return (ret);
}

void re_pgsql_execute_stmt(struct re_pgsql *pgsql, int index_stmt, void (*bind_handler)(struct re_pgsql_bind *, void *), void (*result_handler)(PGresult *, void *), void *arg) {
	struct re_pgsql_queue *queue;
	int index;

	index = get_lower_pool(pgsql);
	queue = malloc(sizeof(struct re_pgsql_queue));
	queue->next = NULL;
	queue->index = index_stmt;
	queue->arg = arg;
	queue->bind_handler = bind_handler;
	queue->result_handler = result_handler;
	if (pgsql->pool[index].queue_length == 0) {
		pgsql->pool[index].queue = queue;
	} else {
		pgsql->pool[index].end->next = queue;
	}
	pgsql->pool[index].queue_length += 1;
	pgsql->pool[index].end = queue;
	if (pgsql->pool[index].state == RE_PGSQL_WAITING) {
		pgsql->pool[index].state = RE_PGSQL_CONNECTED;
		fd_listen(PQsocket(pgsql->pool[index].conn), FD_WRITE | FD_EXCEPT, re_pgsql_handler, &(pgsql->pool[index]));
	}
}
