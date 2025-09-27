#include "ht.h"
#include "inference.h"
#include "types/type.h"
#include "types/type_ser.h"
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

Type t_builtin_print;

ht builtin_types;

void add_builtin(char *name, Type *t) {
  ht_set_hash(&builtin_types, name, hash_string(name, strlen(name)), t);
}

void print_builtin_types() {
  printf("builtins:\n");
  hti it = ht_iterator(&builtin_types);
  bool cont = ht_next(&it);
  for (; cont; cont = ht_next(&it)) {
    const char *key = it.key;
    Type *t = it.value;
    printf("%s: ", key);
    print_type(t);
  }
}
Type *create_tc_resolve(TypeClass *tc, Type *t1, Type *t2);
TypeClass GenericArithmetic = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                               .rank = 1000.};
TypeClass GenericOrd = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.};
TypeClass GenericEq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.};

Type _tvar_a = TVAR("a");
Type id_scheme =
    (Type){T_SCHEME,
           {.T_SCHEME = {.vars =
                             &(TypeList){
                                 .type = &_tvar_a,
                             },
                         .num_vars = 1,
                         .type = &MAKE_FN_TYPE_2(&_tvar_a, &_tvar_a)}}};

Type array_id_scheme =
    (Type){T_SCHEME,
           {.T_SCHEME = {.vars =
                             &(TypeList){
                                 .type = &TARRAY(&_tvar_a),
                             },
                         .num_vars = 1,
                         .type = &MAKE_FN_TYPE_2(&TARRAY(&_tvar_a),
                                                 &TARRAY(&_tvar_a))}}};
TypeList vlist_of_typevar(Type *t) {
  return (TypeList){.type = t, .next = NULL};
}

Type arithmetic_scheme;
Type create_arithmetic_scheme() {
  Type *a = tvar("a");
  Type *b = tvar("b");
  typeclasses_extend(a, &GenericArithmetic);
  typeclasses_extend(b, &GenericArithmetic);
  Type *f = create_tc_resolve(&GenericArithmetic, a, b);
  f->implements = &GenericArithmetic;
  f = type_fn(b, f);
  f = type_fn(a, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList) * 2);
  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = f}}};
}

Type ord_scheme;
Type create_ord_scheme() {
  Type *a = tvar("a");
  Type *b = tvar("b");
  typeclasses_extend(a, &GenericOrd);
  typeclasses_extend(b, &GenericOrd);

  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList) * 2);
  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;
  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = f}}};
}

Type eq_scheme;

Type create_eq_scheme() {
  Type *a = tvar("a");
  typeclasses_extend(a, &GenericEq);

  Type *f = &t_bool;
  f = type_fn(a, f);
  f = type_fn(a, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type array_size_scheme;
Type create_array_size_scheme() {

  Type *a = tvar("a");
  Type *arr = create_array_type(a);

  Type *f = type_fn(arr, &t_int);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type array_range_scheme;
Type create_array_range_scheme() {

  Type *a = tvar("a");
  Type *arr = create_array_type(a);

  Type *f = arr;
  f = type_fn(arr, f);
  f = type_fn(&t_int, f);
  f = type_fn(&t_int, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type array_at_scheme;
Type create_array_at_scheme() {

  Type *a = tvar("a");
  Type *arr = create_array_type(a);

  Type *f = a;
  f = type_fn(&t_int, f);
  f = type_fn(arr, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type array_set_scheme;
Type create_array_set_scheme() {

  Type *a = tvar("a");
  Type *arr = create_array_type(a);

  Type *f = arr;
  f = type_fn(a, f);
  f = type_fn(&t_int, f);
  f = type_fn(arr, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}
Type opt_scheme;
Type create_opt_scheme() {
  Type *var = tvar("a");
  Type *full_type = create_option_type(var);
  // full_type = type_fn(var, full_type);

  TypeList *vars = t_alloc(sizeof(TypeList));
  *vars = vlist_of_typevar(var);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars, .type = full_type}}};
}

Type array_scheme;
Type create_array_scheme() {
  Type *var = tvar("a");
  Type *full_type = create_array_type(var);
  // full_type = type_fn(var, full_type);

  TypeList *vars = t_alloc(sizeof(TypeList));
  *vars = vlist_of_typevar(var);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars, .type = full_type}}};
}

Type list_scheme;
Type create_list_scheme() {
  Type *var = tvar("a");
  Type *full_type = create_list_type_of_type(var);
  // full_type = type_fn(var, full_type);

  TypeList *vars = t_alloc(sizeof(TypeList));
  *vars = vlist_of_typevar(var);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars, .type = full_type}}};
}

Type list_concat_scheme;
Type create_list_concat_scheme() {

  Type *a = tvar("a");
  Type *l = create_list_type_of_type(a);

  Type *f = l;
  f = type_fn(l, f);
  f = type_fn(l, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type list_prepend_scheme;
Type create_list_prepend_scheme() {

  Type *a = tvar("a");
  Type *l = create_list_type_of_type(a);

  Type *f = l;
  f = type_fn(l, f);
  f = type_fn(a, f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type str_fmt_scheme;
Type create_str_fmt_scheme() {

  Type *a = tvar("a");

  Type *f = type_fn(a, &t_string);

  TypeList *vars_mem = t_alloc(sizeof(TypeList));

  vars_mem[0] = vlist_of_typevar(a);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 1, .vars = vars_mem, .type = f}}};
}

Type *create_coroutine_instance_type(Type *ret_type) {
  Type *coroutine_fn = type_fn(&t_void, create_option_type(ret_type));
  coroutine_fn->is_coroutine_instance = true;
  Type **ar = t_alloc(sizeof(Type *));
  ar[0] = ret_type;
  return create_cons_type(TYPE_NAME_COROUTINE_INSTANCE, 1, ar);
}

Type cor_map_scheme;
Type create_cor_map_scheme() {
  Type *a = tvar("a");
  Type *b = tvar("b");
  Type *f = type_fn(a, b);
  Type *cmap_f = create_coroutine_instance_type(b);
  cmap_f = type_fn(create_coroutine_instance_type(a), cmap_f);
  cmap_f = type_fn(f, cmap_f);

  TypeList *vars_mem = t_alloc(sizeof(TypeList) * 2);
  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  return (Type){
      T_SCHEME,
      {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = cmap_f}}};
}

Type iter_of_list_scheme;
Type create_iter_of_list_scheme() {
  Type *a = tvar("a");
  TypeList *vars_mem = t_alloc(sizeof(TypeList));
  vars_mem[0] = vlist_of_typevar(a);

  Type *f = create_coroutine_instance_type(a);
  f = type_fn(create_list_type_of_type(a), f);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = f}}};
}

Type iter_of_array_scheme;
Type create_iter_of_array_scheme() {
  Type *a = tvar("a");
  TypeList *vars_mem = t_alloc(sizeof(TypeList));
  vars_mem[0] = vlist_of_typevar(a);

  Type *f = create_coroutine_instance_type(a);
  f = type_fn(create_array_type(a), f);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = f}}};
}

Type use_or_finish_scheme;
Type create_use_or_finish_scheme() {

  Type *a = tvar("a");
  TypeList *vars_mem = t_alloc(sizeof(TypeList));
  vars_mem[0] = vlist_of_typevar(a);

  Type *f = a;
  f = type_fn(create_option_type(a), f);

  return (Type){T_SCHEME,
                {.T_SCHEME = {.num_vars = 2, .vars = vars_mem, .type = f}}};
}

void initialize_builtin_types() {
  ht_init(&builtin_types);
  static TypeClass tc_int[] = {{
                                   .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                   .rank = 0.0,
                               },
                               {
                                   .name = TYPE_NAME_TYPECLASS_ORD,
                                   .rank = 0.0,
                               },
                               {
                                   .name = TYPE_NAME_TYPECLASS_EQ,
                                   .rank = 0.0,
                               }};
  typeclasses_extend(&t_int, tc_int);
  typeclasses_extend(&t_int, tc_int + 1);
  typeclasses_extend(&t_int, tc_int + 2);

  static TypeClass tc_uint64[] = {{
                                      .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                      .rank = 1.0,
                                  },
                                  {
                                      .name = TYPE_NAME_TYPECLASS_ORD,
                                      .rank = 1.0,
                                  },
                                  {
                                      .name = TYPE_NAME_TYPECLASS_EQ,
                                      .rank = 1.0,
                                  }};

  typeclasses_extend(&t_uint64, tc_uint64);
  typeclasses_extend(&t_uint64, tc_uint64 + 1);
  typeclasses_extend(&t_uint64, tc_uint64 + 2);

  static TypeClass tc_num[] = {{

                                   .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                   .rank = 2.0,
                               },
                               {
                                   .name = TYPE_NAME_TYPECLASS_ORD,
                                   .rank = 2.0,
                               },
                               {
                                   .name = TYPE_NAME_TYPECLASS_EQ,
                                   .rank = 2.0,
                               }};

  typeclasses_extend(&t_num, tc_num);
  typeclasses_extend(&t_num, tc_num + 1);
  typeclasses_extend(&t_num, tc_num + 2);

  static TypeClass TCEq_bool = {
      .name = TYPE_NAME_TYPECLASS_EQ,
      .rank = 0.0,
  };

  typeclasses_extend(&t_bool, &TCEq_bool);

  arithmetic_scheme = create_arithmetic_scheme();
  add_builtin("+", &arithmetic_scheme);
  add_builtin("-", &arithmetic_scheme);
  add_builtin("*", &arithmetic_scheme);
  add_builtin("/", &arithmetic_scheme);
  add_builtin("%", &arithmetic_scheme);

  eq_scheme = create_eq_scheme();
  add_builtin("==", &eq_scheme);
  add_builtin("!=", &eq_scheme);

  ord_scheme = create_ord_scheme();

  add_builtin(">", &ord_scheme);
  add_builtin("<", &ord_scheme);
  add_builtin(">=", &ord_scheme);
  add_builtin("<=", &ord_scheme);

  add_builtin("id", &id_scheme);
  add_builtin("print", type_fn(&t_string, &t_void));

  add_builtin(TYPE_NAME_INT, &t_int);
  add_builtin(TYPE_NAME_DOUBLE, &t_num);
  add_builtin(TYPE_NAME_CHAR, &t_char);
  add_builtin(TYPE_NAME_STRING, &t_string);
  add_builtin(TYPE_NAME_BOOL, &t_bool);
  add_builtin(TYPE_NAME_VOID, &t_void);

  add_builtin("&&", type_fn(&t_bool, type_fn(&t_bool, &t_bool)));
  add_builtin("||", type_fn(&t_bool, type_fn(&t_bool, &t_bool)));

  array_size_scheme = create_array_size_scheme();
  add_builtin("array_size", &array_size_scheme);
  add_builtin("array_succ", &array_id_scheme);
  array_range_scheme = create_array_range_scheme();
  add_builtin("array_range", &array_range_scheme);

  array_at_scheme = create_array_at_scheme();
  add_builtin("array_at", &array_at_scheme);

  array_set_scheme = create_array_set_scheme();
  add_builtin("array_set", &array_set_scheme);

  opt_scheme = create_opt_scheme();
  add_builtin("Option", &opt_scheme);
  add_builtin("Some", &opt_scheme);
  add_builtin("None", &opt_scheme);

  add_builtin("Ptr", &t_ptr);

  array_scheme = create_array_scheme();
  add_builtin("Array", &array_scheme);

  list_scheme = create_list_scheme();
  add_builtin("List", &list_scheme);

  list_concat_scheme = create_list_concat_scheme();
  add_builtin("list_concat", &list_concat_scheme);
  list_prepend_scheme = create_list_prepend_scheme();
  add_builtin("::", &list_prepend_scheme);

  str_fmt_scheme = create_str_fmt_scheme();
  add_builtin("str", &str_fmt_scheme);

  cor_map_scheme = create_cor_map_scheme();
  add_builtin("cor_map", &cor_map_scheme);

  iter_of_list_scheme = create_iter_of_list_scheme();
  add_builtin("iter_of_list", &iter_of_list_scheme);

  iter_of_array_scheme = create_iter_of_array_scheme();
  add_builtin("iter_of_array", &iter_of_array_scheme);

  add_builtin("cor_loop", &id_scheme);

  use_or_finish_scheme = create_use_or_finish_scheme();
  add_builtin("use_or_finish", &use_or_finish_scheme);

  // print_builtin_types();
}

Type *lookup_builtin_type(const char *name) {
  Type *t = ht_get_hash(&builtin_types, name, hash_string(name, strlen(name)));
  return t;
}
