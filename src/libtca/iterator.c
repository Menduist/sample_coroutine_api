#include "iterator.h"

void *iterator_next(struct iterator *it) {
	it->elem = it->iterate(it);
	return it->elem;
}

void *iterator_get(struct iterator *it) {
	return it->elem;
}	

int iterator_get_index(struct iterator *it) {
	return it->i;
}
