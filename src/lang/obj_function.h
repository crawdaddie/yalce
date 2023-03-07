#ifndef _LANG_OBJ_FUNCTION_H
#define _LANG_OBJ_FUNCTION_H
#include "chunk.h"
#include "obj.h"
typedef struct {
  Object object;
  int arity;
  Chunk chunk;
  ObjString *name;
  int upvalue_count;
} ObjFunction;

ObjFunction *make_function();
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

typedef Value (*NativeFn)(int argc_count, Value *args);

typedef struct {
  Object object;
  NativeFn function;
} ObjNative;
ObjNative *make_native(NativeFn function);

/* struct Value {}; */
typedef struct {
  Object object;
  Value *location;
  struct ObjUpvalue *next;
  Value closed;
} ObjUpvalue;

ObjUpvalue *make_upvalue(Value *slot);

typedef struct {
  Object object;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalue_count;
} ObjClosure;

ObjClosure *make_closure(ObjFunction *function);

#endif
