#ifndef _MEMORY_H
#define _MEMORY_H
#include <string.h>

void *allocate(size_t size);
void release(void *obj);

#endif
