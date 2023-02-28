#ifndef _LANG_VM_H
#define _LANG_VM_H
#include "chunk.h"
#include "obj.h"
#include "sym.h"
#include "value.h"
#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
  Object *objects;
  Table strings;
  Table globals;
} VM;
void init_vm();
void free_vm();

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

InterpretResult interpret(const char *source);

void push(Value value);
Value pop();
extern VM vm;

#endif
