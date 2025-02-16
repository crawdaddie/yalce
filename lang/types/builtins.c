#include "ht.h"
#include "inference.h"
#include "types/type.h"
#include <string.h>

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
TypeClass GenericArithmetic = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                               .rank = 1000.};

TypeClass GenericOrd = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.};
TypeClass GenericEq = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.};

Type *create_new_arithmetic_sig(void *i) {
  Type *a = next_tvar();
  typeclasses_extend(a, &GenericArithmetic);
  Type *b = next_tvar();
  typeclasses_extend(b, &GenericArithmetic);
  Type *arith_res = talloc(sizeof(Type));

  Type **args = talloc(sizeof(Type *) * 2);
  args[0] = a;
  args[1] = b;
  *arith_res =
      ((Type){T_TYPECLASS_RESOLVE,
              {.T_CONS = {.name = "arithmetic", .num_args = 2, .args = args}}});
  Type *f = arith_res;
  f = type_fn(b, f);
  f = type_fn(a, f);

  return f;
}

Type *create_new_ord_sig(void *i) {
  Type *a = next_tvar();
  typeclasses_extend(a, &GenericOrd);
  Type *b = next_tvar();
  typeclasses_extend(b, &GenericOrd);

  Type **args = talloc(sizeof(Type *) * 2);
  args[0] = a;
  args[1] = b;
  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  return f;
}

Type *create_new_eq_sig(void *i) {
  Type *a = next_tvar();
  typeclasses_extend(a, &GenericEq);
  Type *b = next_tvar();
  typeclasses_extend(b, &GenericEq);

  Type **args = talloc(sizeof(Type *) * 1);
  args[0] = a;
  args[1] = b;
  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  return f;
}

Type t_arithmetic_fn_sig = {
    T_CREATE_NEW_GENERIC, {.T_CREATE_NEW_GENERIC = create_new_arithmetic_sig}};

Type t_ord_fn_sig = {T_CREATE_NEW_GENERIC,
                     {.T_CREATE_NEW_GENERIC = create_new_ord_sig}};

Type t_eq_fn_sig = {T_CREATE_NEW_GENERIC,
                    {.T_CREATE_NEW_GENERIC = create_new_eq_sig}};

void initialize_builtin_types() {

  ht_init(&builtin_types);
  add_builtin("+", &t_arithmetic_fn_sig);
  add_builtin("-", &t_arithmetic_fn_sig);
  add_builtin("*", &t_arithmetic_fn_sig);
  add_builtin("/", &t_arithmetic_fn_sig);
  add_builtin("%", &t_arithmetic_fn_sig);

  add_builtin(">", &t_ord_fn_sig);
  add_builtin("<", &t_ord_fn_sig);
  add_builtin(">=", &t_ord_fn_sig);
  add_builtin("<=", &t_ord_fn_sig);
  add_builtin("==", &t_eq_fn_sig);
  add_builtin("!=", &t_eq_fn_sig);

  t_option_of_var.alias = "Option";
  add_builtin("Option", &t_option_of_var);
  add_builtin("Some", &t_option_of_var);
  add_builtin("None", &t_option_of_var);
  add_builtin(TYPE_NAME_INT, &t_int);
  add_builtin(TYPE_NAME_DOUBLE, &t_num);
  add_builtin(TYPE_NAME_UINT64, &t_uint64);

  add_builtin(TYPE_NAME_BOOL, &t_bool);

  add_builtin(TYPE_NAME_STRING, &t_string);

  add_builtin(TYPE_NAME_CHAR, &t_char);
  add_builtin(TYPE_NAME_PTR, &t_ptr);

  // add_builtin("Ref", &t_make_ref);

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
  // arithmetic_tc_registry = type_list_extend(arithmetic_tc_registry, &t_int);
  // ord_tc_registry = type_list_extend(ord_tc_registry, &t_int);
  // eq_tc_registry = type_list_extend(eq_tc_registry, &t_int);

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
  // arithmetic_tc_registry = type_list_extend(arithmetic_tc_registry,
  // &t_uint64); ord_tc_registry = type_list_extend(ord_tc_registry, &t_uint64);
  // eq_tc_registry = type_list_extend(eq_tc_registry, &t_uint64);

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
  add_builtin("print", &t_builtin_print);
  add_builtin("array_at", &t_array_at_fn_sig);
  add_builtin("array_set", &t_array_set_fn_sig);
  add_builtin("array_size", &t_array_size_fn_sig);

  add_builtin("||", &t_builtin_or);
  add_builtin("&&", &t_builtin_and);
  add_builtin("cor_wrap_effect", &t_cor_wrap_effect_fn_sig);
  add_builtin("cor_map", &t_cor_map_fn_sig);
  add_builtin("iter_of_list", &t_iter_of_list_sig);
  add_builtin("iter_of_array", &t_iter_of_array_sig);
  add_builtin("cor_loop", &t_cor_loop_sig);
  add_builtin("cor_play", &t_cor_play_sig);
  add_builtin("list_concat", &t_list_concat);
  add_builtin("::", &t_list_prepend);
  add_builtin("queue_of_list", &t_queue_of_list);
  add_builtin("queue_pop_left", &t_queue_pop_left);
  add_builtin("queue_append_right", &t_queue_append_right);
  add_builtin("opt_map", &t_opt_map_sig);
}

Type *lookup_builtin_type(const char *name) {
  Type *t = ht_get_hash(&builtin_types, name, hash_string(name, strlen(name)));
  return t;
}
