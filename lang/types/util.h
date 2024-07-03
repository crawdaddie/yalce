#ifndef _LANG_TYPE_UTIL_H
#define _LANG_TYPE_UTIL_H

#include "parse.h"
#include "types/type.h"
#include <stdbool.h>

void print_type(Type *type);
void print_type_scheme(TypeScheme *scheme);
void print_type_env(TypeEnv *env);

bool is_numeric_type(Type *type);
bool is_type_variable(Type *type);
Type *get_general_numeric_type(Type *t1, Type *t2);
Type *builtin_type(Ast *id);
bool types_equal(Type *t1, Type *t2);
#endif
