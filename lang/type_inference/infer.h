#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "type_inference/type.h"

Type *infer(Env *env, Ast *e, NonGeneric *nongeneric);

Env *new_env();

void print_type(Type *ty);

Type *type_var();

#endif
