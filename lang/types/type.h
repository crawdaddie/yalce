#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H
#include "../parse.h"
#include <stdbool.h>
#include <unistd.h>
#define _TSTORAGE_SIZE_DEFAULT 2000000

typedef struct Type Type;

void reset_type_var_counter();
Type *next_tvar();

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
  Type *module;
  struct TypeClass *next;
} TypeClass;

typedef struct Type Type;

bool is_string_type(Type *type);

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
#define TYPE_NAME_VARIANT "Sum"
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

#define TYPE_NAME_TYPECLASS_ARITHMETIC "Arithmetic"
#define TYPE_NAME_TYPECLASS_EQ "Eq"
#define TYPE_NAME_TYPECLASS_ORD "Ord"
#define TYPE_NAME_RUN_IN_SCHEDULER "run_in_scheduler"
#define TYPE_NAME_COROUTINE_CONSTRUCTOR "CoroutineConstructor"
#define TYPE_NAME_COROUTINE_INSTANCE "Coroutine"



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
  T_SCHEME,
};
#define TYPE_FLAGS_PRIMITIVE ((1 << T_STRING) | ((1 << T_STRING) - 1))

typedef struct TypeList {
  Type *type;
  struct TypeList *next;
} TypeList;

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
      TypeList *vars;
      int num_vars;
      Type *type;
    } T_SCHEME;

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
  struct Type *closure_meta;
  // bool is_recursive_placeholder;
  int scope;
  int yield_boundary;
  void *meta;
} Type;




bool variant_contains_type(Type *variant, Type *member, int *idx);

bool types_equal(Type *l, Type *r);

Type *fn_return_type(Type *);
int fn_type_args_len(Type *);

Type *empty_type();
Type *tvar(const char *name);
bool is_generic(Type *t);

Type *type_fn(Type *from, Type *to);
Type *create_type_multi_param_fn(int len, Type **from, Type *to);

Type *deep_copy_type(const Type *original);

bool is_list_type(Type *type);
bool is_tuple_type(Type *type);
bool is_pointer_type(Type *type);
bool is_closure(Type *type);

bool is_array_type(Type *type);

Type *resolve_tc_rank(Type *type);

Type *replace_in(Type *type, Type *tvar, Type *replacement);
bool is_sum_type(Type *type);

typedef struct VariantContext {
} VariantContext;

Type *create_cons_type(const char *name, int len, Type **unified_args);
Type *create_option_type(Type *option_of);
bool is_option_type(Type *t);

Type *type_of_option(Type *option);

Type *get_builtin_type(const char *id_chars);

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


bool is_simple_enum(Type *t);

bool fn_types_match(Type *t1, Type *t2);


bool application_is_partial(Ast *app);

bool is_coroutine_type(Type *fn_type);

bool is_coroutine_constructor_type(Type *fn_type);

Type *create_coroutine_instance_type(Type *ret_type);

bool is_void_func(Type *f);

void typeclasses_extend(Type *t, TypeClass *tc);

bool is_module(Type *t);

Type *create_tc_resolve(TypeClass *tc, Type *t1, Type *t2);

#define MSCHEME(n, vlist, t) ((Type){T_SCHEME, {.T_SCHEME = {.vars = vlist, .num_vars = n, .type = t}}})

// Recursive macro helpers for building TypeList chains
#define _TYPELIST_1(t1) \
  &(TypeList){.type = (t1), .next = NULL}

#define _TYPELIST_2(t1, t2) \
  &(TypeList){.type = (t1), .next = _TYPELIST_1(t2)}

#define _TYPELIST_3(t1, t2, t3) \
  &(TypeList){.type = (t1), .next = _TYPELIST_2(t2, t3)}

#define _TYPELIST_4(t1, t2, t3, t4) \
  &(TypeList){.type = (t1), .next = _TYPELIST_3(t2, t3, t4)}

#define _TYPELIST_5(t1, t2, t3, t4, t5) \
  &(TypeList){.type = (t1), .next = _TYPELIST_4(t2, t3, t4, t5)}

#define _TYPELIST_6(t1, t2, t3, t4, t5, t6) \
  &(TypeList){.type = (t1), .next = _TYPELIST_5(t2, t3, t4, t5, t6)}

#define _TYPELIST_7(t1, t2, t3, t4, t5, t6, t7) \
  &(TypeList){.type = (t1), .next = _TYPELIST_6(t2, t3, t4, t5, t6, t7)}

#define _TYPELIST_8(t1, t2, t3, t4, t5, t6, t7, t8) \
  &(TypeList){.type = (t1), .next = _TYPELIST_7(t2, t3, t4, t5, t6, t7, t8)}

// Macro overloading helper
#define _GET_TYPELIST_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define TYPELIST(...) \
  _GET_TYPELIST_MACRO(__VA_ARGS__, _TYPELIST_8, _TYPELIST_7, _TYPELIST_6, _TYPELIST_5, _TYPELIST_4, _TYPELIST_3, _TYPELIST_2, _TYPELIST_1)(__VA_ARGS__)

// Convenience macro for creating T_SCHEME with variadic type arguments
#define TSCHEME(scheme_type, ...) \
  MSCHEME(_GET_ARG_COUNT(__VA_ARGS__), TYPELIST(__VA_ARGS__), (scheme_type))

// Helper to count arguments
#define _GET_ARG_COUNT(...) \
  _GET_ARG_COUNT_HELPER(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

#define _GET_ARG_COUNT_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#endif

