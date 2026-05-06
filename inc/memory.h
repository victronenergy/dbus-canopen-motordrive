#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <logger.h>
#include <stdlib.h>

void *_malloc(size_t size);
void *_realloc(void *ptr, size_t size);
void _free(void *ptr);
char *_strdup(const char *s);

#define CHECK_ALLOC(ptr)                                                       \
    do {                                                                       \
        if ((ptr) == NULL) {                                                   \
            error("Allocation failed at %s:%d", __FILE__, __LINE__);           \
            pltExit(5);                                                        \
        }                                                                      \
    } while (0)

#endif