#ifndef _LANG_TYPE_BUILTINS_H
#define _LANG_TYPE_BUILTINS_H
#include "../ht.h"
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

extern Type arithmetic_scheme;
extern Type ord_scheme;
extern Type eq_scheme;
extern Type id_scheme;

extern Type array_id_scheme;
extern Type array_size_scheme;
extern Type array_range_scheme;
extern Type array_at_scheme;
extern Type array_set_scheme;
extern Type array_fill_const_scheme;
extern Type array_fill_scheme;
extern Type cstr_scheme;

extern Type opt_scheme;
extern Type array_scheme;
extern Type list_scheme;
extern Type list_concat_scheme;
extern Type list_prepend_scheme;
extern Type str_fmt_scheme;

extern Type logical_op_scheme;

extern Type cor_map_scheme;
extern Type cor_stop_scheme;
extern Type cor_loop_scheme;
extern Type play_routine_scheme;

extern Type cor_scheme;
extern Type iter_of_list_scheme;
extern Type iter_of_array_scheme;
extern Type use_or_finish_scheme;

#endif
