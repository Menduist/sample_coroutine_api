#include "hashtable.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

const uint32_t seed = 0xF23F5F;

struct hashtable *create_hashtable(int size) {
	struct hashtable *result = calloc(1, sizeof(struct hashtable));
	result->size = size;
	result->table = calloc(size, sizeof(struct hashtable_elem));
	return result;
}

void dump_hashtable(struct hashtable *hashtable) {
	printf("Hashtable buckets:\n");
	int i = 0;
	while (i < hashtable->size) {
		if (hashtable->table[i].used) {
			printf(" %d ==> %s = %p\n", i, hashtable->table[i].key.key, hashtable->table[i].value);
			struct hashtable_elem *elem = hashtable->table[i].next;
			while (elem && elem->used) {
				printf("      %s = %p\n", elem->key.key, elem->value);
				elem = elem->next;
			}
		}
		i++;
	}
}

void hashtable_inserts(struct hashtable *hashtable, char *key, void *value) {
	hashtable_insert(hashtable, key, strlen(key), value);
}

void hashtable_insert(struct hashtable *hashtable, void *key, int keylen, void *value) {
	uint32_t hashed = murmur3_32(key, keylen, seed);
	int index = hashed % hashtable->size;
	int i = 0;

	struct hashtable_elem *ptr = &hashtable->table[index];
	while (ptr->used && ptr->next) {
		ptr = ptr->next;
		i++;
	}
	if (i > 3) {
		printf("Warning: more than 3 collisions on a hashtable: %d\n", i);
	}
	if (ptr->used) {
		ptr->next = calloc(1, sizeof(struct hashtable_elem));
		ptr = ptr->next;
	}

	ptr->used = 1;
	ptr->key.key = key;
	ptr->key.len = keylen;
	ptr->value = value;
}

static struct hashtable_elem *_hashtable_get(struct hashtable *hashtable, void *key, int keylen) {
	uint32_t hashed = murmur3_32(key, keylen, seed);
	int index = hashed % hashtable->size;
	
	struct hashtable_elem *ptr = &hashtable->table[index];
	while (ptr && ptr->used) {
		if (keylen == ptr->key.len && memcmp(ptr->key.key, key, keylen) == 0) {
			return ptr;
		}
		ptr = ptr->next;
	}
	return 0;
}

void *hashtable_gets(struct hashtable *hashtable, char *key) {
	return hashtable_get(hashtable, key, strlen(key));
}


void *hashtable_get(struct hashtable *hashtable, void *key, int keylen) {
	struct hashtable_elem *elem = _hashtable_get(hashtable, key, keylen);

	if (elem) return elem->value;
	return 0;
}

void hashtable_removes(struct hashtable *hashtable, char *key) {
	hashtable_remove(hashtable, key, strlen(key));
}

void hashtable_remove(struct hashtable *hashtable, void *key, int keylen) {
	struct hashtable_elem *elem = _hashtable_get(hashtable, key, keylen);
	
	if (!elem) return;
	struct hashtable_elem *ptr = elem;
	while (ptr && ptr->next && ptr->next->used) {
		ptr = ptr->next;
	}
	if (ptr != elem) {
		ptr->key = elem->key;
		ptr->value = elem->value;
		elem->used = 0;
	} else {
		elem->used = 0;
	}
}
