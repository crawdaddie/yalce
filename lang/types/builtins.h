#ifndef _LANG_TYPE_BUILTINS_H
#define _LANG_TYPE_BUILTINS_H
#include "ht.h"
#include "type.h"
extern ht builtin_types;
void initialize_builtin_types();
void add_builtin(char *name, Type *t);

void print_builtin_types();

Type *lookup_builtin_type(const char *name);

extern Type t_option_of_var;
extern Type t_cor_map_fn_sig;
extern Type t_cor_wrap_effect_fn_sig;
extern Type t_cor_loop_sig;
extern Type t_cor_play_sig;
extern Type t_iter_of_list_sig;
extern Type t_iter_of_array_sig;
extern Type t_cor_loop_sig;
#endif
