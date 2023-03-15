#include "memory.h"
#include <stdlib.h>
void *allocate(size_t size) { return malloc(size); }
void release(void *object) { free(object); };
