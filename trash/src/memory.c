#include "memory.h"
#include <stdlib.h>
void *allocate(size_t size) { return calloc(size, 1); }
void release(void *object) { free(object); };
