#ifndef _LANG_OBJ_FUNCTION_H
#define _LANG_OBJ_FUNCTION_H
#include "chunk.h"
#include "obj.h"
typedef struct {
  Object object;
  int arity;
  Chunk chunk;
  ObjString *name;
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
#endif
