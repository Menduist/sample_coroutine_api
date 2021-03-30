#ifndef HASHTABLE_H
#define HASHTABLE_H

struct key {
	void *key;
	int len;
};

struct hashtable_elem {
	char used;

	struct key key;
	void *value;
	struct hashtable_elem *next;
};

struct hashtable {
	int size;
	struct hashtable_elem *table;
};

struct hashtable *create_hashtable(int size);
void hashtable_inserts(struct hashtable *hashtable, char *key, void *value);
void hashtable_insert(struct hashtable *hashtable, void *key, int keylen, void *value);
void *hashtable_gets(struct hashtable *hashtable, char *key);
void *hashtable_get(struct hashtable *hashtable, void *key, int keylen);
void hashtable_removes(struct hashtable *hashtable, char *key);
void hashtable_remove(struct hashtable *hashtable, void *key, int keylen);

void dump_hashtable(struct hashtable *hashtable);

#endif
