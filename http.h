#ifndef MAIN_HTTP_H
#define MAIN_HTTP_H

struct http_resp {
	int err;
	const struct http_msg *msg;
};

void http_store_result_and_switch(int err, const struct http_msg *msg, void *arg);

struct http_resp *get_http_resp(void);

#endif

