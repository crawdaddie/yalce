#include "./alloc.h"
#include <stdlib.h>

typedef struct AllocatorBacking {
  char *data; // The actual compiled blob data
  char *_mem_ptr;
} AllocatorBacking;

static char *default_allocator(size_t size) { return (char *)malloc(size); }

allocator_fn g_allocator = default_allocator;

void set_allocator(allocator_fn allocator) {
  if (allocator != NULL) {
    g_allocator = allocator;
  } else {
    g_allocator = default_allocator;
  }
}

allocator_fn get_allocator(void) { return g_allocator; }

char *engine_alloc(size_t size) { return g_allocator(size); }
