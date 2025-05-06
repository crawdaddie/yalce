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
extern Type t_builtin_cstr;
extern Type t_run_in_scheduler_sig;
extern Type t_array_fill_sig;
extern Type t_array_fill_const_sig;
extern Type t_array_identity_sig;
extern Type t_array_view_sig;
extern Type t_struct_set_sig;

extern Type t_list_tail_sig;

extern Type t_list_ref_set_sig;

extern Type t_play_routine_sig;

extern Type t_cor_replace_fn_sig;
extern Type t_cor_stop_fn_sig;
extern Type t_cor_loop_sig;

extern Type t_iter_of_list_sig;
extern Type t_iter_of_array_sig;
extern Type t_cor_loop_sig;

extern Type t_empty_cor;

extern Type t_fst_sig;
extern Type t_df_offset_sig;

extern Type t_df_raw_fields_sig;
extern Type t_array_range_sig;
extern Type t_array_offset_sig;
extern Type t_array_size;
#endif
