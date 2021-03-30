#include <ucontext.h>
#define STACK_SIZE 16384

enum routine_error_type {
	ERROR_USER,
	ERROR_INTERNAL
};

struct routine_error {
	enum routine_error_type type;
	char *str;
};

struct creroutine {
	int id;
	void *stack;
	ucontext_t context;
	void *target;

	void *error_arg;
	void (*error_handler)();
};

struct creroutine *create_coro(void);
void switch_to_main(void);
void switch_to_coro(struct creroutine *coro);
void store_result_and_switch(void *result, struct creroutine *coro);
void coro_exit(void);

void coro_set_error_handler(void (*error_handler)(), void *arg);
void coro_raise(enum routine_error_type type, char *str);

int get_coro_id(void);

#define LAUNCH_CORO(func, argcount, ...) \
	{ struct creroutine *coro = create_coro(); \
	makecontext((ucontext_t *)&coro->context, (void (*)(void))func, argcount, __VA_ARGS__); \
	cre_current = coro; \
	swapcontext((ucontext_t *)&cre_main->context, &coro->context); }

extern struct creroutine *cre_main;
extern volatile struct creroutine *cre_current;
