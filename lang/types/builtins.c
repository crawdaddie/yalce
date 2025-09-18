#include "ht.h"
#include "inference.h"
#include "types/type.h"
#include <string.h>

Scheme void_scheme;
Scheme array_at_scheme;

Scheme arithmetic_scheme;
Scheme eq_scheme;
Scheme ord_scheme;

Scheme opt_scheme;

Scheme list_prepend_scheme;
Scheme list_of_scheme;
Scheme array_of_scheme;
Scheme array_set_scheme;
Scheme array_size_scheme;
Type t_builtin_print;

Scheme array_of_scheme;
Scheme array_set_scheme;
Scheme array_size_scheme;
Scheme list_concat_scheme;
Scheme cor_map_scheme;
Scheme iter_of_array_scheme;
Scheme iter_of_list_scheme;
Scheme id_scheme;
Scheme use_or_finish_scheme;

Scheme bool_or_scheme;
Scheme bool_and_scheme;

TypeClass GenericArithmetic = {.name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                               .rank = 1000.};

TypeClass GenericOrd = {.name = TYPE_NAME_TYPECLASS_ORD, .rank = 1000.};
TypeClass GenericEq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.};

static Type array_el = TVAR("a");
static Type arrt = TCONS(TYPE_NAME_ARRAY, 1, &TVAR("a"));
static VarList array_at_scheme_varlist = {.var = "a", .next = NULL};
static Type array_at_fn = MAKE_FN_TYPE_3(&arrt, &t_int, &array_el);
Scheme array_at_scheme_glob = {.vars = &array_at_scheme_varlist,
                               .type = &array_at_fn};

Scheme void_scheme = {.vars = NULL, .type = &t_void};

Scheme *create_new_array_set_sig() {
  Type *el = tvar("a");
  Type *arr = create_array_type(el);

  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = el->data.T_VAR, .next = NULL};

  Scheme *scheme = talloc(sizeof(Scheme));
  Type *f = arr;
  f = type_fn(el, f);
  f = type_fn(&t_int, f);
  f = type_fn(arr, f);
  *scheme = (Scheme){.vars = vars, .type = f};
  return scheme;
}

Scheme *create_new_array_size_sig() {
  Type *el = tvar("a");
  Type *arr = create_array_type(el);
  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = el->data.T_VAR, .next = NULL};
  Type *f = type_fn(arr, &t_int);
  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = vars, .type = f};
  return scheme;
}

Scheme *create_new_array_range_sig() {
  Type *el = tvar("a");
  Type *arr = create_array_type(el);
  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = el->data.T_VAR, .next = NULL};

  Type *f = type_fn(arr, arr);
  f = type_fn(&t_int, f);
  f = type_fn(&t_int, f);
  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = vars, .type = f};
  return scheme;
}

Scheme *create_list_concat_scheme() {
  Type *el = tvar("a");
  Type *l = create_list_type_of_type(el);
  Type *f = l;
  f = type_fn(l, f);
  f = type_fn(l, f);
  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = el->data.T_VAR, .next = NULL};
  Scheme *sch = talloc(sizeof(Scheme));
  *sch = (Scheme){.vars = vars, .type = f};
  return sch;
}

Type *_cor_wrap_effect_fn_sig() {
  Type *t_cor_wrap_ret_type = next_tvar();
  Type *t_cor_wrap_state_type = next_tvar();
  Type *t_cor_wrap = type_fn(&t_void, create_option_type(t_cor_wrap_ret_type));
  t_cor_wrap->is_coroutine_instance = true;
  Type *f = t_cor_wrap;
  f = type_fn(t_cor_wrap, f);
  f = type_fn(type_fn(t_cor_wrap_ret_type, &t_void), f);
  return f;
}

Type *_cor_map_fn_sig() {
  Type *map_from = next_tvar();
  Type *map_to = next_tvar();

  Type *mapper = type_fn(map_from, map_to);
  Type *cor_from = create_coroutine_instance_type(map_from);
  Type *cor_to = create_coroutine_instance_type(map_to);
  Type *f = cor_to;
  f = type_fn(cor_from, f);
  f = type_fn(mapper, f);

  return f;
}

Type *_array_fill_sig() {
  Type *eltype = next_tvar();
  Type *f = create_array_type(eltype);
  Type *filler_cb = type_fn(&t_int, eltype);
  f = type_fn(filler_cb, f);
  f = type_fn(&t_int, f);
  return f;
}

Type *_array_fill_const_sig() {
  Type *eltype = next_tvar();
  Type *f = create_array_type(eltype);
  Type *m = eltype;
  f = type_fn(m, f);
  f = type_fn(&t_int, f);
  return f;
}

Type *_array_succ_sig() {
  Type *eltype = next_tvar();
  Type *f = create_array_type(eltype);
  return type_fn(f, f);
}

Type *_array_range_sig() {
  Type *eltype = next_tvar();
  Type *f = create_array_type(eltype);
  f = type_fn(f, f);
  f = type_fn(&t_int, f);
  f = type_fn(&t_int, f);

  return f;
}

Type *_array_offset_sig() {
  Type *eltype = next_tvar();

  Type *f = create_array_type(eltype);
  f = type_fn(f, f);
  f = type_fn(&t_int, f);

  return f;
}

Type *_array_view_sig() {
  Type *eltype = next_tvar();
  Type *f = create_array_type(eltype);
  Type *func = type_fn(f, f);
  func = type_fn(&t_int, func);
  func = type_fn(&t_int, func);
  return func;
}

Type *_struct_set_sig() {
  Type *eltype = next_tvar();
  Type *rectype = next_tvar();
  Type *f = type_fn(eltype, &t_void);
  f = type_fn(rectype, f);
  f = type_fn(&t_int, f);
  return f;
}

Type *_cstr_sig() {
  Type *el = next_tvar();
  Type *t_arr = create_array_type(el);
  return type_fn(t_arr, &t_ptr
                 // ptr_of_type(el)
  );
}

Type *_play_routine_sig() {
  Type *v = &t_num;
  // next_tvar();
  Type *cor_from = create_coroutine_instance_type(v);
  // Type *sched_cb_type = &t_void;
  // sched_cb_type = type_fn(&t_int, sched_cb_type);
  // sched_cb_type = type_fn(&t_ptr, sched_cb_type);

  Type *f = cor_from;
  f = type_fn(cor_from, f);
  f = type_fn(&t_ptr, f);
  f = type_fn(&t_uint64, f);
  // f = type_fn(&t_num, f);
  return f;
}

Type *_list_tail_sig() {
  Type *list_el = next_tvar();
  Type *list = create_list_type_of_type(list_el);
  return type_fn(list, list);
}

Type *_list_ref_set_sig() {
  Type *list_el = next_tvar();
  Type *list = create_list_type_of_type(list_el);
  Type *f = &t_void;
  f = type_fn(list, f);
  f = type_fn(list, f);
  return f;
}

Type *_cor_replace_fn_sig() {
  Type *map_from = next_tvar();
  Type *map_to = next_tvar();

  Type *cor_from = create_coroutine_instance_type(map_from);
  Type *cor_to = create_coroutine_instance_type(map_to);
  Type *f = cor_to;
  f = type_fn(cor_to, f);
  f = type_fn(cor_from, f);
  return f;
}

Type *_cor_stop_fn_sig() {
  Type *map_from = next_tvar();
  Type *cor_from = create_coroutine_instance_type(map_from);
  return type_fn(cor_from, cor_from);
}

Type *_cor_promise_fn_sig() {
  Type *map_from = next_tvar();
  Type *cor_from = create_coroutine_instance_type(map_from);
  return type_fn(cor_from, fn_return_type(cor_from));
}

Type *_cor_unwrap_or_end_sig() {
  Type *map_from = next_tvar();
  Type *opt = create_option_type(map_from);
  return type_fn(opt, map_from);
}

Type t_current_cor_fn_sig = {T_FN,
                             .data = {.T_FN = {.from = &t_void, .to = &t_ptr}}};

Type *_iter_of_list_sig() {
  Type *el_type = next_tvar();
  Type *list = create_list_type_of_type(el_type);
  Type *cor = create_coroutine_instance_type(el_type);
  return type_fn(list, cor);
}

Type *_iter_of_array_sig() {
  Type *el_type = next_tvar();
  Type *arr = create_array_type(el_type);
  Type *cor = create_coroutine_instance_type(el_type);
  return type_fn(arr, cor);
}

Type *_cor_loop_sig() {
  Type *el_type = next_tvar();
  Type *cor = create_coroutine_instance_type(el_type);
  return type_fn(cor, cor);
}

Type *_df_offset_sig() {
  Type *t = next_tvar();
  Type *f = t;
  f = type_fn(&t_int, f);
  f = type_fn(t, f);
  return f;
}

ht builtin_schemes;

void add_builtin_scheme(char *name, Scheme *t) {
  ht_set_hash(&builtin_schemes, name, hash_string(name, strlen(name)), t);
}

Type *resolve_tc_type(const char *tc_name, Type *t1, Type *t2) {
  Type **l = talloc(sizeof(Type *) * 2);
  l[0] = t1;
  l[1] = t2;
  Type *tcr = talloc(sizeof(Type));

  *tcr = (Type){T_TYPECLASS_RESOLVE,
                {.T_CONS = {.name = tc_name, .args = l, .num_args = 2}}};
  return tcr;
}

TypeClass GenArithmeticTypeClass = {
    .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
    .rank = 1000.,
};

Type *create_tc_resolve(TypeClass *tc, Type *t1, Type *t2);

VarList vlist_of_typevar(Type *t) {
  return (VarList){
      .var = t->data.T_VAR, .implements = t->implements, .next = NULL};
}

Scheme *create_arithmetic_scheme() {

  Type *a = tvar("a");
  Type *b = tvar("b");
  typeclasses_extend(a, &GenericArithmetic);
  typeclasses_extend(b, &GenericArithmetic);
  Type *f = create_tc_resolve(&GenericArithmetic, a, b);
  f->implements = &GenericArithmetic;
  f = type_fn(b, f);
  f = type_fn(a, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);
  vars_mem[1] = vlist_of_typevar(b);

  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = vars_mem, .type = f};

  return scheme;
}

Scheme _create_arithmetic_scheme() {

  Type *a = tvar("a");
  Type *b = tvar("b");
  typeclasses_extend(a, &GenericArithmetic);
  typeclasses_extend(b, &GenericArithmetic);
  Type *f = create_tc_resolve(&GenericArithmetic, a, b);
  f->implements = &GenericArithmetic;
  f = type_fn(b, f);
  f = type_fn(a, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);
  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  return (Scheme){.vars = vars_mem, .type = f};
}

Scheme *create_eq_scheme() {

  Type *a = tvar("a");
  Type *b = tvar("b");

  typeclasses_extend(a, &GenericEq);
  typeclasses_extend(b, &GenericEq);

  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);

  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = vars_mem, .type = f};

  return scheme;
}

Scheme _create_eq_scheme() {

  Type *a = tvar("a");
  Type *b = tvar("b");

  typeclasses_extend(a, &GenericEq);
  typeclasses_extend(b, &GenericEq);

  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);

  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  return (Scheme){.vars = vars_mem, .type = f};
}

Scheme _create_ord_scheme() {

  Type *a = tvar("a");
  Type *b = tvar("b");

  typeclasses_extend(a, &GenericOrd);
  typeclasses_extend(b, &GenericOrd);

  Type *f = &t_bool;
  f = type_fn(b, f);
  f = type_fn(a, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);

  vars_mem[1] = vlist_of_typevar(b);
  vars_mem[0] = vlist_of_typevar(a);
  vars_mem[0].next = vars_mem + 1;

  return (Scheme){.vars = vars_mem, .type = f};
}

Scheme create_id_scheme() {

  Type *var_a = tvar("a");
  Type *full_type = type_fn(var_a, var_a); // a → a

  VarList *vars = talloc(sizeof(VarList));
  *vars = vlist_of_typevar(var_a);

  return (Scheme){.vars = vars, .type = full_type};
}
Scheme create_opt_scheme() {

  Type *var = tvar("a");
  Type *full_type = create_option_type(var);

  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = var->data.T_VAR, .next = NULL};

  return (Scheme){.vars = vars, .type = full_type};
}

Scheme create_list_of_scheme() {

  Type *var = tvar("a");
  Type *full_type = create_list_type_of_type(var);

  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = var->data.T_VAR, .next = NULL};

  return (Scheme){.vars = vars, .type = full_type};
}

Scheme create_array_of_scheme() {

  Type *var = tvar("a");
  Type *full_type = create_array_type(var);

  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = var->data.T_VAR, .next = NULL};

  return (Scheme){.vars = vars, .type = full_type};
}
Scheme create_list_prepend_scheme() {

  Type *var = tvar("a");
  Type *list_type = create_list_type_of_type(var);

  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = var->data.T_VAR, .next = NULL};

  Scheme *scheme = talloc(sizeof(Scheme));
  Type *f = list_type;
  f = type_fn(list_type, f);
  f = type_fn(var, f);
  return (Scheme){.vars = vars, .type = f};
}

void add_primitive_scheme(char *tname, Type *t) {
  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = NULL, .type = t};
  add_builtin_scheme(tname, scheme);
}

Scheme *create_cor_map_scheme() {
  Type *a = tvar("a");
  Type *b = tvar("b");
  Type *cor_a = create_coroutine_instance_type(a);
  Type *cor_b = create_coroutine_instance_type(b);
  Type *map_fn = type_fn(a, b);

  Type *f = cor_b;
  f = type_fn(cor_a, f);
  f = type_fn(map_fn, f);

  VarList *vars_mem = talloc(sizeof(VarList) * 2);
  vars_mem[1] = (VarList){.var = b->data.T_VAR, .next = NULL};
  vars_mem[0] = (VarList){.var = a->data.T_VAR, .next = vars_mem + 1};

  Scheme *scheme = talloc(sizeof(Scheme));
  *scheme = (Scheme){.vars = vars_mem, .type = f};
  return scheme;
}

Scheme create_iter_of_list_scheme() {
  Scheme sch = create_list_of_scheme();
  Type *l = sch.type;
  Type *el = l->data.T_CONS.args[0];
  Type *cor = create_coroutine_instance_type(el);
  sch.type = type_fn(l, cor);
  return sch;
}

Scheme create_iter_of_array_scheme() {
  Scheme sch = create_array_of_scheme();
  Type *l = sch.type;
  Type *el = l->data.T_CONS.args[0];
  Type *cor = create_coroutine_instance_type(el);
  sch.type = type_fn(l, cor);
  return sch;
}

Scheme create_use_or_finish_scheme() {
  Type *var = tvar("a");
  Type *opt = create_option_type(var);
  Type *f = type_fn(opt, var);
  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = var->data.T_VAR, .next = NULL};
  return (Scheme){.vars = vars, .type = f};
}

Scheme create_logical_binop_scheme() {
  Scheme sch;
  Type *f = type_fn(&t_bool, &t_bool);
  f = type_fn(&t_bool, f);
  sch.type = f;
  return sch;
}

Scheme create_and_scheme() {
  Scheme sch = create_list_of_scheme();
  Type *l = sch.type;
  Type *el = l->data.T_CONS.args[0];
  Type *cor = create_coroutine_instance_type(el);
  sch.type = type_fn(l, cor);
  return sch;
}

void initialize_builtin_schemes() {

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

  ht_init(&builtin_schemes);
  arithmetic_scheme = _create_arithmetic_scheme();
  add_builtin_scheme("+", &arithmetic_scheme);
  add_builtin_scheme("-", &arithmetic_scheme);
  add_builtin_scheme("*", &arithmetic_scheme);
  add_builtin_scheme("/", &arithmetic_scheme);
  add_builtin_scheme("%", &arithmetic_scheme);

  eq_scheme = _create_eq_scheme();
  add_builtin_scheme("==", &eq_scheme);
  add_builtin_scheme("!=", &eq_scheme);

  ord_scheme = _create_ord_scheme();

  add_builtin_scheme(">", &ord_scheme);
  add_builtin_scheme("<", &ord_scheme);
  add_builtin_scheme(">=", &ord_scheme);
  add_builtin_scheme("<=", &ord_scheme);
  id_scheme = create_id_scheme();
  add_builtin_scheme("id", &id_scheme);

  add_primitive_scheme(TYPE_NAME_INT, &t_int);
  add_primitive_scheme(TYPE_NAME_FLOAT, &t_fl);
  add_primitive_scheme(TYPE_NAME_DOUBLE, &t_num);
  add_primitive_scheme(TYPE_NAME_UINT64, &t_uint64);
  add_primitive_scheme(TYPE_NAME_BOOL, &t_bool);
  add_primitive_scheme(TYPE_NAME_STRING, &t_string);
  add_primitive_scheme(TYPE_NAME_CHAR, &t_char);
  add_primitive_scheme(TYPE_NAME_VOID, &t_void);
  add_primitive_scheme(TYPE_NAME_PTR, &t_ptr);

  opt_scheme = create_opt_scheme();
  add_builtin_scheme("Option", &opt_scheme);
  add_builtin_scheme("Some", &opt_scheme);
  add_builtin_scheme("None", &opt_scheme);

  list_prepend_scheme = create_list_prepend_scheme();
  add_builtin_scheme("::", &list_prepend_scheme);

  list_of_scheme = create_list_of_scheme();
  add_builtin_scheme("List", &list_of_scheme);

  array_of_scheme = create_array_of_scheme();
  add_builtin_scheme("Array", &array_of_scheme);

  add_primitive_scheme("&&", &t_builtin_and);

  add_builtin_scheme("array_at", &array_at_scheme_glob);
  add_builtin_scheme("array_set", create_new_array_set_sig());
  add_builtin_scheme("array_size", create_new_array_size_sig());
  add_builtin_scheme("array_range", create_new_array_range_sig());
  add_builtin_scheme("array_succ", &id_scheme);

  add_primitive_scheme("print", type_fn(&t_string, &t_void));
  // Type t_list_prepend = MAKE_FN_TYPE_3(&t_list_var_el, &t_list_var,
  // &t_list_var);
  //
  list_concat_scheme = *create_list_concat_scheme();
  add_builtin_scheme("list_concat", &list_concat_scheme);
  add_builtin_scheme("cor_map", create_cor_map_scheme());

  iter_of_list_scheme = create_iter_of_list_scheme();
  add_builtin_scheme("iter_of_list", &iter_of_list_scheme);

  iter_of_array_scheme = create_iter_of_array_scheme();
  add_builtin_scheme("iter_of_array", &iter_of_array_scheme);

  add_builtin_scheme("cor_loop", &id_scheme);

  use_or_finish_scheme = create_use_or_finish_scheme();
  add_builtin_scheme("use_or_finish", &use_or_finish_scheme);

  bool_or_scheme = create_logical_binop_scheme();
  add_primitive_scheme("||", bool_or_scheme.type);

  bool_and_scheme = create_logical_binop_scheme();
  add_primitive_scheme("&&", bool_and_scheme.type);
}

Scheme *lookup_builtin_scheme(const char *name) {
  Scheme *builtin =
      ht_get_hash(&builtin_schemes, name, hash_string(name, strlen(name)));
  return builtin;
}
void print_builtin_types() {}
