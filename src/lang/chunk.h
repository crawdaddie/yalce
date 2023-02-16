#ifndef _LANG_CHUNK_H
#define _LANG_CHUNK_H
#include "common.h"
#include "value.h"

typedef enum { OP_RETURN, OP_CONSTANT, } OpCode;
typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  ValueArray constants;
} Chunk;
void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);
void free_chunk(Chunk *chunk);
int add_constant(Chunk *chunk, Value value);

#endif
