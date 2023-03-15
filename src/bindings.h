#ifndef _BINDINGS_H
#define _BINDINGS_H
#include "lang/obj.h"
#include "lang/obj_function.h"
#include "lang/value.h"

typedef Value (*BoundFunction)(int arg_count, Value *args);

typedef struct {
  const char *name;
  BoundFunction function;
} Binding;

void bindings_setup();

#endif
