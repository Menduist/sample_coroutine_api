#include "coro.h"
#include <stdlib.h>
#include <stdio.h>

struct creroutine *cre_main = 0;
volatile struct creroutine *cre_current = 0;

void switch_to_main(void) {
	struct creroutine *backup = (struct creroutine *)cre_current;
	cre_current = 0;
	swapcontext((ucontext_t *)&backup->context, (ucontext_t *)&cre_main->context);
}

void switch_to_coro(struct creroutine *coro) {
	cre_current = coro;
	swapcontext((ucontext_t *)&cre_main->context, (ucontext_t *)&coro->context);
}

void store_result_and_switch(void *res, struct creroutine *coro) {
	void **tar = coro->target;
	*tar = res;
	switch_to_coro(coro);
}

int get_coro_id(void) {
	if (cre_current) return cre_current->id;
	return 0;
}

void coro_set_error_handler(void (*error_handler)(), void *arg) {
	cre_current->error_handler = error_handler;
	cre_current->error_arg = arg;
}

void coro_mraise(struct routine_error *err) {
	if (!cre_current->error_handler) coro_exit();
	makecontext((ucontext_t *)&cre_current->context, cre_current->error_handler, 2, err, cre_current->error_arg);
	setcontext((ucontext_t *)&cre_current->context);
}

void coro_raise(enum routine_error_type type, char *str) {
	struct routine_error *err = calloc(1, sizeof(struct routine_error));
	err->type = type;
	err->str = str;
	coro_mraise(err);
}

ucontext_t _killer_context;
void *killerstack;
ucontext_t _junk;
ucontext_t *killer_context;

static void coro_suicide() {
	while (1) {
		printf("[%d] Stack freed for %p:", cre_current->id, cre_current);
		printf(" %p\n", cre_current->stack);
		free(cre_current->stack);
		free((void *)cre_current);
		cre_current = 0;
		swapcontext(killer_context, &cre_main->context);
	}
}

void coro_exit(void) {
	printf("[%d] Stack freed for %p:", cre_current->id, cre_current);
	printf(" %p\n", cre_current->stack);
	free(cre_current->stack);
	free((void *)cre_current);
	cre_current = 0;
	setcontext(&cre_main->context);
}

struct creroutine *create_coro(void) {
	static int id = 1;
	if (!cre_main) {
		cre_main = calloc(1, sizeof(struct creroutine));
	}
	struct creroutine *result = calloc(1, sizeof(struct creroutine));
	result->id = id++;

	if (killer_context == 0) {
		getcontext(&_killer_context);
		killerstack = malloc(STACK_SIZE);
		_killer_context.uc_stack.ss_sp = killerstack;
		_killer_context.uc_stack.ss_size = STACK_SIZE;
		_killer_context.uc_link = 0;
		makecontext(&_killer_context, coro_suicide, 0);
		killer_context = &_killer_context;
	}

	getcontext(&result->context);
	result->stack = malloc(STACK_SIZE);
	printf("[%d] Stack allocated for %p: %p\n", result->id, result, result->stack);
	result->context.uc_stack.ss_sp = result->stack;
        result->context.uc_stack.ss_size = STACK_SIZE;
        result->context.uc_link = killer_context;
	return result;
}
