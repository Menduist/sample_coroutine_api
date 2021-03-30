#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "utils.h"
void filter_null(void **array, int start, int max) {
	if (start != -1) {
		int y = start, i = start;
		while (i < max) {
			while (array[i] == 0 && i < max)
				i++;
			array[y++] = array[i++];
		}
	}
}

char *strcdup(const char *s, int c) {
	int len = strchr(s, c) - s;
	return strndup(s, len);
}

int array_len(void **array) {
	int i = 0;
	while (array[i]) {
		i++;
	}
	return i;
}

void memdump(void *t, int l) {
	while (l--) {
		printf("%02X", *((char *)t) & 0xff);
		t++;
	}
}

void array_rev(void *array, int array_len, int elem_length) {
	unsigned char buf[elem_length];
	int i = 0;

	while (i < array_len / 2) {
		memmove(buf, array + (i * elem_length), elem_length);
		memmove(array + (i * elem_length), array + (array_len - i) * elem_length, elem_length);
		memmove(array + (array_len - i) * elem_length, buf, elem_length);
		i++;
	}
}

void *memdup(const void *src, size_t n) {
	void *result = malloc(n);
	return memcpy(result, src, n);
}

void append_to_array(void **array, void *elem) {
	int i = 0;
	while (array[i]) {
		i++;
	}
	array[i] = elem;
}

int is_file_valid(const char *pathname) {
	struct stat	sb;

	if (stat(pathname, &sb) == -1) {
		perror(pathname);
		return (0);
	}
	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s: Not a regular file\n", pathname);
		return (0);
	}
	if (access(pathname, R_OK) == -1) {
		perror(pathname);
		return (0);
	}
	return (1);
}

unsigned adler32(char *data, int len) {
	const unsigned	mod_adler = 65521;
	size_t		index;
	unsigned	a;
	unsigned	b;

	a = 1;
	b = 0;
	for (index = 0; index < len; index += 1) {
		a = (a + data[index]) % mod_adler;
		b = (b + a) % mod_adler;
	}
	return ((b << 16) | a);
}

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
	uint32_t h = seed;
	if (len > 3) {
		const uint32_t* key_x4 = (const uint32_t*) key;
		size_t i = len >> 2;
		do {
			uint32_t k = *key_x4++;
			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;
			h ^= k;
			h = (h << 13) | (h >> 19);
			h = (h * 5) + 0xe6546b64;
		} while (--i);
		key = (const uint8_t*) key_x4;
	}
	if (len & 3) {
		size_t i = len & 3;
		uint32_t k = 0;
		key = &key[i - 1];
		do {
			k <<= 8;
			k |= *key--;
		} while (--i);
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
	}
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

char **strsplit(char *c, char *delim) {
	char *cur;
	char **result = calloc(strlen(c) + 2, sizeof(char *)); //TODO use less memory
	int i = 0;
	while ((cur = strtok(c, delim))) {
		result[i++] = strdup(cur);
		c = 0;
	}
	return result;
}

char *mfgets(char *buf, int len, FILE *file) {
	int curlen = 0;
	char *ret;
	do {
		ret = fgets(buf + curlen, len - curlen, file);
		curlen = strlen(buf);
	} while(ret && buf[curlen - 1] != '\n');
	return ret;
}
