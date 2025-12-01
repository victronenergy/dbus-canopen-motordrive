#include <memory.h>
#include <stdlib.h>

void *_malloc(size_t size) { return malloc(size); }

void *_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void _free(void *ptr) { free(ptr); }