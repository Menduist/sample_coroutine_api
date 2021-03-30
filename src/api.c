#include "api.h"
#include "main.h"
#include "coro.h"
#include <string.h>
#include <stdlib.h>

/*
** Internal structs
*/
struct api {
        struct http_sock *sock;
};

struct http_route {
        const char *path;
        void (*fn)(struct main *, struct http_conn *, const struct http_msg *, struct odict *);
        int size;
};

/*
** Helpers
*/
static char *get_data_str(struct odict *data, char *key) {
        const struct odict_entry *entry = odict_lookup(data, key);
        if (entry && entry->type == ODICT_STRING)
                return strdup(entry->u.str);
        return 0;
}

static int64_t get_data_int(struct odict *data, char *key) {
        const struct odict_entry *entry = odict_lookup(data, key);
        if (entry && entry->type == ODICT_INT)
                return entry->u.integer;
        return 0;
}

static void prm_to_odict(const struct pl *pl, struct odict **data) {
        struct pl prmv;
        struct pl key;
        struct pl val;
        char ckey[255];
        char cval[255];

        if (!pl) {
                return ;
        }
        prmv = *pl;
        while (!re_regex(prmv.p, prmv.l, "[&?]*[^=]+=[^&]+", NULL, &key, &val)) {
                pl_advance(&prmv, val.p + val.l - prmv.p);
                if (!*data) {
                        odict_alloc(data, 8);
                }
                memcpy(ckey, key.p, key.l); ckey[key.l] = 0;
                memcpy(cval, val.p, val.l); cval[val.l] = 0;
                odict_entry_add(*data, ckey, ODICT_STRING, cval);
        }
}

static char *myjson_encode(struct odict *root) {
        int err;
        char *result = 0;

        if (root){
                err = re_sdprintf(&result, "%H", json_encode_odict, root);
                if (err){
                        fprintf(stderr, "%s\n", "Error while encoding json to mbuf.");
                }
        }
        return result;
}

static int http_reply_json(struct http_conn *conn, struct odict *json) {
        char *json_as_text = myjson_encode(json);
        printf("[%d] Reply %s\n", get_coro_id(), json_as_text);
        return http_creply(conn, 200, "OK", "application/json", "%s", json_as_text);
}

static void http_error_handler(struct routine_error *err, void *arg) {
        struct http_conn *conn = arg;
        int error_code;
        char *error_msg;
        switch (err->type) {
        case ERROR_USER:
                error_code = 400;
                error_msg = "Bad Request";
                break;
        case ERROR_INTERNAL:
                error_code = 500;
                error_msg = "Internal Error";
                break;
        }
        struct odict *response;
        odict_alloc(&response, 2);
        odict_entry_add(response, "error", ODICT_STRING, err->str);
        char *json = myjson_encode(response);
        http_reply(conn, error_code, error_msg, "Content-Length: %d\r\n\r\n%s", strlen(json), json);
        mem_deref(response);
        mem_deref(json);
}

/*
** Routes
*/
void on_db_test(struct main *main, struct http_conn *conn, const struct http_msg *msg, struct odict *data) {
        coro_set_error_handler(http_error_handler, conn);
        char *string = get_data_str(data, "string");

        char *result_string = get_database_echo(main, string);

        struct odict *result;
        odict_alloc(&result, 1);

        odict_entry_add(result, "echo", ODICT_STRING, result_string);

        http_reply_json(conn, result);
        mem_deref(result);
}

void on_get_url(struct main *main, struct http_conn *conn, const struct http_msg *msg, struct odict *data) {
        coro_set_error_handler(http_error_handler, conn);
        char *url = get_data_str(data, "url");

        char *result_string = get_url_value(main, url);

        http_reply(conn, 200, "Ok", "Content-Length: %d\r\n\r\n%s", strlen(result_string), result_string);
        mem_deref(result_string);
}

static struct http_route routes[] = {
        { "/db_test", on_db_test },
        { "/get_url", on_get_url },
        { NULL, NULL }
};

/*
** Request handler
*/
void on_request_coro(struct http_conn *conn, const struct http_msg *msg, void *arg) {
        int err;
        struct odict *data = 0;
        if (pl_strcmp(&msg->met, "GET") == 0) {
                prm_to_odict(&msg->prm, &data);
                printf("[%d] Launch coroutine: %.*s %.*s\n", get_coro_id(), msg->path.l, msg->path.p, msg->prm.l, msg->prm.p);
        } else if (mbuf_get_left(msg->mb) > 0) {
                printf("[%d] Launch coroutine: %.*s %.*s\n", get_coro_id(), msg->path.l, msg->path.p, mbuf_get_left(msg->mb), mbuf_buf(msg->mb));
                err = json_decode_odict(&data, 16, mbuf_buf(msg->mb), mbuf_get_left(msg->mb), 8);
                if (err) {
                        http_reply(conn, 400, "Bad Request", "Content-Length: 0\r\n\r\n");
                        return;
                }
        }

        bool found = 0;
        for (int i = 0; routes[i].path; i++) {
                if (!strncmp(routes[i].path, msg->path.p, routes[i].size)) {
                        routes[i].fn(arg, conn, msg, data);
                        found = 1;
                        break;
                }
        }
        if (!found)
                http_reply(conn, 404, "Not Found", "Content-Length: 0\r\n\r\n");
        if (data) {
                mem_deref(data);
        }
        mem_deref(conn);
}

void on_request(struct http_conn *conn, const struct http_msg *msg, void *arg) {
        mem_ref(conn);
        LAUNCH_CORO(on_request_coro, 3, conn, msg, arg);
}

/*
** Init
*/
#define TRYC(c) err = c; if (err) { printf(#c " failed: %s\n", strerror(err)); goto out;}
struct api *api_init(struct main *main, struct config_api *conf) {
        struct api *result = calloc(1, sizeof(struct api));
        int err = 0;

        for (int i = 0; routes[i].path; i++) {
                routes[i].size = strlen(routes[i].path);
        }
        struct sa local;
        sa_init(&local, AF_INET);
        sa_set_port(&local, conf->port);
        TRYC(http_listen(&result->sock, &local, on_request, main));
out:
        return result;
}
