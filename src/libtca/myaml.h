#ifndef MYAML_H
#define MYAML_H

#include <stdio.h>

enum myaml_type {
	MYAML_OBJECT,
	MYAML_ARRAY,
	MYAML_SCALAR,
	MYAML_NULL,
};

struct myaml_t {
	enum myaml_type type;

	/* Object */
	char **keys;
	struct myaml_t **childs;

	/* Scalar */
	char *value;
};

struct myaml_t *myaml_loadf(FILE *input);
struct myaml_t *myaml_loads(char *str);
void dump_myaml_t(struct myaml_t *t, int level);
void dump_myaml_type(enum myaml_type t);

struct myaml_t *myaml_object_get(struct myaml_t *obj, char *key);
struct myaml_t *myaml_array_get(struct myaml_t *obj, int index);
int myaml_array_size(struct myaml_t *obj);
char *myaml_scalar_value(struct myaml_t *obj);
struct myaml_t *myaml_null(void);

void myaml_free(struct myaml_t *f);
int myaml_unpack(struct myaml_t *f, const char *fmt, ...);

#endif
