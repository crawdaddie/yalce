#ifndef _LANG_TYPE_SER_H
#define _LANG_TYPE_SER_H

#include "types/inference.h"
#include "types/type.h"
void print_type(Type *t);
void print_type_err(Type *t);
char *type_to_string(Type *t, char *buffer);

void print_type_env(TypeEnv *env);
#endif
