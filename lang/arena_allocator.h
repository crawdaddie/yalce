#ifndef LANG_ARENA_ALLOCATOR_H
#define LANG_ARENA_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef ARENA_DEFAULT_BLOCK_SIZE
#define ARENA_DEFAULT_BLOCK_SIZE (64 * 1024) // 64KB
#endif

#ifndef ARENA_ALIGNMENT
#define ARENA_ALIGNMENT 8
#endif

// Arena block structure - each block is a chunk of memory
typedef struct ArenaBlock {
  struct ArenaBlock *next;
  size_t size;
  size_t used;
} ArenaBlock;

typedef struct {
  ArenaBlock *current_block;
  size_t default_block_size;
  size_t total_allocated;
  size_t block_count;
} Arena;

#define ARENA_BLOCK_DATA(block) ((char *)(block) + sizeof(ArenaBlock))

#define ARENA_ALIGN_SIZE(size)                                                 \
  (((size) + (ARENA_ALIGNMENT - 1)) & ~(ARENA_ALIGNMENT - 1))

// Macro to declare an arena allocator instance and functions
#define DECLARE_ARENA_ALLOCATOR(name, block_size)                              \
  static Arena name##_arena = {NULL, block_size, 0, 0};                        \
                                                                               \
  static void *name##_alloc(size_t size) {                                     \
    if (size == 0)                                                             \
      return NULL;                                                             \
                                                                               \
    size_t aligned_size = ARENA_ALIGN_SIZE(size);                              \
                                                                               \
    if (!name##_arena.current_block ||                                         \
        name##_arena.current_block->used + aligned_size >                      \
            name##_arena.current_block->size) {                                \
                                                                               \
      size_t new_block_data_size =                                             \
          aligned_size > name##_arena.default_block_size                       \
              ? aligned_size                                                   \
              : name##_arena.default_block_size;                               \
      size_t total_block_size = sizeof(ArenaBlock) + new_block_data_size;      \
                                                                               \
      ArenaBlock *new_block = (ArenaBlock *)malloc(total_block_size);          \
      if (!new_block)                                                          \
        return NULL;                                                           \
                                                                               \
      new_block->next = name##_arena.current_block;                            \
      new_block->size = new_block_data_size;                                   \
      new_block->used = 0;                                                     \
                                                                               \
      name##_arena.current_block = new_block;                                  \
      name##_arena.total_allocated += total_block_size;                        \
      name##_arena.block_count++;                                              \
    }                                                                          \
                                                                               \
    void *ptr = ARENA_BLOCK_DATA(name##_arena.current_block) +                 \
                name##_arena.current_block->used;                              \
    name##_arena.current_block->used += aligned_size;                          \
                                                                               \
    return ptr;                                                                \
  }                                                                            \
                                                                               \
  static void name##_reset(void) {                                             \
    ArenaBlock *block = name##_arena.current_block;                            \
    while (block) {                                                            \
      ArenaBlock *next = block->next;                                          \
      free(block);                                                             \
      block = next;                                                            \
    }                                                                          \
    name##_arena.current_block = NULL;                                         \
    name##_arena.total_allocated = 0;                                          \
    name##_arena.block_count = 0;                                              \
  }                                                                            \
                                                                               \
  static size_t name##_total_allocated(void) {                                 \
    return name##_arena.total_allocated;                                       \
  }                                                                            \
                                                                               \
  static size_t name##_total_used(void) {                                      \
    size_t used = 0;                                                           \
    ArenaBlock *block = name##_arena.current_block;                            \
    while (block) {                                                            \
      used += block->used;                                                     \
      block = block->next;                                                     \
    }                                                                          \
    return used;                                                               \
  }                                                                            \
                                                                               \
  static size_t name##_block_count(void) { return name##_arena.block_count; }  \
                                                                               \
  static size_t name##_current_block_remaining(void) {                         \
    if (!name##_arena.current_block)                                           \
      return 0;                                                                \
    return name##_arena.current_block->size -                                  \
           name##_arena.current_block->used;                                   \
  }

#define DECLARE_ARENA_ALLOCATOR_DEFAULT(name)                                  \
  DECLARE_ARENA_ALLOCATOR(name, ARENA_DEFAULT_BLOCK_SIZE)

#define WITH_ARENA_ALLOCATOR(name, code_block)                                 \
  do {                                                                         \
    code_block;                                                                \
    name##_reset();                                                            \
  } while (0)

#define ARENA_ALLOC_TYPE(arena_name, type)                                     \
  ((type *)arena_name##_alloc(sizeof(type)))

#define ARENA_ALLOC_ARRAY(arena_name, type, count)                             \
  ((type *)arena_name##_alloc(sizeof(type) * (count)))

#define ARENA_ALLOC_ZEROED(arena_name, size)                                   \
  memset(arena_name##_alloc(size), 0, size)

#ifdef _STRING_H
#define ARENA_STRDUP(arena_name, str)                                          \
  ({                                                                           \
    size_t len = strlen(str) + 1;                                              \
    char *copy = (char *)arena_name##_alloc(len);                              \
    copy ? memcpy(copy, str, len) : NULL;                                      \
  })
#endif

#endif // ARENA_ALLOCATOR_H

/*
Usage Examples:

// Basic usage in escape_analysis.c:
#include "arena_allocator.h"

DECLARE_ARENA_ALLOCATOR_DEFAULT(ea);

typedef void *(*AllocatorFnType)(size_t size);
AllocatorFnType ea_alloc_fn = ea_alloc;

void escape_analysis(Ast *prog) {
    EACtx ctx = {};

    // All allocations go through ea_alloc
    Allocation *alloc = ea_alloc(sizeof(Allocation));

    ea(prog, &ctx);

    // Clean up everything at once
    ea_reset();

    printf("Used %zu bytes across %zu blocks\n",
           ea_total_used(), ea_block_count());
}

// Custom block size for memory-intensive module:
DECLARE_ARENA_ALLOCATOR(parser, 256 * 1024); // 256KB blocks

void parse_file(const char *filename) {
    Node *node = ARENA_ALLOC_TYPE(parser, Node);
    Token *tokens = ARENA_ALLOC_ARRAY(parser, Token, 1000);

    // ... parsing logic ...

    parser_reset();
}

// Scoped usage:
void some_function() {
    WITH_ARENA_ALLOCATOR(temp, 4096, {
        char *buffer = temp_alloc(1024);
        Data *data = ARENA_ALLOC_TYPE(temp, Data);
        // ... use allocations ...
        // temp_reset() called automatically
    });
}

*/
