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

Value clock_native(int arg_count, Value *args);
Value square_generator_native(int arg_count, Value *args);
Value out_native(int arg_count, Value *args);
Value print_native(int arg_count, Value *arg);

#define NUM_BINDINGS 4
static Binding bindings[NUM_BINDINGS] = {{"print", print_native},
                                         {"clock", clock_native},
                                         {"sq", square_generator_native},
                                         {"out", out_native}};

#endif
