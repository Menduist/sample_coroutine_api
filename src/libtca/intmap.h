#ifndef INTMAP_H
#define INTMAP_H

struct intmap {
	void **data;
	int min;
	int max;
};

int intmap_init(struct intmap *v, int center);

void *intmap_get(struct intmap *v, int index);
void intmap_set(struct intmap *v, int index, void *data);

int intmap_size(struct intmap *v);

struct iterator intmap_iterator(struct intmap *v);
struct iterator intmap_rev_iterator(struct intmap *v);

#endif
