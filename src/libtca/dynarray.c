#include "dynarray.h"
#include "utils.h"
#include <string.h>

void dynarray_append(void ***array, void *value) {
	int curlen = *array ? array_len(*array) : 0;
	*array = realloc(*array, sizeof(void *) * (curlen + 2));
	(*array)[curlen] = value;
	(*array)[curlen + 1] = 0;
}

void dynarray_remove(void ***array, void *value) {
	int i = 0;
	while ((*array)[i]) {
		if ((*array)[i] == value) {
			memmove((*array) + i, (*array) + i + 1, (array_len(*array) - i) * sizeof(void *));
			return;
		}
		i++;
	}
}

