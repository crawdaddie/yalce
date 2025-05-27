#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H
#include "parse.h"
#include <stdbool.h>
#include <unistd.h>
#define _TSTORAGE_SIZE_DEFAULT 2000000

typedef struct Type Type;

void reset_type_var_counter();
Type *next_tvar();

typedef struct TypeConstraint {
  Type *t1;
  Type *t2;
  struct TypeConstraint *next;
  Ast *src;
} TypeConstraint;

typedef struct Method {
  const char *name;
  void *method;
  size_t size;
  Type *signature;
} Method;

enum TypeClassType { TC_FN, TC_STRUCTURAL };

typedef struct TypeClass {
  const char *name;
  double rank;
  struct TypeClass *next;
} TypeClass;

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
extern Type t_empty_list;
extern Type t_list_var;

extern Type t_list_prepend;
extern Type t_list_concat;

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
extern Type t_array_to_list_fn_sig;
extern Type t_array_at_fn_sig;
extern Type t_array_set_fn_sig;
extern Type t_array_data_ptr_fn_sig;
extern Type t_array_slice_fn_sig;
extern Type t_array_new_fn_sig;
extern Type t_make_ref;

extern Type t_array_of_chars_fn_sig;

extern Type t_ptr_deref_sig;
extern Type t_none;

extern Type t_iter_cor_sig;
extern Type t_cor_map_iter_sig;
extern Type t_coroutine_concat_sig;

extern Type t_cor_iter_of_list_sig;

extern Type t_builtin_or;
extern Type t_builtin_and;

extern Type t_builtin_char_of;

extern Type t_cor_wrap_effect_fn_sig;
extern Type t_cor_play_sig;

extern Type t_opt_map_sig;

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
#define TYPE_NAME_QUEUE   "Queue"
#define TYPE_NAME_MODULE  "Module"

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

#define TYPE_NAME_TYPECLASS_ARITHMETIC "arithmetic"
#define TYPE_NAME_TYPECLASS_EQ "eq"
#define TYPE_NAME_TYPECLASS_ORD "ord"
#define TYPE_NAME_RUN_IN_SCHEDULER "run_in_scheduler"

#define TYPE_NAME_REF "mut"

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
        .implements = &(TypeClass){.name = TYPE_NAME_TYPECLASS_ARITHMETIC,     \
                                   .rank = 1000.},                             \
  }

#define MAKE_TC_RESOLVE_2(tc, a, b)                                            \
  ((Type){T_TYPECLASS_RESOLVE,                                                 \
          {.T_CONS = {.name = tc, .num_args = 2, .args = (Type *[]){a, b}}}})

#define ord_var(n)                                                             \
  (Type) {                                                                     \
    T_VAR, {.T_VAR = n},                                                       \
        .implements =                                                          \
            &(TypeClass){.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.},      \
  }

// #define eq_var(n)                                                              \
//   (Type) {                                                                     \
//     T_VAR, {.T_VAR = n},                                                       \
//         .implements =                                                          \
//             &(TypeClass){.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.},       \
//   }
//
// #define eq_var(n) \
//   (Type) { \
//     T_VAR, {.T_VAR = n}, \
//         .implements = \
//             &(TypeClass){.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.}, \
//   }
//
#define eq_var(n)                                                              \
  (Type) { T_VAR, {.T_VAR = n}, }

#define TCONS(name, num, ...)                                                  \
  ((Type){T_CONS, {.T_CONS = {name, (Type *[]){__VA_ARGS__}, num}}})

#define TLIST(_t)                                                              \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, (Type *[]){_t}, 1}}})
#define TARRAY(_t)                                                             \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){_t}, 1}}})

#define TTUPLE(num, ...)                                                       \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, (Type *[]){__VA_ARGS__}, num}}})

#define TOPT(of) TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, of), &t_none)

typedef Type *(*TypeClassResolver)(struct Type *this, TypeConstraint *env);

// #define TYPECLASS_RESOLVE(tc_name, dep1, dep2, resolver)                       \
//   ((Type){                                                                     \
//       .kind = T_TYPECLASS_RESOLVE,                                             \
//       .data = {.T_TYPECLASS_RESOLVE = {.comparison_tc = tc_name,               \
//                                        .dependencies = (Type *[]){dep1, dep2}, \
//                                        .resolve_dependencies = resolver}}})

#define TVAR(n)                                                                \
  ((Type){                                                                     \
      T_VAR,                                                                   \
      {.T_VAR = n},                                                            \
  })

typedef struct Type *(*CreateNewGenericTypeFn)(void *);
enum TypeKind {
  T_INT = 0,    // Will give 1 << 0
  T_UINT64 = 1, // Will give 1 << 1
  T_NUM = 2,    // Will give 1 << 2
  T_CHAR = 3,
  T_BOOL = 4,
  T_VOID = 5,
  T_STRING = 6,
  T_FN,
  T_CONS,
  T_VAR,
  T_EMPTY_LIST,
  T_TYPECLASS_RESOLVE,
  T_CREATE_NEW_GENERIC,
};
#define TYPE_FLAGS_PRIMITIVE ((1 << T_STRING) | ((1 << T_STRING) - 1))

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
      const char **names;
    } T_CONS;

    struct {
      struct Type *from;
      struct Type *to;
    } T_FN;

    struct {
      struct Type *from;
      struct Type *to;
      struct Type *state;
    } T_COROUTINE_FN;

    struct {
      CreateNewGenericTypeFn fn;
      struct Type *template;
    } T_CREATE_NEW_GENERIC;

  } data;

  const char *alias;
  TypeClass *implements;

  void *constructor;
  // size_t constructor_size;
  bool is_coroutine_instance;
  bool is_coroutine_constructor;
  bool is_recursive_type_ref;
  bool is_ref;
  bool alloced_in_func; // metadata if this type appears as a function return
  // bool is_recursive_placeholder;
  int scope;
  int yield_boundary;
  void *meta;
} Type;

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  int ref_count;
  int is_fn_param;
  int is_recursive_fn_ref;
  int yield_boundary;
  struct TypeEnv *next;
} TypeEnv;

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
Type *rec_env_lookup(TypeEnv *env, Type *var);

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

Type *create_array_type(Type *of);

Type *create_list_type_of_type(Type *of);

Type *create_tuple_type(int len, Type **contained_types);

int get_struct_member_idx(const char *member_name, Type *type);

Type *get_struct_member_type(const char *member_name, Type *type);

Type *concat_struct_types(Type *a, Type *b);

bool is_struct_of_coroutines(Type *fn_type);

TypeClass *get_typeclass_by_name(Type *t, const char *name);
double get_typeclass_rank(Type *t, const char *name);

bool type_implements(Type *t, TypeClass *tc);

bool is_forall_type(Type *type);

extern Type t_builtin_print;

// extern TypeList *arithmetic_tc_registry;
// extern TypeList *ord_tc_registry;
// extern TypeList *eq_tc_registry;
//
bool is_simple_enum(Type *t);

bool fn_types_match(Type *t1, Type *t2);

Type *resolve_tc_rank_in_env(Type *type, TypeEnv *env);

Type *resolve_type_in_env(Type *r, TypeEnv *env);

bool application_is_partial(Ast *app);

bool is_coroutine_type(Type *fn_type);

bool is_coroutine_constructor_type(Type *fn_type);

Type *create_coroutine_instance_type(Type *ret_type);

bool is_void_func(Type *f);

void typeclasses_extend(Type *t, TypeClass *tc);

bool is_module(Type *t);

typedef struct TICtx {
  TypeEnv *env;
  TypeConstraint *constraints;
  Ast *current_fn_ast;
  Type *yielded_type;
  int scope;
  int current_fn_scope;
  const char *err;
  FILE *err_stream; // Replace const char *err
} TICtx;

// Substitution map for type variables
typedef struct Substitution {
  Type *from; // Type variable
  Type *to;   // Replacement type
  struct Substitution *next;
} Substitution;
#endif
