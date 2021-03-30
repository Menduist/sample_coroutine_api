#include "intmap.h"
#include "iterator.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

void *intmap_get(struct intmap *v, int index) {
	if (!v->data) return 0;
	return v->data[index - v->min];
}

int intmap_init(struct intmap *v, int center) {
	int min = TCAMAX(0, center - 32);
	int max = center + 32;

	v->data = calloc(max - min, sizeof(void *));
	v->min = min;
	v->max = max;
	return 0;
}

void intmap_set(struct intmap *v, int index, void *data) {
	if (v->data == 0) {
		intmap_init(v, index);
	}
	if (index < v->min) {
		int offset = v->min - index;
		void **newdata = calloc(v->max - index + 1, sizeof(void *));
		memcpy(newdata + offset, v->data, sizeof(void *) * (v->max - v->min));
		free(v->data);
		v->data = newdata;
		v->min = index;
	} else if (index >= v->max) {
		int newsize = TCAMAX((v->max - v->min) * 2, index - v->min);
		int cursize = intmap_size(v);
		v->data = realloc(v->data, newsize * sizeof(void *));
		memset(v->data + cursize, 0, (newsize - cursize) * sizeof(void *));
		v->max = newsize + v->min;
	}
	v->data[index - v->min] = data;
}

int intmap_size(struct intmap *v) {
	return v->max - v->min;
}

void *__intmap_iterate(struct iterator *it) {
	struct intmap *v = (struct intmap *)it->container;
	it->i++;
	while (it->i < v->max) {
		if (v->data[it->i - v->min]) {
			return v->data[it->i - v->min];
		}
		it->i++;
	}
	return 0;
}

void *__intmap_iterate_rev(struct iterator *it) {
	struct intmap *v = (struct intmap *)it->container;
	it->i--;
	while (it->i >= v->min) {
		if (v->data[it->i - v->min]) {
			return v->data[it->i - v->min];
		}
		it->i--;
	}
	return 0;
}

struct iterator intmap_iterator(struct intmap *v) {
	struct iterator result;
	result.container = v;
	result.i = v->min - 1;
	result.iterate = __intmap_iterate;
	return result;
}

struct iterator intmap_rev_iterator(struct intmap *v) {
	struct iterator result;
	result.container = v;
	result.i = v->max;
	result.iterate = __intmap_iterate_rev;
	return result;
}
