#include "../ht.h"
#include "../types/inference.h"
#include "../types/type.h"
#include "../types/type_ser.h"
#include <string.h>

Type t_int = {T_INT};
Type t_uint64 = {T_UINT64};
Type t_num = {T_NUM};

Type t_char = {T_CHAR};

Type t_string = {T_CONS,
                 {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){&t_char}, 1}},
                 .alias = TYPE_NAME_STRING};

Type t_bool = {T_BOOL};
Type t_void = {T_VOID};
Type t_empty_list = {T_EMPTY_LIST};
Type t_ptr = {T_CONS,
              {.T_CONS = {.name = TYPE_NAME_PTR, .num_args = 0}},
              .alias = TYPE_NAME_PTR};
Type t_none =
    (Type){T_CONS, {.T_CONS = {.name = TYPE_NAME_NONE, .num_args = 0}}};

Type t_builtin_print = MAKE_FN_TYPE_2(&t_string, &t_void);

// Builtin hash table now stores TypeEnv* entries directly.
// This eliminates the need for T_SCHEME wrappers.
static ht builtin_env_ht;

void add_builtin_env(const char *name, TypeEnv *entry) {
  ht_set_hash(&builtin_env_ht, (char *)name, hash_string(name, strlen(name)),
              entry);
}

TypeEnv *lookup_builtin_env(const char *name) {
  return ht_get_hash(&builtin_env_ht, (char *)name,
                     hash_string(name, strlen(name)));
}

// Backward-compatible: callers that need a Type* get it from the env's type
Type *lookup_builtin_type(const char *name) {
  TypeEnv *entry = lookup_builtin_env(name);
  return entry ? entry->type : NULL;
}

// ============================================================================
// Typeclass definitions
// ============================================================================

static TypeClass _GenericArithmetic = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                       .rank = 1000.};
static TypeClass _GenericOrd = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.};
static TypeClass _GenericEq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.};

TypeClass *GenericArithmetic = &_GenericArithmetic;
TypeClass *GenericOrd = &_GenericOrd;
TypeClass *GenericEq = &_GenericEq;

// ============================================================================
// Builtin construction helpers (no T_SCHEME)
// ============================================================================

static TypeList *vlist_of_typevar(Type *t) {
  TypeList *node = t_alloc(sizeof(TypeList));
  node->type = t;
  node->next = NULL;
  return node;
}

static TypeEnv *make_arithmetic_env(const char *id) {
  // (+) : a -> b -> c
  // Predicates: Arithmetic(a), Arithmetic(b), Comparable(c, Arithmetic, [a, b])
  Type *a = tvar("a");
  Type *b = tvar("b");
  Type *c = tvar("c");

  Type *fn_type = type_fn(a, type_fn(b, c));

  // Arithmetic(a), Arithmetic(b)
  Predicate *preds = NULL;
  preds = predicate_append(preds, GenericArithmetic, a);
  preds = predicate_append(preds, GenericArithmetic, b);

  // Comparable(c, Arithmetic, [a, b])
  Type **prom_args = t_alloc(sizeof(Type *) * 3);
  prom_args[0] = a;
  prom_args[1] = b;
  prom_args[2] = NULL;
  preds = predicate_append_comparable(preds, GenericArithmetic, c, prom_args);

  // Scheme vars: a, b, c
  TypeList *tl_a = vlist_of_typevar(a);
  TypeList *tl_b = vlist_of_typevar(b);
  TypeList *tl_c = vlist_of_typevar(c);
  tl_a->next = tl_b;
  tl_b->next = tl_c;

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = id,
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = preds,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_eq_env(const char *id) {
  // (==) : a -> b -> Bool
  // Predicates: Eq(a), Eq(b), Comparable(w, Eq, [a, b])
  Type *a = tvar("a");
  Type *b = tvar("b");
  Type *w = tvar("w");

  Type *fn_type = type_fn(a, type_fn(b, &t_bool));

  Predicate *preds = NULL;
  preds = predicate_append(preds, GenericEq, a);
  preds = predicate_append(preds, GenericEq, b);

  Type **prom_args = t_alloc(sizeof(Type *) * 3);
  prom_args[0] = a;
  prom_args[1] = b;
  prom_args[2] = NULL;
  preds = predicate_append_comparable(preds, GenericEq, w, prom_args);

  TypeList *tl_a = vlist_of_typevar(a);
  TypeList *tl_b = vlist_of_typevar(b);
  TypeList *tl_w = vlist_of_typevar(w);
  tl_a->next = tl_b;
  tl_b->next = tl_w;

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = id,
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = preds,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_ord_env(const char *id) {
  // (<) : a -> b -> Bool
  // Predicates: Ord(a), Ord(b), Comparable(w, Ord, [a, b])
  Type *a = tvar("a");
  Type *b = tvar("b");
  Type *w = tvar("w");

  Type *fn_type = type_fn(a, type_fn(b, &t_bool));

  Predicate *preds = NULL;
  preds = predicate_append(preds, GenericOrd, a);
  preds = predicate_append(preds, GenericOrd, b);

  Type **prom_args = t_alloc(sizeof(Type *) * 3);
  prom_args[0] = a;
  prom_args[1] = b;
  prom_args[2] = NULL;
  preds = predicate_append_comparable(preds, GenericOrd, w, prom_args);

  TypeList *tl_a = vlist_of_typevar(a);
  TypeList *tl_b = vlist_of_typevar(b);
  TypeList *tl_w = vlist_of_typevar(w);
  tl_a->next = tl_b;
  tl_b->next = tl_w;

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = id,
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = preds,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_str_env(void) {
  // str : a -> String
  Type *a = tvar("a");
  Type *fn_type = type_fn(a, &t_string);

  TypeList *tl_a = vlist_of_typevar(a);

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = "str",
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = NULL,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_option_env(const char *name) {
  Type *a = tvar("a");
  Type *opt = create_option_type(a);

  TypeList *tl_a = vlist_of_typevar(a);

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = name,
                     .type = opt,
                     .scheme_vars = tl_a,
                     .predicates = NULL,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_some_env(void) {
  Type *a = tvar("a");
  Type *opt = create_option_type(a);
  Type *fn_type = type_fn(a, opt);

  TypeList *tl_a = vlist_of_typevar(a);

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = TYPE_NAME_SOME,
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = NULL,
                     .next = NULL};
  return entry;
}

static TypeEnv *make_none_env(void) {
  return make_option_env(TYPE_NAME_NONE);
}
static TypeEnv *make_monomorphic_env(const char *name, Type *type) {
  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry =
      (TypeEnv){.name = name, .type = type, .next = NULL, .predicates = NULL};
  return entry;
}

static TypeEnv *make_id_env(void) {
  // id : a -> a
  Type *a = tvar("a");
  Type *fn_type = type_fn(a, a);

  TypeList *tl_a = vlist_of_typevar(a);

  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = "id",
                     .type = fn_type,
                     .scheme_vars = tl_a,
                     .predicates = NULL,
                     .next = NULL};
  return entry;
}

// ============================================================================
// Initialization
// ============================================================================

void initialize_builtin_types() {
  ht_init(&builtin_env_ht);

  // Typeclass ranks: Int = 0.0, Uint64 = 1.0, Double = 2.0
  static TypeClass tc_int_arith = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                   .rank = 0.0};
  static TypeClass tc_int_ord = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 0.0};
  static TypeClass tc_int_eq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 0.0};

  static TypeClass tc_uint64_arith = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                      .rank = 1.0};
  static TypeClass tc_uint64_ord = {.name = TYPE_NAME_TYPECLASS_ORD,
                                    .rank = 1.0};
  static TypeClass tc_uint64_eq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1.0};

  static TypeClass tc_num_arith = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                   .rank = 2.0};
  static TypeClass tc_num_ord = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 2.0};
  static TypeClass tc_num_eq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 2.0};

  // Attach typeclasses to primitive types
  typeclasses_extend(&t_int, &tc_int_arith);
  typeclasses_extend(&t_int, &tc_int_ord);
  typeclasses_extend(&t_int, &tc_int_eq);

  typeclasses_extend(&t_uint64, &tc_uint64_arith);
  typeclasses_extend(&t_uint64, &tc_uint64_ord);
  typeclasses_extend(&t_uint64, &tc_uint64_eq);

  typeclasses_extend(&t_num, &tc_num_arith);
  typeclasses_extend(&t_num, &tc_num_ord);
  typeclasses_extend(&t_num, &tc_num_eq);

  // Register primitive types as monomorphic builtins (no predicates)
  add_builtin_env(TYPE_NAME_INT, make_monomorphic_env(TYPE_NAME_INT, &t_int));
  add_builtin_env(TYPE_NAME_UINT64,
                  make_monomorphic_env(TYPE_NAME_UINT64, &t_uint64));
  add_builtin_env(TYPE_NAME_DOUBLE,
                  make_monomorphic_env(TYPE_NAME_DOUBLE, &t_num));
  add_builtin_env(TYPE_NAME_CHAR,
                  make_monomorphic_env(TYPE_NAME_CHAR, &t_char));
  add_builtin_env(TYPE_NAME_STRING,
                  make_monomorphic_env(TYPE_NAME_STRING, &t_string));
  add_builtin_env(TYPE_NAME_BOOL,
                  make_monomorphic_env(TYPE_NAME_BOOL, &t_bool));
  add_builtin_env(TYPE_NAME_VOID,
                  make_monomorphic_env(TYPE_NAME_VOID, &t_void));
  add_builtin_env(TYPE_NAME_PTR, make_monomorphic_env(TYPE_NAME_PTR, &t_ptr));

  // Register print: String -> Void (monomorphic)
  add_builtin_env("print", make_monomorphic_env("print", &t_builtin_print));
  add_builtin_env("str", make_str_env());
  add_builtin_env("Option", make_option_env("Option"));
  add_builtin_env("Some", make_some_env());
  add_builtin_env("None", make_none_env());

  // Register id: polymorphic identity function
  add_builtin_env("id", make_id_env());

  // Register arithmetic operators as polymorphic builtins
  add_builtin_env("+", make_arithmetic_env("+"));
  add_builtin_env("-", make_arithmetic_env("-"));
  add_builtin_env("*", make_arithmetic_env("*"));
  add_builtin_env("/", make_arithmetic_env("/"));
  add_builtin_env("%", make_arithmetic_env("%"));

  add_builtin_env("==", make_eq_env("=="));
  add_builtin_env("!=", make_eq_env("!="));
  add_builtin_env("<", make_ord_env("<"));
  add_builtin_env("<=", make_ord_env("<="));
  add_builtin_env(">", make_ord_env(">"));
  add_builtin_env(">=", make_ord_env(">="));
}
