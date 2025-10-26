#ifndef _LANG_TYPE_SER_H
#define _LANG_TYPE_SER_H

#include "./inference.h"
#include "./type.h"
void print_type(Type *t);
void print_type_err(Type *t);
char *type_to_string(Type *t, char *buffer);

void print_type_env(TypeEnv *env);

void print_type_to_stream(Type *t, FILE *stream);
#endif
