#ifndef _LANG_VM_H
#define _LANG_VM_H
#include "chunk.h"
#include "obj.h"
#include "obj_function.h"
#include "sym.h"
#include "value.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
typedef struct {
  ObjFunction *function;
  uint8_t *ip;
  Value *slots; // pointer to VM's stack
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
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
