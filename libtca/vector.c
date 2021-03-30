#include "vector.h"
#include <stdlib.h>
#include <string.h>

/*
 * Utils
 */

static int vector_realloc(struct vector *this) {
	this->limit *= 2;
	this->data = realloc(this->data, this->limit * sizeof(void *));
	return (0);
}

size_t vector_len(struct vector *this) {
	return (this->current);
}

void vector_rev(struct vector *this) {
	size_t beg;
	size_t end;
	void *swap;
	if (this->current < 2)
		return ;
	beg = 0;
	end = this->current - 1;
	while (beg < end) {
		swap = this->data[beg];
		this->data[beg] = this->data[end];
		this->data[end] = swap;
		beg += 1;
		end -= 1;
	}
}

void vector_shrink(struct vector *this) {
	size_t index;
	size_t limit;

	index = 0;
	while ((this->current >> index) != 1) {
		index += 1;
	}
	limit = 1 << (index + 1);
	if (limit == this->limit || limit <= VECTOR_DFL_SIZE) {
		return ;
	}
	this->data = realloc(this->data, limit * sizeof(void *));
}

/*
 * Initialization / Deletion
 */

struct vector *vector_new(void) {
	struct vector *this;
	if (!(this = (struct vector *)malloc(sizeof(struct vector)))) {
		return (NULL);
	}
	if (!(this->data = calloc(VECTOR_DFL_SIZE, sizeof(void *)))) {
		free(this);
		return (NULL);
	}
	this->limit = VECTOR_DFL_SIZE;
	this->current = 0;
	return (this);
}

void vector_init(struct vector *this) {
	if (!(this->data = calloc(VECTOR_DFL_SIZE, sizeof(void *)))) {
		return ;
	}
	this->limit = VECTOR_DFL_SIZE;
	this->current = 0;
}

void vector_del(struct vector *this, void (*fn)(void *)) {
	size_t index;
	if (fn != NULL) {
		index = 0;
		while (index < this->current) {
			fn(this->data[index]);
			index += 1;
		}
	}
	free(this->data);
	free(this);
}

void vector_destroy(struct vector *this, void (*fn)(void *)) {
	size_t index;
	if (fn != NULL) {
		index = 0;
		while (index < this->current) {
			fn(this->data[index]);
			index += 1;
		}
	}
	free(this->data);
}

void vector_clear(struct vector *this, void (*fn)(void *)) {
	size_t index;
	if (fn != NULL) {
		index = 0;
		while (index < this->current) {
			fn(this->data[index]);
			index += 1;
		}
	}
	this->current = 0;
}

/*
 * Getter
 */

void *vector_get(struct vector *this, size_t pos) {
	if (this->current < pos) {
		return (NULL);
	}
	return (this->data[pos]);
}

void *vector_get_first(struct vector *this) {
	if (this->current == 0)
		return (NULL);
	return (this->data[0]);
}

void *vector_get_last(struct vector *this) {
	if (this->current == 0)
		return (NULL);
	return (this->data[this->current - 1]);
}

int vector_get_index(struct vector *this, void *data) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (this->data[index] == data)
			return ((int)index);
		index += 1;
	}
	return (-1);
}

/*
 * Insert
 */

void vector_insert(struct vector *this, void *data, size_t pos) {
	if (pos > this->current) {
		vector_push_back(this, data);
		return ;
	}
	if (this->current + 1 == this->limit) {
		if (vector_realloc(this)) {
			return ;
		}
	}
	memmove(this->data + pos + 1, this->data + pos,
			(this->current - pos) * sizeof(void *));
	this->data[pos] = data;
	this->current += 1;
}

void vector_insert_when_match(struct vector *this, void *data, bool (*match_fn)(void *, void *)) {
	size_t pos;
	pos = 0;
	while (pos < this->current) {
		if (match_fn(this->data[pos], data))
			break ;
		pos += 1;
	}
	if (this->current + 1 == this->limit) {
		if (vector_realloc(this))
			return ;
	}
	if (pos == this->current) {
		this->data[this->current] = data;
	}
	else {
		memmove(this->data + pos + 1, this->data + pos,
				(this->current - pos) * sizeof(void *));
		this->data[pos] = data;
	}
	this->current += 1;
}

void vector_push_back(struct vector *this, void *data) {
	if (this->limit == this->current + 1) {
		if (vector_realloc(this))
			return ;
	}
	this->data[this->current] = data;
	this->current += 1;
}

void vector_push_front(struct vector *this, void *data) {
	if (this->limit == this->current + 1) {
		if (vector_realloc(this))
			return ;
	}
	memmove(this->data + 1, this->data, this->current * sizeof(void *));
	this->data[0] = data;
	this->current += 1;
}

/*
 * Pop
 */

void *vector_pop_back(struct vector *this) {
	void *data;
	if (this->current == 0) {
		return (NULL);
	}
	this->current -= 1;
	data = this->data[this->current];
	this->data[this->current] = NULL;
	return (data);
}

void *vector_pop_front(struct vector *this) {
	void *data;
	if (this->current == 0) {
		return (NULL);
	}
	data = this->data[0];
	this->current -= 1;
	memmove(this->data, this->data + 1, this->current * sizeof(void *));
	return (data);
}

void *vector_pop_index(struct vector *this, size_t pos) {
	void *data;
	if (this->current == 0 || pos > this->current) {
		return (NULL);
	}
	data = this->data[pos];
	this->current -= 1;
	memmove(this->data + pos, this->data + pos + 1,
										(this->current - pos) * sizeof(void *));
	this->data[this->current] = NULL;
	return (data);
}

/*
 * Iterate
 */

void vector_iter0(struct vector *this, void (*fn)(void *)) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index]);
		index += 1;
	}
}

void vector_iter2(struct vector *this, void (*fn)(void *, void *, void *), void *ctx1, void *ctx2) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index], ctx1, ctx2);
		index += 1;
	}
}

void vector_iter(struct vector *this, void (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index], ctx);
		index += 1;
	}
}

void vector_iteri0(struct vector *this, void (*fn)(void *, int)) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index], (int)index);
		index += 1;
	}
}

void vector_iteri2(struct vector *this, void (*fn)(void *, void *, void *, int), void *ctx1, void *ctx2) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index], ctx1, ctx2, (int)index);
		index += 1;
	}
}

void vector_iteri(struct vector *this, void (*fn)(void *, void *, int), void *ctx) {
	size_t index;
	index = 0;
	while (index < this->current) {
		fn(this->data[index], ctx, (int)index);
		index += 1;
	}
}

void vector_itern(struct vector *this, void (*fn)(void *, void *, void *), void *ctx) {
	size_t index;
	void *next;
	index = 0;
	while (index < this->current) {
		next = (index + 1 == this->current) ? NULL : this->data[index + 1];
		fn(this->data[index], next, ctx);
		index += 1;
	}
}

void vector_iterp(struct vector *this, void (*fn)(void *, void *, void *), void *ctx) {
	size_t index;
	void *prev;
	index = 0;
	prev = NULL;
	while (index < this->current) {
		fn(this->data[index], prev, ctx);
		prev = this->data[index];
		index += 1;
	}
}

void vector_iter_rev(struct vector *this, void (*fn)(void *, void *), void *ctx) {
	int index;
	index = (int)this->current - 1;
	while (index >= 0) {
		fn(this->data[index], ctx);
		index += 1;
	}
}

/*
 * Exists / Find
 */

bool vector_exists0(struct vector *this, bool (*fn)(void *)) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index]))
			return (true);
		index += 1;
	}
	return (false);
}

bool vector_exists(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index], ctx))
			return (true);
		index += 1;
	}
	return (false);
}

struct vector *vector_find0_all(struct vector *this, bool (*fn)(void *)) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index]))
			vector_push_back(ret, this->data[index]);
		index += 1;
	}
	return (ret);
}

struct vector *vector_find0_all_pop(struct vector *this, bool (*fn)(void *)) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index])) {
			vector_push_back(ret, vector_pop_index(this, index));
		}
		else {
			index += 1;
		}
	}
	return (ret);
}

void *vector_find0(struct vector *this, bool (*fn)(void *)) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index]))
			return (this->data[index]);
		index += 1;
	}
	return (NULL);
}

void *vector_find0_pop(struct vector *this, bool (*fn)(void *)) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index]))
			return (vector_pop_index(this, index));
		index += 1;
	}
	return (NULL);
}

void *vector_find0_rev(struct vector *this, bool (*fn)(void *)) {
	size_t index;
	index = this->current;
	while (index > 0) {
		index -= 1;
		if (fn(this->data[index]))
			return (this->data[index]);
	}
	return (NULL);
}

void *vector_find0_rev_pop(struct vector *this, bool (*fn)(void *)) {
	size_t index;
	index = this->current;
	while (index > 0) {
		index -= 1;
		if (fn(this->data[index]))
			return (vector_pop_index(this, index));
	}
	return (NULL);
}

struct vector *vector_find_all(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index], ctx))
			vector_push_back(ret, this->data[index]);
		index += 1;
	}
	return (ret);
}

struct vector *vector_find_all_pop(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index], ctx)) {
			vector_push_back(ret, vector_pop_index(this, index));
		}
		else {
			index += 1;
		}
	}
	return (ret);
}

void *vector_find(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index], ctx))
			return (this->data[index]);
		index += 1;
	}
	return (NULL);
}

void *vector_find_pop(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = 0;
	while (index < this->current) {
		if (fn(this->data[index], ctx))
			return (vector_pop_index(this, index));
		index += 1;
	}
	return (NULL);
}

void *vector_find_rev(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = this->current;
	while (index > 0) {
		index -= 1;
		if (fn(this->data[index], ctx))
			return (this->data[index]);
	}
	return (NULL);
}

void *vector_find_rev_pop(struct vector *this, bool (*fn)(void *, void *), void *ctx) {
	size_t index;
	index = this->current;
	while (index > 0) {
		index -= 1;
		if (fn(this->data[index], ctx))
			return (vector_pop_index(this, index));
	}
	return (NULL);
}

/*
 * Map
 */

struct vector *vector_copy(struct vector *this) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, this->data[index]);
		index += 1;
	}
	return (ret);
}

struct vector *vector_map0(struct vector *this, void *(*fn)(void *)) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index]));
		index += 1;
	}
	return (ret);
}

struct vector *vector_map2(struct vector *this, void *(*fn)(void *, void *, void *), void *ctx1, void *ctx2) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index], ctx1, ctx2));
		index += 1;
	}
	return (ret);
}

struct vector *vector_map(struct vector *this, void *(*fn)(void *, void *), void *ctx) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index], ctx));
		index += 1;
	}
	return (ret);
}

struct vector *vector_mapi0(struct vector *this, void *(*fn)(void *, int)) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index], (int)index));
		index += 1;
	}
	return (ret);
}

struct vector *vector_mapi2(struct vector *this, void *(*fn)(void *, void *, void *, int), void *ctx1, void *ctx2) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index], ctx1, ctx2, (int)index));
		index += 1;
	}
	return (ret);
}

struct vector *vector_mapi(struct vector *this, void *(*fn)(void *, void *, int), void *ctx) {
	struct vector *ret;
	size_t index;
	ret = vector_new();
	index = 0;
	while (index < this->current) {
		vector_push_back(ret, fn(this->data[index], ctx, (int)index));
		index += 1;
	}
	return (ret);
}

/*
 * Extends
 */

void vector_extends_back(struct vector *this, struct vector *next) {
	vector_rev(next);
	while (vector_len(next) != 0) {
		vector_push_back(this, vector_pop_back(next));
	}
	vector_del(next, NULL);
}

void vector_extends_front(struct vector *this, struct vector *prev) {
	vector_rev(this);
	while (vector_len(prev) != 0) {
		vector_push_back(this, vector_pop_back(prev));
	}
	vector_rev(this);
	vector_del(prev, NULL);
}
