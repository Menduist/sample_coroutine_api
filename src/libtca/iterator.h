#ifndef ITERATOR_H
#define ITERATOR_H

struct iterator;
typedef void *(*next_it)(struct iterator *);

struct iterator {
	void *container;
	void *elem;
	int i;
	next_it iterate;
};

void *iterator_next(struct iterator *it);
void *iterator_get(struct iterator *it);
int iterator_get_index(struct iterator *it);

#endif
