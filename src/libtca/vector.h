#ifndef VECTOR_H
# define VECTOR_H

# include <unistd.h>
# include <stdbool.h>
# define VECTOR_DFL_SIZE 64

struct vector {
	void **data;
	size_t limit;
	size_t current;
};

struct vector *vector_new(void);
void vector_init(struct vector *vect);
void vector_del(struct vector *vect, void (*fn)(void *));
void vector_destroy(struct vector *vect, void (*fn)(void *));

void vector_clear(struct vector *vect, void (*fn)(void *));

size_t vector_len(struct vector *vect);
void vector_shrink(struct vector *vect);

void vector_push_front(struct vector *vect, void *data);
void vector_push_back(struct vector *vect, void *data);
void vector_insert(struct vector *vect, void *data, size_t pos);
void vector_insert_when_match(struct vector *vect, void *data, bool(*fn)(void *, void *));

void *vector_pop_front(struct vector *vect);
void *vector_pop_back(struct vector *vect);
void *vector_pop_index(struct vector *vect, size_t pos);

void *vector_get(struct vector *vect, size_t pos);
void *vector_get_first(struct vector *vect);
void *vector_get_last(struct vector *vect);
int vector_get_index(struct vector *vect, void *data);

void vector_iter0(struct vector *vect, void (*fn)(void *));
void vector_iter(struct vector *vect, void (*fn)(void *, void *), void *ctx);
void vector_iter2(struct vector *vect, void (*fn)(void *, void *, void *), void *ctx1, void *ctx2);
void vector_iteri0(struct vector *vect, void (*fn)(void *, int));
void vector_iteri(struct vector *vect, void (*fn)(void *, void *, int), void *ctx);
void vector_iteri2(struct vector *vect, void (*fn)(void *, void *, void *, int), void *ctx1, void *ctx2);
void vector_iterp(struct vector *vect, void (*fn)(void *, void *, void*), void *ctx);
void vector_itern(struct vector *vect, void (*fn)(void *, void *, void*), void *ctx);
void vector_iter_rev(struct vector *vect, void(*fn)(void *, void *), void *ctx);

struct vector *vector_copy(struct vector *vect);
struct vector *vector_map0(struct vector *vect, void *(*fn)(void *));
struct vector *vector_map(struct vector *vect, void *(*fn)(void *, void *), void *ctx);
struct vector *vector_map2(struct vector *vect, void *(*fn)(void *, void *, void *), void *ctx1, void *ctx2);
struct vector *vector_mapi0(struct vector *vect, void *(*fn)(void *, int));
struct vector *vector_mapi(struct vector *vect, void *(*fn)(void *, void *, int), void *ctx);
struct vector *vector_mapi2(struct vector *vect, void *(*fn)(void *, void *, void *, int), void *ctx1, void *ctx2);

bool vector_exists0(struct vector *vect, bool (*fn)(void *));
bool vector_exists(struct vector *vect, bool (*fn)(void *, void *), void *ctx);

void *vector_find0(struct vector *vect, bool (*fn)(void *));
void *vector_find(struct vector *vect, bool (*fn)(void *, void *), void *ctx);
void *vector_find0_pop(struct vector *vect, bool (*fn)(void *));
void *vector_find_pop(struct vector *vect, bool (*fn)(void *, void *), void *ctx);
void *vector_find0_rev(struct vector *vect, bool (*fn)(void *));
void *vector_find_rev(struct vector *vect, bool (*fn)(void *, void *), void *ctx);
void *vector_find0_rev_pop(struct vector *vect, bool (*fn)(void *));
void *vector_find_rev_pop(struct vector *vect, bool (*fn)(void *, void *), void *ctx);

struct vector *vector_find0_all(struct vector *vect, bool (*fn)(void *));
struct vector *vector_find_all(struct vector *vect, bool (*fn)(void *, void *), void *ctx);
struct vector *vector_find0_all_pop(struct vector *vect, bool (*fn)(void *));
struct vector *vector_find_all_pop(struct vector *vect, bool (*fn)(void *, void *), void *ctx);

void vector_rev(struct vector *vect);

void vector_extends_front(struct vector *vect, struct vector *prev);
void vector_extends_back(struct vector *vect, struct vector *next);

int vector_realloc__(struct vector *vect);

#endif
