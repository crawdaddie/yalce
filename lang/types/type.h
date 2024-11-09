#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H
#include "typeclass.h"
#include <stdbool.h>

#define _TSTORAGE_SIZE_DEFAULT 2000000

typedef struct TypeEnv TypeEnv;
typedef struct Type Type;

extern Type t_int;
extern Type t_uint64;
extern Type t_num;
extern Type t_string;
extern Type t_char_array;
extern Type t_string_add_fn_sig;
extern Type t_string_array;

bool is_string_type(Type *type);

extern Type t_bool;
extern Type t_void;
extern Type t_char;
extern Type t_ptr;

// builtin binop types
extern Type t_add;
extern Type t_sub;
extern Type t_mul;
extern Type t_div;
extern Type t_mod;
extern Type t_gt;
extern Type t_lt;
extern Type t_gte;
extern Type t_lte;
extern Type t_eq;
extern Type t_neq;
extern Type t_bool_binop;

extern Type t_array_var_el;
extern Type t_array_var;

extern Type t_array_size_fn_sig;
extern Type t_array_incr_fn_sig;
extern Type t_array_to_list_fn_sig;
extern Type t_array_at_fn_sig;
extern Type t_array_data_ptr_fn_sig;
extern Type t_array_slice_fn_sig;
extern Type t_array_new_fn_sig;

extern Type t_array_of_chars_fn_sig;

extern Type t_ptr_deref_sig;
extern Type t_option_of_var;

extern Type t_iter_of_list_sig;
extern Type t_iter_of_array_sig;
// clang-format off
#define TYPE_NAME_LIST    "List"
#define TYPE_NAME_ARRAY   "Array"
#define TYPE_NAME_TUPLE   "Tuple"
#define TYPE_NAME_PTR     "Ptr"
#define TYPE_NAME_CHAR    "Char"
#define TYPE_NAME_STRING  "String"
#define TYPE_NAME_BOOL    "Bool"
#define TYPE_NAME_INT     "Int"
#define TYPE_NAME_DOUBLE  "Double"
#define TYPE_NAME_UINT64  "Uint64"
#define TYPE_NAME_VOID    "()"
#define TYPE_NAME_VARIANT "Variant"
#define TYPE_NAME_SOME    "Some"
#define TYPE_NAME_NONE    "None"

#define TYPE_NAME_OP_ADD  "+"
#define TYPE_NAME_OP_SUB  "-"
#define TYPE_NAME_OP_MUL  "*"
#define TYPE_NAME_OP_DIV  "/"
#define TYPE_NAME_OP_MOD  "%"
#define TYPE_NAME_OP_LT  "<"
#define TYPE_NAME_OP_GT  ">"
#define TYPE_NAME_OP_LTE  "<="
#define TYPE_NAME_OP_GTE  ">="
#define TYPE_NAME_OP_EQ  "=="
#define TYPE_NAME_OP_NEQ  "!="
#define TYPE_NAME_OP_LIST_PREPEND  "::"
#define TYPE_NAME_OP_AND  "&&"
#define TYPE_NAME_OP_OR  "||"

typedef struct _binop_map {
  const char *name;
  Type *binop_fn_type;
} _binop_map;

#define _NUM_BINOPS 12
extern _binop_map binop_map[];
// clang-format on
//

#define MAKE_FN_TYPE_2(arg_type, ret_type)                                     \
  ((Type){T_FN, {.T_FN = {.from = arg_type, .to = ret_type}}})

#define MAKE_FN_TYPE_3(arg1_type, arg2_type, ret_type)                         \
  ((Type){T_FN,                                                                \
          {.T_FN = {.from = arg1_type,                                         \
                    .to = &MAKE_FN_TYPE_2(arg2_type, ret_type)}}})

#define MAKE_FN_TYPE_4(arg1_type, arg2_type, arg3_type, ret_type)              \
  ((Type){T_FN,                                                                \
          {.T_FN = {.from = arg1_type,                                         \
                    .to = &MAKE_FN_TYPE_3(arg2_type, arg3_type, ret_type)}}})

#define MAKE_FN_TYPE_5(arg1_type, arg2_type, arg3_type, arg4_type, ret_type)   \
  ((Type){T_FN,                                                                \
          {.T_FN = {.from = arg1_type,                                         \
                    .to = &MAKE_FN_TYPE_4(arg2_type, arg3_type, arg4_type,     \
                                          ret_type)}}})
#define arithmetic_var(n)                                                      \
  (Type) {                                                                     \
    T_VAR, {.T_VAR = n},                                                       \
        .implements = (TypeClass *[]){&(TypeClass){.name = "arithmetic"}},     \
        .num_implements = 1                                                    \
  }

#define ord_var(n)                                                             \
  (Type) {                                                                     \
    T_VAR, {.T_VAR = n},                                                       \
        .implements = (TypeClass *[]){&(TypeClass){.name = "ord"}},            \
        .num_implements = 1                                                    \
  }

#define eq_var(n)                                                              \
  (Type) {                                                                     \
    T_VAR, {.T_VAR = n},                                                       \
        .implements = (TypeClass *[]){&(TypeClass){.name = "eq"}},             \
        .num_implements = 1                                                    \
  }

#define TYPECLASS_RESOLVE(tc_name, dep1, dep2)                                 \
  ((Type){.kind = T_TYPECLASS_RESOLVE,                                         \
          .data = {.T_TYPECLASS_RESOLVE = {.comparison_tc = tc_name,           \
                                           .dependencies =                     \
                                               (Type *[]){dep1, dep2}}}})
enum TypeKind {
  /* Type Operator */
  T_INT,
  T_UINT64,
  T_NUM,
  T_CHAR,
  T_BOOL,
  T_VOID,
  T_STRING,
  T_FN,
  T_CONS,
  /* Type Variable  */
  T_VAR,
  // T_MODULE,
  // T_TYPECLASS,
  T_TYPECLASS_RESOLVE,
  // T_VARIANT_MEMBER,
  T_COROUTINE_INSTANCE,
};

typedef struct Type {
  enum TypeKind kind;
  union {
    // Type Variables (T_VAR):
    // They represent unknown types that can be unified with other types during
    // inference.
    const char *T_VAR;
    struct {
      const char *name;
      struct Type **args;
      int num_args;
    } T_CONS;

    struct {
      struct Type *from;
      struct Type *to;
    } T_FN;
    struct {
      struct Type **dependencies; // contains 2
      const char *comparison_tc; // use the comparison typeclass name to compare
                                 // the rank of all dependencies
    } T_TYPECLASS_RESOLVE;
    struct {
      struct Type *params_type; // internal parameter type
      struct Type *yield_interface;
      /* interface for interaction from outside
ie:
```
let f = fn a -> yield a; yield 2; yield 3;;
let x = f 1;
x () # : Some 1
```
yield interface here is () -> Option of Int so we know that x () has type
Option Of Int

for compilation purposes we want to know what's the internal parameter type of x
which in this case would be t_int
this is necessary so that we can properly compile higher-order stream-combining
functions
      */
    } T_COROUTINE_INSTANCE;

    // struct {
    //   struct Type *variant; // pointer to T_CONS with name "Variant"
    //   int index;
    // } T_VARIANT_MEMBER;

  } data;

  const char *alias;
  TypeClass **implements; // Array of type classes this type implements
  int num_implements;
  void *constructor;
  size_t constructor_size;
  bool is_recursive_fn_ref;
  bool is_coroutine_fn;
  bool is_coroutine_instance;
  void *meta;
} Type;

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  struct TypeEnv *next;
} TypeEnv;

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
Type *env_lookup(TypeEnv *env, const char *name);

Type *variant_member_lookup(TypeEnv *env, const char *name, int *idx,
                            char **variant_name);

bool variant_contains_type(Type *variant, Type *member, int *idx);
void free_type_env(TypeEnv *env);
void print_type_env(TypeEnv *env);
Type *find_type_in_env(TypeEnv *env, const char *name);

char *type_to_string(Type *t, char *buffer);

void print_type(Type *t);
void print_type_err(Type *t);
bool types_equal(Type *l, Type *r);

Type *fn_return_type(Type *);
int fn_type_args_len(Type *);

void *talloc(size_t size);
void tfree(void *mem);
Type *empty_type();
Type *tvar(const char *name);
bool is_generic(Type *t);

Type *type_fn(Type *from, Type *to);
Type *create_type_multi_param_fn(int len, Type **from, Type *to);

Type *deep_copy_type(const Type *original);

bool is_list_type(Type *type);
bool is_tuple_type(Type *type);
bool is_pointer_type(Type *type);

bool is_array_type(Type *type);

Type *copy_type(Type *t);

Type *create_typeclass_resolve_type(const char *comparison_tc, Type *dep1,
                                    Type *dep2);

Type *resolve_tc_rank(Type *type);

Type *replace_in(Type *type, Type *tvar, Type *replacement);
Type *resolve_generic_type(Type *t, TypeEnv *env);
bool is_variant_type(Type *type);
Type *variant_lookup(TypeEnv *env, Type *member, int *member_idx);
Type *variant_lookup_name(TypeEnv *env, const char *name, int *member_idx);

typedef struct VariantContext {
} VariantContext;

Type *create_cons_type(const char *name, int len, Type **unified_args);
Type *create_option_type(Type *option_of);
bool is_option_type(Type *t);

Type *type_of_option(Type *option);

Type *get_builtin_type(const char *id_chars);

typedef struct TypeMap {
  Type *key;
  Type *val;
  struct TypeMap *next;
} TypeMap;

TypeMap *constraints_map_extend(TypeMap *map, Type *key, Type *val);
void print_constraints_map(TypeMap *map);
Type *constraints_map_lookup(TypeMap *map, Type *key);

Type *ptr_of_type(Type *);

int *array_type_size_ptr(Type *t);

Type *create_array_type(Type *of, int size);

Type *create_tuple_type(int len, Type **contained_types);

Type *create_coroutine_instance_type(Type *param, Type *ret_type);

bool is_coroutine_instance_type(Type *inst);

bool is_coroutine_generator_fn(Type *gen);
Type *coroutine_instance_fn_def_type(Type *inst);
#endif
