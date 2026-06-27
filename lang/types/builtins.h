#ifndef _LANG_TYPE_BUILTINS_H
#define _LANG_TYPE_BUILTINS_H
#include "../ht.h"
#include "./inference.h"
#include "type.h"

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

typedef struct BuiltinEnvRefs {
  TypeEnv *print;
  TypeEnv *str;
  TypeEnv *some;
  TypeEnv *list_concat;
  TypeEnv *array_at;
  TypeEnv *array_size;
  TypeEnv *array_set;
  TypeEnv *array_fill_const;
  TypeEnv *array_fill;
  TypeEnv *array_range;
  TypeEnv *array_succ;
  TypeEnv *array_offset;
  TypeEnv *list_prepend;
  TypeEnv *cstr;
  TypeEnv *sizeof_env;
  TypeEnv *cor_map;
  TypeEnv *cor_loop;
  TypeEnv *cor_zip;
  TypeEnv *cor_zip_struct;
  TypeEnv *cor_current;
  TypeEnv *cor_try_opt;
  TypeEnv *iter;
  TypeEnv *play_routine;
  TypeEnv *play_routine_quant;
  TypeEnv *dlopen_env;
  TypeEnv *is_null;
  TypeEnv *asbytes;
  TypeEnv *typeof_env;
  TypeEnv *arith_add;
  TypeEnv *arith_sub;
  TypeEnv *arith_mul;
  TypeEnv *arith_div;
  TypeEnv *arith_mod;
  TypeEnv *eq;
  TypeEnv *neq;
  TypeEnv *lt;
  TypeEnv *lte;
  TypeEnv *gt;
  TypeEnv *gte;
  TypeEnv *logical_and;
  TypeEnv *logical_or;
} BuiltinEnvRefs;

extern BuiltinEnvRefs builtin_envs;

void initialize_builtin_types();

// Deprecated: old API returning Type* (uses T_SCHEME internally).
// Prefer lookup_builtin_env() for the new predicate-aware type system.
Type *lookup_builtin_type(const char *name);

// New API: builtins stored as TypeEnv entries with predicates.
TypeEnv *lookup_builtin_env(const char *name);

// Convenience: expose the generic typeclasses for external use.
extern TypeClass *GenericArithmetic;
extern TypeClass *GenericOrd;
extern TypeClass *GenericEq;
extern TypeClass *GenericFrom;

// Kept for backward compat with existing callers — will be removed.
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
extern Type array_offset_scheme;
extern Type cstr_scheme;

extern Type opt_scheme;
extern Type array_scheme;
extern Type list_scheme;
extern Type list_concat_scheme;
extern Type list_prepend_scheme;
extern Type str_fmt_scheme;

extern Type logical_op_scheme;

extern Type cor_map_scheme;
extern Type cor_filter_scheme;
extern Type cor_stop_scheme;
extern Type cor_loop_scheme;
extern Type cor_take_scheme;
// extern Type loop_cor_scheme;
extern Type cor_combine_scheme;
extern Type cor_try_opt_scheme;
extern Type play_routine_scheme;
extern Type play_routine_quant_scheme;

extern Type cor_scheme;
extern Type cor_current_scheme;
extern Type iter_of_list_scheme;
extern Type iter_cor_list_scheme;
extern Type iter_of_array_scheme;
extern Type use_or_finish_scheme;
extern Type dlopen_type;

extern Type sizeof_scheme;
extern Type asbytes_scheme;

extern Type typeof_scheme;
extern Type cor_zip_scheme;
extern Type is_null_type;
void print_builtin_types(void);
#endif
