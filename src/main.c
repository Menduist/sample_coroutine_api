#include "config.h"
#include "main.h"
#include "db/db.h"
#include "coro.h"
#include "api.h"
#include "http.h"
#include <string.h>

#define TRYC(c) err = c; if (err) { printf(#c " failed: %s\n", strerror(err)); goto out;}

char *get_database_echo(struct main *main, char *str) {
	//This function would contain actual logic
	char *echo = db_get_echo(main->db, str);
	printf("Result: %s\n", echo);
	return echo;
}

char *get_url_value(struct main *main, char *url) {
	printf("Mapping '%s' url through database\n");
	url = db_get_echo(main->db, url);
	printf("Getting %s data\n", url);
	http_request(NULL, main->http_client, "GET", url, http_store_result_and_switch, NULL, (void *)cre_current,
		"Content-Length: 0\r\n"
                "\r\n"
                );
	struct http_resp *resp = get_http_resp();
        if (!resp->msg || (resp->msg->scode / 100 != 2)) return 0;
	char *result = 0;
	mbuf_strdup(resp->msg->mb, &result, mbuf_get_left(resp->msg->mb));
	return result;
}

/*
** Main init
*/
int main(int ac, char **av) {
        int err = 0;
	struct main main;

        setbuf(stdout, NULL);
        printf("Init..\n");
        TRYC(config_init(&main.config, ac, av));
        TRYC(config_read_yaml(&main.config));

        TRYC(libre_init());
        main.db = db_init(&main, &main.config.db);
        if (!main.db) return 1;
        if (main.config.api) main.api = api_init(&main, main.config.api);

        struct dnsc *dnsc;
        struct sa dns;
        sa_set_str(&dns, "8.8.8.8", 53);
        TRYC(dnsc_alloc(&dnsc, NULL, &dns, 1));
        TRYC(http_client_alloc(&main.http_client, dnsc));

        printf("Starting main loop\n");
        TRYC(re_main(NULL));
out:
        return err;
}
