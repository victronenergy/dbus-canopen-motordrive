#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdlib.h>

void *_malloc(size_t size);
void *_realloc(void *ptr, size_t size);
void _free(void *ptr);

#endif