#include "bump_alloc.h"

// Initialize a bump allocator with a given capacity
BumpAllocator *bump_allocator_create(size_t capacity) {
  BumpAllocator *allocator = malloc(sizeof(BumpAllocator));
  if (!allocator) {
    return NULL;
  }

  void *memory = malloc(capacity);
  if (!memory) {
    free(allocator);
    return NULL;
  }

  allocator->memory = memory;
  allocator->capacity = capacity;
  allocator->used = 0;
  allocator->owns_memory = true;

  return allocator;
}

// Initialize a bump allocator with external memory
BumpAllocator *bump_allocator_create_with_memory(void *memory,
                                                 size_t capacity) {
  if (!memory) {
    return NULL;
  }

  BumpAllocator *allocator = malloc(sizeof(BumpAllocator));
  if (!allocator) {
    return NULL;
  }

  allocator->memory = memory;
  allocator->capacity = capacity;
  allocator->used = 0;
  allocator->owns_memory = false;

  return allocator;
}

// Allocate memory from the bump allocator
void *bump_alloc(BumpAllocator *allocator, size_t size) {
  if (!allocator || size == 0) {
    return NULL;
  }

  // Align size to 8 bytes (common alignment requirement)
  size_t aligned_size = (size + 7) & ~7;

  // Check if we have enough capacity
  if (allocator->used + aligned_size > allocator->capacity) {
    return NULL; // Out of memory
  }

  // Get the current allocation pointer
  void *ptr = (char *)allocator->memory + allocator->used;

  // Bump the pointer
  allocator->used += aligned_size;

  return ptr;
}

// Reset the bump allocator (free all allocations at once)
void bump_allocator_reset(BumpAllocator *allocator) {
  if (allocator) {
    allocator->used = 0;
  }
}

// Destroy the bump allocator
void bump_allocator_destroy(BumpAllocator *allocator) {
  if (allocator) {
    if (allocator->owns_memory) {
      free(allocator->memory);
    }
    free(allocator);
  }
}

// Utility to get remaining capacity
size_t bump_allocator_remaining(BumpAllocator *allocator) {
  if (!allocator) {
    return 0;
  }
  return allocator->capacity - allocator->used;
}

// // Example usage
// int main() {
//   // Create a bump allocator with 1MB of memory
//   BumpAllocator *allocator = bump_allocator_create(1024 * 1024);
//   if (!allocator) {
//     fprintf(stderr, "Failed to create allocator\n");
//     return 1;
//   }
//
//   // Allocate some memory
//   char *str1 = bump_alloc(allocator, 16);
//   strcpy(str1, "Hello, World!");
