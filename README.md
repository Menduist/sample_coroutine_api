# Example C Coroutine based api

This project is a dummy program illustrating my current "framework" for C programming. It has been use for numerous projects.

The main highlight of this sample is the coroutine system. It allows asynchronous linear programming (eg, without callbacks), which is uncommon in C.

```C
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
```
This function will be paused during the `db_get_echo` and `get_http_resp` functions (since they are IO operation), allowing other requests to be treated. When the response is received, the function will be seamlessly resumed.

This has been critical for more complex processes, where a single request could requires 10s of IO operations, which would be impractical in traditional callback based methods.

This program is also a good example of organization for small & medium size projects, with some nice features, such as YAML config file reading.

### Details
The "scheduling" and protocols are handled by the brillant [libre](https://github.com/creytiv/re). If libre doesn't support a protocol, it's very straightforward to add it (as done by [lchenut](https://github.com/lchenut)) in the `re_pgsql` used in this project)

The [libtca](https://github.com/Menduist/libtca) is used for various utils, and the `myaml` powerful unpacking.

The coroutines are not preempted (eg, a stuck coroutine would stuck the entire program). They can be preempted using an [itimer](https://linux.die.net/man/2/setitimer), but is not demonstrated here.

The program is also single-threaded. 

Files list :

- `main.c` | `main.h`: Main logic of the application. In a real application, it would be split into sub-files for sub-logics.
- `config.c` | `config.h`: Argv & config file reader. All options are stored in structures for easy reading. Can also dump config.
- `api.c` | `api.h`: HTTP(s) server. Handles routing, argument parsing, and response (eg json) generation.
- `coro.c` | `coro.h`: Coroutine handling.
- `http.c` | `http.h`: Helpers functions to use coroutines with libre requests
- `db/db.c` | `db/db.h`: Bridge from the program to the database. In a real application, this would handle the translation between the program structs and the database.
