#ifndef _ENGINE_ALLOC_H
#define _ENGINE_ALLOC_H
#include <stddef.h>

typedef char *(*allocator_fn)(size_t size);

extern allocator_fn g_allocator;

void set_allocator(allocator_fn allocator);

allocator_fn get_allocator(void);

char *engine_alloc(size_t size);

#endif
