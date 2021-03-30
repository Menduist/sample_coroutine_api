#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

void filter_null(void **array, int start, int max);
char *strcdup(const char *s, int c);
int array_len(void **array);
void array_rev(void *array, int array_len, int elem_length);
void *memdup(const void *src, size_t size);
void append_to_array(void **array, void *elem);
static inline int TCAMAX(int a, int b) { return a > b ? a : b; }
static inline int TCAMIN(int a, int b) { return a < b ? a : b; }
//static inline int pow10(int a) { int results[] = {1, 10, 100, 1000, 10000, 100000, 1000000}; return results[a]; }
int is_file_valid(const char *pathname);
unsigned adler32(char *data, int len);
uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);
void memdump(void *t, int l);
char **strsplit(char *c, char *delim);
char *mfgets(char *buf, int len, FILE *file);


#endif
