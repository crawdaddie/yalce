#ifndef _LANG_CHUNK_H
#define _LANG_CHUNK_H
#include "common.h"
#include "value.h"

typedef enum {
  OP_RETURN,
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_LT,
  OP_GT,
  OP_LTE,
  OP_GTE,
  OP_PIPE,
  OP_EQUAL,
  OP_CALL,
  OP_NEGATE,
  OP_NOT,
  OP_PRINT,
  OP_POP,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_LOCAL,
  OP_GET_LOCAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_CLOSE_UPVALUE,
  OP_JUMP_IF_FALSE,
  OP_JUMP,
  OP_LOOP,
  OP_CLOSURE,
  OP_ALLOC_ARRAY_LITERAL,
  OP_ARRAY_INDEX,
  OP_ARRAY_INDEX_ASSIGNMENT,
} OpCode;

typedef struct Chunk {
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
