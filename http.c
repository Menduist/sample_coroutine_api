#include "http.h"
#include "coro.h"
#include <stdlib.h>

void http_store_result_and_switch(int err, const struct http_msg *msg, void *arg) {
	struct creroutine *coro = arg;
	struct http_resp *resp = coro->target;
	resp->err = err;
	resp->msg = msg;
	switch_to_coro(coro);
}

struct http_resp *get_http_resp(void) {
	struct http_resp *result = calloc(1, sizeof(struct http_resp));
	cre_current->target = result;
	switch_to_main();
	return result;
}
