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

#endif
