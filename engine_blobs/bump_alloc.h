#ifndef _ENGINE_BUMP_ALLOC_H
#define _ENGINE_BUMP_ALLOC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bump allocator structure
typedef struct {
  void *memory;     // Base memory pointer
  size_t capacity;  // Total capacity in bytes
  size_t used;      // Current used bytes
  bool owns_memory; // Whether we own the memory (for cleanup)
} BumpAllocator;

#endif
