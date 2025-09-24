#ifndef _LANG_TYPE_BUILTINS_H
#define _LANG_TYPE_BUILTINS_H
#include "ht.h"
#include "type.h"
extern ht builtin_types;

extern Type t_int;
extern Type t_uint64;
extern Type t_num;
extern Type t_string;
extern Type t_char_array;
extern Type t_string_add_fn_sig;
extern Type t_string_array;
extern Type t_bool;
extern Type t_void;
extern Type t_char;
extern Type t_ptr;
extern Type t_none;

extern Type t_builtin_print;

void initialize_builtin_types();
void add_builtin(char *name, Type *t);

void print_builtin_types();

Type *lookup_builtin_type(const char *name);

#endif
