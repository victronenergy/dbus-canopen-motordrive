#include <memory.h>
#include <stdlib.h>
#include <string.h>

void *_malloc(size_t size) { return malloc(size); }

void *_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void _free(void *ptr) { free(ptr); }

char *_strdup(const char *s) { return strdup(s); }