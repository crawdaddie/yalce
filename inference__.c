#include "ht.h"
#include "inference.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include <stdlib.h>
#include <string.h>
Type *infer_fn_application(Ast *ast, TICtx *ctx);
Type *infer_cons_application(Ast *ast, TICtx *ctx);
Type *infer_yield_expr(Ast *ast, TICtx *ctx);
Type *infer_lambda(Ast *ast, TICtx *ctx);

uint64_t hash_type(Type *type); // Forward declaration

uint64_t hash_spec_fn(Type *fn) {
  // Start with the FNV offset
  uint64_t hash = FNV_OFFSET;

  // First mix in a value to distinguish function types from other types
  hash ^= (uint64_t)T_FN;
  hash *= FNV_PRIME;

  // Hash the "from" type
  uint64_t from_hash = hash_type(fn->data.T_FN.from);
  hash ^= from_hash;
  hash *= FNV_PRIME;

  // Hash the "to" type
  uint64_t to_hash = hash_type(fn->data.T_FN.to);
  hash ^= to_hash;
  hash *= FNV_PRIME;
  return hash;
}

// Helper function to hash any type
uint64_t hash_type(Type *type) {
  uint64_t hash = FNV_OFFSET;

  // Mix in the kind
  hash ^= (uint64_t)type->kind;
  hash *= FNV_PRIME;

  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_EMPTY_LIST:
    // For simple types, just the kind is enough
    return hash;

  case T_CONS: {
    // Hash the constructor name
    int name_len = strlen(type->data.T_CONS.name);
    hash ^= hash_string(type->data.T_CONS.name, name_len);
    hash *= FNV_PRIME;

    // Hash each argument
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      uint64_t arg_hash = hash_type(type->data.T_CONS.args[i]);
      hash ^= arg_hash;
      hash *= FNV_PRIME;
    }
    return hash;
  }

  case T_FN:
    return hash_spec_fn(type);

  default:
    // For unsupported types, just use the kind
    return hash;
  }
}

Type *resolve_type_in_constraints(Type *r, TypeConstraint *env);
TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2);

void typeclasses_extend(Type *t, TypeClass *tc);
bool occurs_check(Type *var, Type *t);
Type *substitute_type(Type *t, Type *var, Type *replacement);
bool unify(Type *t1, Type *t2, TypeConstraint **constraints);

Type *apply_substitution(Substitution *subst, Type *t);

Type *resolve_type_in_env(Type *r, TypeEnv *env);
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

Type *find_variant_member(Type *variant, const char *name);

Type *next_tvar() {
  Type *tvar = talloc(sizeof(Type));
  char *tname = talloc(sizeof(char) * 3);
  for (int i = 0; i < 3; i++) {
    tname[i] = 0;
  }
  sprintf(tname, "`%d", type_var_counter);
  // *tname = (char)type_var_counter;

  *tvar = (Type){T_VAR, {.T_VAR = tname}};
  type_var_counter++;
  return tvar;
}

TICtx pop_ctx(TICtx ctx) {}

void set_in_env(const char *name, Type *t, TICtx *ctx) {
  ctx->env = env_extend(ctx->env, name, t);
}

Type *find_type_in_ctx(const char *name, TICtx *ctx) {
  return env_lookup(ctx->env, name);
}

static ht builtin_types;
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

TypeList *type_list_extend(TypeList *l, Type *t) {
  TypeList *tl = talloc(sizeof(TypeList));
  tl->type = t;
  tl->next = l;
  return tl;
}

void initialize_builtin_types() {

  ht_init(&builtin_types);
  add_builtin("+", &t_add);
  add_builtin("-", &t_sub);
  add_builtin("*", &t_mul);
  add_builtin("/", &t_div);
  add_builtin("%", &t_mod);
  add_builtin(">", &t_gt);
  add_builtin("<", &t_lt);
  add_builtin(">=", &t_gte);
  add_builtin("<=", &t_lte);
  add_builtin("==", &t_eq);
  add_builtin("!=", &t_neq);

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

Type *param_binding_type(Ast *ast) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    return next_tvar();
  }

  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;

    Type **tuple_mems = malloc(sizeof(Type *) * len);

    for (int i = 0; i < len; i++) {
      Ast *mem = ast->data.AST_LIST.items + i;
      tuple_mems[i] = param_binding_type(mem);
    }

    Type *tup = empty_type();

    *tup = (Type){T_CONS,
                  {.T_CONS = {.name = TYPE_NAME_TUPLE,
                              .args = tuple_mems,
                              .num_args = len}}};
    return tup;
  }

  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}

Type *lookup_builtin_type(const char *name) {
  Type *t = ht_get_hash(&builtin_types, name, hash_string(name, strlen(name)));
  return t;
}

Type *handle_binding(Ast *binding, Ast *expr, TICtx *ctx) {
  Type *expr_type = infer(expr, ctx);
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    char *name = binding->data.AST_IDENTIFIER.value;
    set_in_env(name, expr_type, ctx);
  }
  }
  return expr_type;
}

Type *handle_binding_to_type(Ast *binding, Type *expr_type, TICtx *ctx) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    const char *name = binding->data.AST_IDENTIFIER.value;
    set_in_env(name, expr_type, ctx);
  }
  }
  return expr_type;
}

TypeEnv *bind_in_env(TypeEnv *env, Ast *binding, Type *expr_type) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    const char *name = binding->data.AST_IDENTIFIER.value;
    if (strcmp(name, "_") == 0) {
      break;
    }
    env = env_extend(env, name, expr_type);
    break;
  }

  case AST_TUPLE: {
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      env = bind_in_env(env, b, expr_type->data.T_CONS.args[i]);
    }
    break;
  }
  case AST_APPLICATION: {
    if (strcmp(
            binding->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
            "::") == 0) {
      Ast *b = binding->data.AST_APPLICATION.args;
      env = bind_in_env(env, b, expr_type->data.T_CONS.args[0]);
      Ast *r = binding->data.AST_APPLICATION.args + 1;
      env = bind_in_env(env, r, expr_type);
      break;
    }
    Ast *fn_id = binding->data.AST_APPLICATION.function;
    if (fn_id->tag != AST_IDENTIFIER) {
      return NULL;
    }
    const char *name = fn_id->data.AST_IDENTIFIER.value;

    if (is_variant_type(expr_type) &&
        strcmp(name, expr_type->data.T_CONS.name) != 0) {
      expr_type = find_variant_member(expr_type, name);
    }
    for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
      Ast *arg = binding->data.AST_APPLICATION.args + i;
      if (arg->tag == AST_IDENTIFIER) {
        env = bind_in_env(env, arg, expr_type->data.T_CONS.args[i]);
      }
    }

    break;
  }
  }
  return env;
}

bool is_recursive_ref_id(const char *name, TICtx *ctx) {
  return (ctx->current_fn_ast != NULL) &&
         (ctx->current_fn_ast->data.AST_LAMBDA.fn_name.chars != NULL) &&
         (strcmp(name, ctx->current_fn_ast->data.AST_LAMBDA.fn_name.chars) ==
          0);
}

bool is_recursive_ref(Ast *ast, TICtx *ctx) {
  return (ast->tag == AST_IDENTIFIER &&
          is_recursive_ref_id(ast->data.AST_IDENTIFIER.value, ctx));
}

bool is_none_expr(Ast *none) {
  return none->tag == AST_IDENTIFIER &&
         (strcmp(none->data.AST_IDENTIFIER.value, "None") == 0);
}

void compare_args(Type *free_arg, Type *arg, TypeEnv **env) {

  if (free_arg->kind == T_VAR && !is_generic(arg)) {
    *env = env_extend(*env, free_arg->data.T_VAR, arg);
    *free_arg = *arg;
    return;
  }
}

Type *constraints_lookup(TypeConstraint *env, const char *name) {
  while (env) {
    Type *t1 = env->t1;
    Type *t2 = env->t2;
    if ((t1->kind == T_VAR) && strcmp(t1->data.T_VAR, name) == 0) {
      return t2;
    }

    env = env->next;
  }
  return NULL;
}

Substitution *substitutions_extend(Substitution *subst, Type *t1, Type *t2) {
  Substitution *new_subst = talloc(sizeof(Substitution));
  new_subst->from = t1;
  new_subst->to = t2;
  new_subst->next = subst;
  return new_subst;
}

TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2) {
  TypeConstraint *c = talloc(sizeof(TypeConstraint));
  c->t1 = t1;
  c->t2 = t2;
  c->next = constraints;
  return c;
}

TypeClass *impls_extend(TypeClass *impls, TypeClass *tc) {
  tc->next = impls;
  return tc;
}

void typeclasses_extend(Type *t, TypeClass *tc) {
  if (!type_implements(t, tc)) {
    t->implements = impls_extend(t->implements, tc);
  }
}

Type *create_list_type(Ast *ast, const char *cons_name, TICtx *ctx) {

  if (ast->data.AST_LIST.len == 0) {
    // Type *t = talloc(sizeof(Type));
    // *t = t_empty_list;
    // return t;
    return &t_empty_list;
  }

  int len = ast->data.AST_LIST.len;
  Type *el_type = infer(ast->data.AST_LIST.items, ctx);

  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *_el_type = infer(el, ctx);

    if (!types_equal(el_type, _el_type)) {
      fprintf(stderr, "Error typechecking list literal - all elements must "
                      "be of the same type\n");
      print_type_err(el_type);
      fprintf(stderr, " != ");
      print_type_err(_el_type);
      return NULL;
    }
    el_type = _el_type;
  }
  Type *type = talloc(sizeof(Type));
  Type **contained = talloc(sizeof(Type *));
  contained[0] = el_type;
  *type = (Type){T_CONS, {.T_CONS = {cons_name, contained, 1}}};
  return type;
}

/**
 * given tvar t, range over all constraints in C that match t : t'
 * and call the callback for each constraint t : t'
 * */
void constraints_iter(Type *tvar, TypeConstraint *constraints,
                      void (*callback)(Type *, Type *)) {
  for (TypeConstraint *constraint = constraints; constraint != NULL;
       constraint = constraint->next) {
    Type *t1 = constraint->t1;
    Type *t2 = constraint->t2;
    if (t1->kind == T_VAR && strcmp(t1->data.T_VAR, tvar->data.T_VAR) == 0) {
      callback(t1, t2);
    }
  }
}

void constraints_iter_upto(Type *tvar, TypeConstraint *constraints,
                           TypeConstraint *end,
                           void (*callback)(Type *, Type *)) {
  for (TypeConstraint *constraint = constraints;
       constraint != NULL || constraint != end; constraint = constraint->next) {
    Type *t1 = constraint->t1;
    Type *t2 = constraint->t2;
    if (t1->kind == T_VAR && strcmp(t1->data.T_VAR, tvar->data.T_VAR) == 0) {
      callback(t1, t2);
    }
  }
}
void print_constraints(TypeConstraint *c) {
  for (TypeConstraint *con = c; con != NULL; con = con->next) {

    printf("constraint: ");
    if (con->t1->kind == T_VAR) {
      printf("%s : ", con->t1->data.T_VAR);
      print_type(con->t2);
    } else {
      print_type(con->t1);
      print_type(con->t2);
    }
  }
}

void print_subst(Substitution *c) {
  for (Substitution *con = c; con != NULL; con = con->next) {

    printf("subst: ");
    if (con->from->kind == T_VAR) {
      printf("%s with ", con->from->data.T_VAR);
      print_type(con->to);
    } else {
      print_type(con->from);
      printf("with ");
      print_type(con->to);
    }
  }
}

bool satisfies_tc_constraint(Type *t, TypeClass *constraint_tc) {
  if (t->kind == T_TYPECLASS_RESOLVE &&
      strcmp(t->data.T_CONS.name, constraint_tc->name) == 0) {
    return true;
  }

  for (TypeClass *tc = t->implements; tc != NULL; tc = tc->next) {
    if (strcmp(tc->name, constraint_tc->name) == 0) {
      return true;
    }
  }
  return false;
}

double get_type_rank(Type *t, TypeClass *tc) {

  if (t->kind == T_VAR) {
    return 1000.; // arbitrary large number
  }

  TypeClass *T = get_typeclass_by_name(t, tc->name);

  return T->rank;
}

bool unify_option(Type *t1, Type *t2, TypeConstraint **constraints) {

  if (is_option_type(t1) && t2->alias && (strcmp(t2->alias, "Option") == 0)) {
    return true;
  }

  if (is_option_type(t2) && t1->alias && (strcmp(t1->alias, "Option") == 0)) {
    return true;
  }
  return false;
}

bool is_none_variant_member_type(Type *t) {
  return (t->kind == T_CONS) && (strcmp(t->data.T_CONS.name, "None") == 0);
}

Type *find_option_in_constraints(Type *var, TypeConstraint *constraints) {
  while (constraints) {
    Type *t1 = constraints->t1;
    Type *t2 = constraints->t2;
    if (types_equal(t1, var) && t2->kind == T_CONS &&
        strcmp(t2->data.T_CONS.name, "variant")) {
      return t2;
    }
    constraints = constraints->next;
  }
  return NULL;
}

bool unify(Type *t1, Type *t2, TypeConstraint **constraints) {

  // printf("unify conses??\n");
  // print_type(t1);
  // print_type(t2);

  if (t2->kind == T_EMPTY_LIST) {
    return true;
  }

  if (t1->kind == T_VAR) {
    if (is_none_variant_member_type(t2) &&
        find_option_in_constraints(t1, *constraints)) {
      return true;
    }

    if (t2->kind == T_VAR && strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0) {
      return true; // Same type variable
    }

    // If t1 has a typeclass constraint
    if (t1->implements) {

      // If t2 is a concrete type, check if it satisfies the constraint
      if (t2->kind != T_VAR) {

        if (!satisfies_tc_constraint(t2, t1->implements)) {
          fprintf(stderr, "Type doesn't satisfy typeclass constraint '%s'\n",
                  t1->implements->name);
          return false;
        } else {

          *constraints = constraints_extend(*constraints, t1, t2);
          return true;
        }
      } else {
        // If t2 is a type var, it inherits the constraint
        //
        // printf("extend typeclasses\n");
        // print_type(t2);
        // print_type(t1);
        t2->implements = t1->implements;
      }
    }

    // Check for recursive types
    if (occurs_check(t1, t2)) {
      return false; // Would create infinite type
    }

    // Add a new constraint
    *constraints = constraints_extend(*constraints, t1, t2);
    return true;
  }

  if (t2->kind == T_VAR) {
    return unify(t2, t1, constraints);
  }

  // Handle function types
  if (t1->kind == T_FN && t2->kind == T_FN) {
    return unify(t1->data.T_FN.from, t2->data.T_FN.from, constraints) &&
           unify(t1->data.T_FN.to, t2->data.T_FN.to, constraints);
  }

  // Handle constructed types
  if (t1->kind == T_CONS && t2->kind == T_CONS) {
    if (is_pointer_type(t1)) {
      return true;
    }

    // if (is_list_type(t1) && is_list_type(t2)) {
    //   return unify(t1->data.T_CONS.args[0], t2->data.T_CONS.args[0],
    //   constraints);
    // }

    if (is_option_type(t1) && t2->alias && (strcmp(t2->alias, "Option") == 0)) {
      // TODO: unify contained types here as well?
      return true;
    }

    if (is_option_type(t2) && t1->alias && (strcmp(t1->alias, "Option") == 0)) {
      return true;
    }

    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return false;
    }

    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      if (!unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i],
                 constraints)) {
        return false;
      }
    }
    return true;
  }
  if (t1->kind == T_UINT64 && t2->kind == T_INT) {
    return true;
  }

  if ((t1->kind != t2->kind) && is_pointer_type(t1) && (t2->kind == T_FN)) {
    return true;
  }

  if ((t1->kind != t2->kind) && is_pointer_type(t1)) {
    return true;
  }

  // Handle primitive types
  return t1->kind == t2->kind;
}

bool match_typeclasses(Type *t1, Type *t2) {
  for (TypeClass *tc = t1->implements; tc; tc = tc->next) {
    if (!satisfies_tc_constraint(t2, tc)) {
      return false;
    }
  }
  return true;
}
double rank_sum(Type *t) {

  double res;
  for (TypeClass *tc = t->implements; tc; tc = tc->next) {
    res += tc->rank;
  }
  return res;
}
double sum_tc_ranks(Type *t1, Type *t2) {
  double sum = 0.;
  for (TypeClass *tc = t2->implements; tc != NULL; tc = tc->next) {
    sum += get_typeclass_rank(t1, tc->name);
  }
  return sum;
}

Substitution *__solve_constraints(TypeConstraint *constraints) {
  Substitution *subst = NULL;

  while (constraints) {
    Type *t1 = apply_substitution(subst, constraints->t1);
    Type *t2 = apply_substitution(subst, constraints->t2);

    if (is_none_variant_member_type(t1) && is_option_type(t2)) {
      subst = substitutions_extend(subst, t1, t2);
    } else if (t1->kind == T_VAR) {
      if (occurs_check(t1, t2)) {
        return NULL; // Infinite type error
      }
      subst = substitutions_extend(subst, t1, t2);
    } else if (t2->kind == T_VAR) {
      if (occurs_check(t2, t1)) {
        return NULL; // Infinite type error
      }
      subst = substitutions_extend(subst, t2, t1);
    } else if (t1->kind == T_EMPTY_LIST) {
      subst = substitutions_extend(subst, t1, t2);
    } else if (t1->kind == T_TYPECLASS_RESOLVE && !is_generic(t2)) {
      for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t1->data.T_CONS.args[i], t2);
      }
    } else if (is_list_type(t1) && t2->kind == T_EMPTY_LIST) {
      // Lists are compatible - continue
    } else if (t1->kind == T_CONS && t2->kind == T_CONS) {
      printf("UNIFY All constructor args\n");
      print_type(t1);
      print_type(t2);

      // Add this implementation for constructor types
      if ((strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0) ||
          t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
        return NULL; // Constructor mismatch
      }

      // Unify all constructor arguments
      for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
        Type *cons_arg1 = t1->data.T_CONS.args[i];
        Type *cons_arg2 = t2->data.T_CONS.args[i];

        if (is_none_variant_member_type(cons_arg1) &&
            is_none_variant_member_type(cons_arg2)) {
          continue;
        }

        if ((cons_arg1->alias && strcmp(cons_arg1->alias, "Option") == 0) &&
            (cons_arg2->alias && strcmp(cons_arg2->alias, "Option") == 0)) {
          Type *v = cons_arg2->data.T_CONS.args[0]->data.T_CONS.args[0];
          Type *v1 = cons_arg1->data.T_CONS.args[0]->data.T_CONS.args[0];
          subst = substitutions_extend(subst, v, v1);
        }
      }
    } else if (t1->kind != t2->kind) {
      fprintf(stderr, "Error: type mismatch in solve constraints\n");
      print_type_err(t1);
      print_type_err(t2);
      fprintf(stderr, "\n");
      return NULL;
    }

    constraints = constraints->next;
  }

  return subst;
}
bool _is_option_type(Type *t) {
  return (t->alias != NULL) && (strcmp(t->alias, "Option") == 0);
}
Substitution *solve_constraints(TypeConstraint *constraints) {
  Substitution *subst = NULL;

  while (constraints) {
    Type *t1 = apply_substitution(subst, constraints->t1);
    Type *t2 = apply_substitution(subst, constraints->t2);

    if (is_none_variant_member_type(t1) && is_option_type(t2)) {
      subst = substitutions_extend(subst, t1, t2);
    } else if (t1->kind == T_VAR) {
      if (occurs_check(t1, t2)) {
        return NULL; // Infinite type error
      }
      subst = substitutions_extend(subst, t1, t2);
    } else if (t2->kind == T_VAR) {
      if (occurs_check(t2, t1)) {
        return NULL; // Infinite type error
      }
      subst = substitutions_extend(subst, t2, t1);
    } else if (t1->kind == T_EMPTY_LIST) {
      subst = substitutions_extend(subst, t1, t2);
    } else if (t1->kind == T_TYPECLASS_RESOLVE && !is_generic(t2)) {
      for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t1->data.T_CONS.args[i], t2);
      }
    } else if (is_list_type(t1) && t2->kind == T_EMPTY_LIST) {
      // Lists are compatible - continue
    } else if (t1->kind == T_CONS && t2->kind == T_CONS) {
      // Handle variant types with same constructor differently
      if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) == 0 &&
          t1->data.T_CONS.num_args == t2->data.T_CONS.num_args) {

        // If these are option types, unify their contained types
        if (_is_option_type(t1) && _is_option_type(t2)) {
          Type *contained1 = t1->data.T_CONS.args[0]->data.T_CONS.args[0];
          Type *contained2 = t2->data.T_CONS.args[0]->data.T_CONS.args[0];

          // Create substitution between contained types if either is a type
          // variable
          if (contained1->kind == T_VAR) {
            subst = substitutions_extend(subst, contained1, contained2);
          } else if (contained2->kind == T_VAR) {
            subst = substitutions_extend(subst, contained2, contained1);
          }
        }

        // For all constructor types, unify their arguments
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          Type *cons_arg1 = t1->data.T_CONS.args[i];
          Type *cons_arg2 = t2->data.T_CONS.args[i];

          if (is_none_variant_member_type(cons_arg1) &&
              is_none_variant_member_type(cons_arg2)) {
            continue;
          }

          // If either argument is a type variable, create substitution
          if (cons_arg1->kind == T_VAR) {
            subst = substitutions_extend(subst, cons_arg1, cons_arg2);
          } else if (cons_arg2->kind == T_VAR) {
            subst = substitutions_extend(subst, cons_arg2, cons_arg1);
          } else if (_is_option_type(cons_arg2) && _is_option_type(cons_arg1)) {
            subst = substitutions_extend(
                subst, cons_arg2->data.T_CONS.args[0]->data.T_CONS.args[0],
                cons_arg1->data.T_CONS.args[0]->data.T_CONS.args[0]);
          }
        }
      } else {
        return NULL; // Constructor mismatch
      }
    }
    // else if (is_pointer_type(t1) && is_coroutine_type(t2)) {
    //   printf("pointer type vs coroutine inst\n");
    // }
    else if (t1->kind != t2->kind) {

      fprintf(stderr, "Error: type mismatch in solve constraints\n");
      print_type_err(t1);
      print_type_err(t2);
      fprintf(stderr, "\n");
      return NULL;
    }

    constraints = constraints->next;
  }

  return subst;
}

// Helper functions

bool occurs_check(Type *var, Type *t) {
  if (t->kind == T_VAR) {
    return strcmp(var->data.T_VAR, t->data.T_VAR) == 0;
  }

  if (t->kind == T_FN) {
    return occurs_check(var, t->data.T_FN.from) ||
           occurs_check(var, t->data.T_FN.to);
  }

  if (t->kind == T_CONS || t->kind == T_TYPECLASS_RESOLVE) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      if (occurs_check(var, t->data.T_CONS.args[i])) {
        return true;
      }
    }
  }

  return false;
}

Type *apply_substitution(Substitution *subst, Type *t) {
  if (!subst) {
    return t;
  }

  if (!t)
    return NULL;

  if (t->kind == T_VAR) {
    Substitution *s = subst;
    while (s) {
      if (s->from->kind == T_EMPTY_LIST) {
        *s->from = *s->to;
      } else if (strcmp(t->data.T_VAR, s->from->data.T_VAR) == 0 &&
                 !(occurs_check(t, s->to))) {
        return apply_substitution(subst, s->to);
      }
      s = s->next;
    }
    return t;
  }

  if (t->kind == T_FN) {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_FN.from = apply_substitution(subst, t->data.T_FN.from);
    new_t->data.T_FN.to = apply_substitution(subst, t->data.T_FN.to);
    return new_t;
  }

  if (t->kind == T_TYPECLASS_RESOLVE) {

    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return resolve_tc_rank(new_t);
  }

  if (t->kind == T_CONS) {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return new_t;
  }

  return t;
}
void apply_substitution_to_nodes_rec(Substitution *subst, Ast *ast) {
  switch (ast->tag) {
  case AST_APPLICATION: {
    ast->data.AST_APPLICATION.function->md =
        apply_substitution(subst, ast->data.AST_APPLICATION.function->md);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      apply_substitution_to_nodes_rec(subst,
                                      ast->data.AST_APPLICATION.args + i);
    }
    break;
  }

  case AST_BODY: {
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      apply_substitution_to_nodes_rec(subst, ast->data.AST_BODY.stmts[i]);
    }
    break;
  }

  case AST_LAMBDA: {
    apply_substitution_to_nodes_rec(subst, ast->data.AST_LAMBDA.body);
    break;
  }

  case AST_TUPLE:
  case AST_ARRAY:
  case AST_LIST: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      apply_substitution_to_nodes_rec(subst, ast->data.AST_LIST.items + i);
    }
    break;
  }
  }
  ast->md = apply_substitution(subst, ast->md);
}

// Collect free type variables in a type
void collect_type_vars(Type *t, const char **vars, int *count) {
  if (!t)
    return;

  if (t->kind == T_VAR) {
    // Check if we already have this variable
    for (int i = 0; i < *count; i++) {
      if (strcmp(vars[i], t->data.T_VAR) == 0) {
        return;
      }
    }
    // Add new variable
    vars[*count] = t->data.T_VAR;
    (*count)++;
    return;
  }

  if (t->kind == T_FN) {
    collect_type_vars(t->data.T_FN.from, vars, count);
    collect_type_vars(t->data.T_FN.to, vars, count);
    return;
  }

  if (t->kind == T_CONS) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      collect_type_vars(t->data.T_CONS.args[i], vars, count);
    }
  }
}

// Collect type variables free in the environment
void collect_env_vars(TypeEnv *env, char **vars, int *count) {
  while (env) {
    collect_type_vars(env->type, vars, count);
    env = env->next;
  }
}
Type *create_forall_type(int quantified_count, const char **quantified_vars,
                         Type *t) {
  // Create forall type constructor
  Type *forall = talloc(sizeof(Type));
  forall->kind = T_CONS;
  forall->data.T_CONS.name = "forall";
  forall->data.T_CONS.num_args = quantified_count + 1;
  forall->data.T_CONS.args = talloc(sizeof(Type *) * (quantified_count + 1));
  // forall->data.T_CONS.names = talloc(sizeof(char *) * quantified_count);

  // Add quantified variables
  for (int i = 0; i < quantified_count; i++) {
    Type *var = tvar(quantified_vars[i]);
    forall->data.T_CONS.args[i] = var;
    // forall->data.T_CONS.names[i] = quantified_vars[i];
  }

  // Add the type body
  forall->data.T_CONS.args[quantified_count] = t;
  return forall;
}

Type *generalize_type(TypeEnv *env, Type *t) {
  if (t->kind == T_FN && is_generic(t)) {
    return t;
  }
  return t;
  /*
    // Collect variables from type
    const char *type_vars[100]; // Arbitrary limit
    int type_var_count = 0;
    collect_type_vars(t, type_vars, &type_var_count);

    // Collect variables from environment
    char *env_vars[100];
    int env_var_count = 0;
    collect_env_vars(env, env_vars, &env_var_count);

    // Find variables to quantify (in type but not in env)
    const char *quantified_vars[100];
    int quantified_count = 0;

    for (int i = 0; i < type_var_count; i++) {
      bool in_env = false;
      for (int j = 0; j < env_var_count; j++) {
        if (strcmp(type_vars[i], env_vars[j]) == 0) {
          in_env = true;
          break;
        }
      }
      if (!in_env) {
        quantified_vars[quantified_count++] = type_vars[i];
      }
    }

    // If no variables to quantify, return type as is
    if (quantified_count == 0) {
      return t;
    }

    Type *forall = create_forall_type(quantified_count, quantified_vars, t);

    return forall;
    */
}
bool is_list_cons_pattern(Ast *pattern) {
  Ast *fn = pattern->data.AST_APPLICATION.function;
  if (fn->tag != AST_IDENTIFIER) {
    return false;
  }
  return strcmp(fn->data.AST_IDENTIFIER.value, "::") == 0;
}

Type *infer_pattern(Ast *pattern, TICtx *ctx) {
  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    const char *name = pattern->data.AST_IDENTIFIER.value;
    Type *type = infer(pattern, ctx);

    if (is_variant_type(type) && strcmp(type->data.T_CONS.name, name) != 0) {
      type = deep_copy_type(type);
    }
    return type;
  }

  case AST_TUPLE: {
    // Tuple pattern (x, y)
    int len = pattern->data.AST_LIST.len;
    Type **member_types = talloc(sizeof(Type *) * len);

    for (int i = 0; i < len; i++) {
      member_types[i] = infer_pattern(&pattern->data.AST_LIST.items[i], ctx);
      if (!member_types[i])
        return NULL;
    }

    Type *tuple_type = create_tuple_type(len, member_types);

    return tuple_type;
  }

  case AST_APPLICATION: {
    // Handle cons pattern (x::xs)
    if (is_list_cons_pattern(pattern)) {
      Type *elem_type = infer_pattern(pattern->data.AST_APPLICATION.args, ctx);
      Type *rest_type =
          infer_pattern(pattern->data.AST_APPLICATION.args + 1, ctx);

      if (!elem_type)
        return NULL;

      // Create list type containing elem_type
      Type **mems = talloc(sizeof(Type));
      mems[0] = elem_type;
      Type *list_type = create_cons_type(TYPE_NAME_LIST, 1, mems);
      return list_type;
    }

    return infer(pattern, ctx);
  }
  case AST_LIST:
  case AST_INT:
  case AST_DOUBLE:
  case AST_BOOL:
  case AST_CHAR:
  case AST_STRING:
  case AST_VOID: {
    return infer(pattern, ctx);
  }
  case AST_MATCH_GUARD_CLAUSE: {
    return infer_pattern(pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr, ctx);
  }
  }

  fprintf(stderr, "Unsupported pattern in let binding\n");
  print_ast_err(pattern);
  return NULL;
}

Type *find_variant_member(Type *variant, const char *name) {
  for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
    Type *mem = variant->data.T_CONS.args[i];
    if (strcmp(mem->data.T_CONS.name, name) == 0) {
      return mem;
    }
  }
  return NULL;
}

bool unify_in_ctx(Type *arg_type, Type *constraint_type, TICtx *ctx) {

  TypeConstraint *constraints = ctx->constraints;
  if (!unify(arg_type, constraint_type, &constraints)) {
    return false;
  }
  ctx->constraints = constraints;

  return true;
}

Type *coroutine_constructor_type_from_fn_type(Type *fn_type) {
  Type *ret = fn_return_type(fn_type);
  Type *coroutine_fn = create_coroutine_instance_type(ret);

  Type *f = deep_copy_type(fn_type);
  Type *ff = f;

  while (ff->kind == T_FN) {
    ff = ff->data.T_FN.to;
  }

  *ff = *coroutine_fn;
  f->is_coroutine_constructor = true;
  // printf("coroutine type: ");
  // print_type(f);

  return f;
}

Type *infer_anonymous_lambda(Ast *ast, Type **param_types, int num_params,
                             TICtx *lambda_ctx) {
  Type *body_type = infer(ast->data.AST_LAMBDA.body, lambda_ctx);
  if (!body_type)
    return NULL;

  Type *fn_type = body_type;
  for (int i = num_params - 1; i >= 0; i--) {
    Type *new_fn = talloc(sizeof(Type));
    new_fn->kind = T_FN;
    new_fn->data.T_FN.from = param_types[i];
    new_fn->data.T_FN.to = fn_type;
    fn_type = new_fn;
  }

  // Solve constraints to get concrete types
  if (lambda_ctx->constraints) {
    Substitution *subst = solve_constraints(lambda_ctx->constraints);
    if (!subst)
      return NULL;
    fn_type = apply_substitution(subst, fn_type);
  }
  return fn_type;
}
static void bind_lambda_args(TICtx *lambda_ctx, Ast *param, Type **param_types,
                             int i) {
  if (param->tag == AST_TUPLE) {
    int len = param->data.AST_LIST.len;
    Type **types = talloc(sizeof(Type *));
    for (int i = 0; i < len; i++) {
      types[i] = next_tvar();
    }
    param_types[i] = create_tuple_type(len, types);
    param->md = param_types[i];
  }
  lambda_ctx->env = bind_in_env(lambda_ctx->env, param, param_types[i]);
}

Type *infer(Ast *ast, TICtx *ctx) {
  Type *type = NULL;
  switch (ast->tag) {

  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      stmt = ast->data.AST_BODY.stmts[i];

      Type *t = infer(stmt, ctx);

      if (!t) {
        fprintf(stderr, "Failure typechecking body statement: ");
        print_ast_err(stmt);
        print_location(stmt);
        return NULL;
      }
      type = t;
    }
    break;
  }

  case AST_INT: {
    type = &t_int;
    break;
  }

  case AST_DOUBLE: {
    type = &t_num;
    break;
  }

  case AST_STRING: {
    type = &t_string;
    break;
  }

  case AST_CHAR: {
    type = &t_char;
    break;
  }

    // case AST_FMT_STRING: {
    //   break;
    // }

  case AST_BOOL: {
    type = &t_bool;
    break;
  }

  case AST_VOID: {
    type = &t_void;
    break;
  }

  case AST_IDENTIFIER: {

    const char *name = ast->data.AST_IDENTIFIER.value;
    type = find_type_in_ctx(name, ctx);
    if (type && type->kind == T_CREATE_NEW_GENERIC) {
      type = type->data.T_CREATE_NEW_GENERIC(NULL);
    }

    // if (type == NULL) {
    //   type = lookup_builtin_type(name);
    // }

    if (type == NULL) {
      type = next_tvar();
    }

    break;
  }

  case AST_UNOP: {
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_STAR: {
      break;
    }

    case TOKEN_AMPERSAND: {
      break;
    }
    }
  }

  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;

    Type **cons_args = talloc(sizeof(Type *) * len);
    bool is_struct_of_coroutines = false;

    for (int i = 0; i < len; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = infer(member, ctx);
      cons_args[i] = mtype;
      if (is_coroutine_type(mtype)) {
        is_struct_of_coroutines = true;
      }
    }

    type = talloc(sizeof(Type));

    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, cons_args, len}}};

    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      const char **names = talloc(sizeof(char *) * len);
      for (int i = 0; i < len; i++) {
        Ast *member = ast->data.AST_LIST.items + i;
        names[i] = member->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      }
      type->data.T_CONS.names = names;
    }
    if (is_struct_of_coroutines) {
      for (int i = 0; i < len; i++) {
        if (is_coroutine_type(type->data.T_CONS.args[i])) {
          type->data.T_CONS.args[i] =
              type_of_option(fn_return_type(type->data.T_CONS.args[i]));
        }
      }
    }
    if (is_struct_of_coroutines) {
      type = type_fn(&t_void, create_option_type(type));
      type->is_coroutine_instance = true;
    }

    break;
  }

  case AST_LIST: {
    type = create_list_type(ast, TYPE_NAME_LIST, ctx);
    break;
  }

  case AST_EMPTY_LIST: {
    Type *ltype = talloc(sizeof(Type));
    Type **contained = talloc(sizeof(Type *));
    contained[0] =
        find_type_in_ctx(ast->data.AST_EMPTY_LIST.type_id.chars, ctx);
    *ltype = (Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, contained, 1}}};
    type = ltype;

    break;
  }

  case AST_ARRAY: {
    type = create_list_type(ast, TYPE_NAME_ARRAY, ctx);
    break;
  }

  case AST_EXTERN_FN: {
    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;
    int params_count = sig->data.AST_LIST.len - 1;

    if (sig->tag == AST_FN_SIGNATURE) {
      Type *f = compute_type_expression(sig->data.AST_LIST.items + params_count,
                                        ctx->env);
      sig->data.AST_LIST.items[params_count].md = f;

      for (int i = params_count - 1; i >= 0; i--) {
        Type *p =
            compute_type_expression(sig->data.AST_LIST.items + i, ctx->env);

        sig->data.AST_LIST.items[i].md = p;

        f = type_fn(p, f);
      }
      type = f;
    }

    Type *f = compute_type_expression(sig->data.AST_LIST.items + params_count,
                                      ctx->env);
    break;
  }

  case AST_RECORD_ACCESS: {
    Ast *obj = ast->data.AST_RECORD_ACCESS.record;
    Ast *mem_ast = ast->data.AST_RECORD_ACCESS.member;
    const char *mem = mem_ast->data.AST_IDENTIFIER.value;
    Type *obj_type = infer(obj, ctx);

    for (int i = 0; i < obj_type->data.T_CONS.num_args; i++) {
      if (strcmp(obj_type->data.T_CONS.names[i], mem) == 0) {
        type = obj_type->data.T_CONS.args[i];
        break;
      }
    }
    if (type == NULL) {
      fprintf(stderr, "Typecheck Error, no member %s found in\n", mem);
      print_ast_err(obj);
    }

    break;
  }
  case AST_APPLICATION: {
    // First infer the function type
    Type *fn_type = infer(ast->data.AST_APPLICATION.function, ctx);

    if (ast->data.AST_APPLICATION.args->tag == AST_LIST &&
        (ast->data.AST_APPLICATION.args->data.AST_LIST.len == 0)) {
      ast->data.AST_APPLICATION.args->md = &t_empty_list;
      Type **cont = talloc(sizeof(Type *));
      cont[0] = fn_type;
      Type *ltype = create_cons_type(TYPE_NAME_LIST, 1, cont);
      type = ltype;
      break;
    }

    if (!fn_type->is_recursive_fn_ref) {
      fn_type = deep_copy_type(fn_type);
    }

    if (!fn_type) {
      fprintf(stderr, "Could not infer function type in application\n");
      return NULL;
    }

    const char *fn_name =
        ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    if (fn_type->kind == T_CONS) {
      type = infer_cons_application(ast, ctx);
      break;
    }
    type = infer_fn_application(ast, ctx);
    break;
  }

  case AST_LET: {
    // printf("AST LET: ");
    // print_ast(ast);

    // First infer definition type
    Type *def_type = infer(ast->data.AST_LET.expr, ctx);
    if (!def_type) {
      fprintf(stderr, "Could not infer definition type in let\n");
      return NULL;
    }

    // Create binding pattern type and add constraints
    Type *binding_type = infer_pattern(ast->data.AST_LET.binding, ctx);
    if (!binding_type) {
      fprintf(stderr, "Could not infer binding pattern type\n");
      return NULL;
    }

    // Unify definition type with binding pattern type
    if (ast->data.AST_LET.binding->tag == AST_TUPLE) {
      print_type(def_type);
      print_type(binding_type);
    }
    if (!unify_in_ctx(def_type, binding_type, ctx)) {
      print_ast_err(ast->data.AST_LET.binding);
      fprintf(stderr, "Definition type doesn't match binding pattern\n");
      print_type_err(def_type);
      fprintf(stderr, " != ");
      print_type_err(binding_type);
      return NULL;
    }

    // Solve all constraints
    if (ctx->constraints) {
      Substitution *subst = solve_constraints(ctx->constraints);
      if (!subst) {
        fprintf(stderr, "Could not solve constraints for let definition\n");
        return NULL;
      }
      def_type = apply_substitution(subst, def_type);
    }

    // Generalize the type
    Type *gen_type = generalize_type(ctx->env, def_type);

    // Infer the body type if there is one
    if (ast->data.AST_LET.in_expr) {
      TICtx body_ctx = *ctx;
      body_ctx.constraints = NULL;
      body_ctx.scope++;
      body_ctx.env =
          bind_in_env(body_ctx.env, ast->data.AST_LET.binding, gen_type);

      type = infer(ast->data.AST_LET.in_expr, &body_ctx);
    } else {
      ctx->env = bind_in_env(ctx->env, ast->data.AST_LET.binding, gen_type);
      type = gen_type;
    }

    // print_type_env(ctx->env);
    // print_type(gen_type);

    break;
  }

  case AST_LAMBDA: {
    type = infer_lambda(ast, ctx);
    break;
  }
  case AST_MATCH: {
    Type *result = next_tvar();
    Ast *expr = ast->data.AST_MATCH.expr;
    // Infer type of expression being matched
    Type *expr_type = infer(expr, ctx);

    if (!expr_type) {
      fprintf(stderr, "Could not infer match expression type\n");
      print_ast_err(expr);
      return NULL;
    }

    int len = ast->data.AST_MATCH.len;

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Ast *branch_pattern = &ast->data.AST_MATCH.branches[2 * i];
      Ast *guard_clause = NULL;

      if (branch_pattern->tag == AST_MATCH_GUARD_CLAUSE) {
        guard_clause = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
        branch_pattern = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
      }

      Ast *branch_body = &ast->data.AST_MATCH.branches[2 * i + 1];

      Type *pattern_type = infer_pattern(branch_pattern, ctx);

      if (!pattern_type) {
        fprintf(stderr, "Could not infer pattern type in match branch\n");
        return NULL;
      }
      // print_type(pattern_type);
      // print_type(expr_type);

      // Add pattern <> expression constraint
      if ((!is_none_expr(branch_pattern)) &&
          !unify_in_ctx(expr_type, pattern_type, ctx)) {

        fprintf(stderr, "Pattern type doesn't match expression type\n");
        print_type_err(expr_type);
        fprintf(stderr, " != ");
        print_type_err(pattern_type);
        return NULL;
      }

      // handle branch body
      TICtx branch_ctx = *ctx;
      branch_ctx.scope++;
      branch_ctx.constraints = NULL; // Start with fresh constraints for
      // branch
      branch_ctx.env =
          bind_in_env(branch_ctx.env, branch_pattern, pattern_type);

      if (guard_clause) {
        infer(guard_clause, &branch_ctx);
      }

      Type *branch_type = infer(branch_body, &branch_ctx);

      if (!branch_type) {
        fprintf(stderr, "Could not infer match branch body type\n");
        return NULL;
      }

      if (!unify_in_ctx(result, branch_type, ctx)) {
        fprintf(stderr, "Inconsistent types in match branches\n");
        return NULL;
      }

      if (branch_ctx.current_fn_constraints) {
        ctx->current_fn_constraints = branch_ctx.current_fn_constraints;
      }
    }

    Substitution *subst = solve_constraints(ctx->constraints);

    if (!subst) {
      fprintf(stderr, "Could not solve match constraints\n");
      return NULL;
    }

    type = apply_substitution(subst, result);

    apply_substitution_to_nodes_rec(subst, expr);

    for (int i = 0; i < len; i++) {
      Ast *branch_pattern = ast->data.AST_MATCH.branches + (2 * i);
      Ast *guard_clause = NULL;

      Ast *branch_body = ast->data.AST_MATCH.branches + (2 * i + 1);
      apply_substitution_to_nodes_rec(subst, branch_body);

      if (branch_pattern->tag == AST_MATCH_GUARD_CLAUSE) {
        guard_clause = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
        branch_pattern = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;

        branch_pattern->md = apply_substitution(subst, branch_pattern->md);
        ast->data.AST_MATCH.branches[2 * i].md = branch_pattern->md;
        guard_clause->md = apply_substitution(subst, guard_clause->md);
        apply_substitution_to_nodes_rec(subst, guard_clause);

      } else {
        apply_substitution_to_nodes_rec(subst, branch_pattern);
      }
    }

    expr_type = apply_substitution(subst, expr_type);
    break;
  }

  case AST_TYPE_DECL: {
    type = type_declaration(ast, &ctx->env);
    break;
  }
  case AST_FMT_STRING: {

    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      infer(member, ctx);
    }

    type = &t_string;
    break;
  }
  case AST_YIELD: {
    type = infer_yield_expr(ast, ctx);
    break;
  }
  }
  ast->md = type;
  return type;
}

Type *infer_cons_application(Ast *ast, TICtx *ctx) {
  Type *fn_type = ast->data.AST_APPLICATION.function->md;

  // cons application
  Ast *fn_id = ast->data.AST_APPLICATION.function;
  const char *fn_name = fn_id->data.AST_IDENTIFIER.value;
  Type *cons = fn_type;

  if (is_variant_type(fn_type)) {

    cons = find_variant_member(fn_type, fn_name);

    if (!cons) {
      fprintf(stderr, "Error: %s not found in variant %s\n", fn_name,
              cons->data.T_CONS.name);
      return NULL;
    }
  }
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {

    Type *cons_arg = cons->data.T_CONS.args[i];
    Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, ctx);

    if (!arg_type) {
      fprintf(stderr, "Could not infer argument type in cons %s application\n",
              cons->data.T_CONS.name);
      return NULL;
    }

    if (!unify_in_ctx(cons_arg, arg_type, ctx)) {
      fprintf(stderr, "Could not constrain type variable to function type\n");
      print_type_err(arg_type);
      print_type_err(cons_arg);
      return NULL;
    }
  }

  Substitution *subst = solve_constraints(ctx->constraints);
  return apply_substitution(subst, fn_type);
}

Type *constrain_fn_type_var(Type *fn_type, Ast *ast, TICtx *ctx) {

  TICtx app_ctx = *ctx;

  int app_len = ast->data.AST_APPLICATION.len;
  Type *arg_types[app_len];

  for (int i = 0; i < app_len; i++) {
    Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, &app_ctx);
    if (!arg_type) {
      fprintf(stderr, "Could not infer argument type in application\n");
      return NULL;
    }
    arg_types[i] = arg_type;
  }

  Type *ret_type = next_tvar();
  Type *fn_constraint =
      create_type_multi_param_fn(app_len, arg_types, ret_type);

  if (!unify_in_ctx(fn_type, fn_constraint, &app_ctx)) {
    fprintf(stderr, "Could not constrain type variable to function type\n");
    print_type_err(fn_type);
    print_type_err(fn_constraint);
    return NULL;
  }
  Type *current_type = ret_type;

  for (TypeConstraint *c = app_ctx.constraints; c != NULL; c = c->next) {
    ctx->constraints = constraints_extend(ctx->constraints, c->t1, c->t2);
  }
  // After processing all arguments, solve collected constraints
  Substitution *subst = solve_constraints(app_ctx.constraints);

  if (!subst) {
    fprintf(stderr, "Could not solve type constraints in application\n");
    print_ast_err(ast);
    print_type_err(fn_type);
    return NULL;
  }

  // Apply substitutions to get final type
  Type *type = apply_substitution(subst, current_type);
  Type *spec_fn = apply_substitution(subst, fn_type);
  ast->data.AST_APPLICATION.function->md = spec_fn;

  for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    Type inferred = *((Type *)ast->data.AST_APPLICATION.args[i].md);

    apply_substitution_to_nodes_rec(subst, ast->data.AST_APPLICATION.args + i);

    Type *after = ast->data.AST_APPLICATION.args[i].md;

    if (inferred.kind == T_VAR) {
      ctx->current_fn_constraints = constraints_extend(
          ctx->current_fn_constraints, deep_copy_type(&inferred), after);
    }
  }
  return type;
}

Type *infer_fn_application(Ast *ast, TICtx *ctx) {

  Type *fn_type = ast->data.AST_APPLICATION.function->md;
  TICtx app_ctx = *ctx;
  Type *type;

  Type *current_type = fn_type;

  int app_len = ast->data.AST_APPLICATION.len;
  Type *arg_types[app_len];

  for (int i = 0; i < app_len; i++) {
    Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, &app_ctx);
    if (!arg_type) {
      fprintf(stderr, "Could not infer argument type in application\n");
      return NULL;
    }
    arg_types[i] = arg_type;
  }

  if (fn_type->kind == T_VAR) {
    // If current_type is a type variable, constrain it to be a function
    Type *ret_type = next_tvar();
    Type *fn_constraint =
        create_type_multi_param_fn(app_len, arg_types, ret_type);

    if (!unify_in_ctx(fn_type, fn_constraint, &app_ctx)) {
      fprintf(stderr, "Could not constrain type variable to function type\n");
      print_type_err(fn_type);
      print_type_err(fn_constraint);
      return NULL;
    }
    current_type = ret_type;

    for (TypeConstraint *c = app_ctx.constraints; c != NULL; c = c->next) {
      ctx->constraints = constraints_extend(ctx->constraints, c->t1, c->t2);
    }

  } else {
    for (int i = 0; i < app_len; i++) {
      Type *arg_type = arg_types[i];

      if (current_type->kind != T_FN) {
        fprintf(stderr, "Attempting to apply to non-function type\n");
        print_ast_err(ast);
        print_type(current_type);
        print_type(arg_type);
        return NULL;
      } else {

        // Regular function type case
        if (!unify_in_ctx(current_type->data.T_FN.from, arg_type, &app_ctx)) {
          fprintf(stderr, "Type mismatch in function application\n");
          print_ast_err(ast);
          print_type_err(current_type->data.T_FN.from);
          fprintf(stderr, " != ");
          print_type_err(arg_type);
          fprintf(stderr, "\n");

          return NULL;
        }

        if (!types_equal(current_type->data.T_FN.from, arg_type)) {
          ctx->current_fn_constraints = constraints_extend(
              ctx->current_fn_constraints,
              deep_copy_type(current_type->data.T_FN.from), arg_type);
        }

        current_type = current_type->data.T_FN.to;
      }
    }
  }

  Type *spec_fn;

  if (app_ctx.constraints) {
    // After processing all arguments, solve collected constraints
    Substitution *subst = solve_constraints(app_ctx.constraints);

    if (!subst) {
      fprintf(stderr, "Could not solve type constraints in application\n");
      print_ast_err(ast);
      print_type_err(fn_type);
      return NULL;
    }

    // Apply substitutions to get final type
    type = apply_substitution(subst, current_type);
    spec_fn = apply_substitution(subst, fn_type);
    ast->data.AST_APPLICATION.function->md = spec_fn;

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Type inferred = *((Type *)ast->data.AST_APPLICATION.args[i].md);

      apply_substitution_to_nodes_rec(subst,
                                      ast->data.AST_APPLICATION.args + i);

      Type *after = ast->data.AST_APPLICATION.args[i].md;

      if ((inferred.kind == T_VAR) && (!types_equal(&inferred, after))) {
        ctx->current_fn_constraints = constraints_extend(
            ctx->current_fn_constraints, deep_copy_type(&inferred), after);
      }
    }

  } else {
    type = current_type;
    ast->data.AST_APPLICATION.function->md = fn_type;
  }
  return type;
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  Type *type;

  // Create new context for lambda body
  TICtx lambda_ctx = *ctx;
  lambda_ctx.yielded_type = NULL;
  lambda_ctx.scope++;
  lambda_ctx.current_fn_ast = ast;
  lambda_ctx.current_fn_constraints = NULL;

  // Fresh type vars for each parameter
  int num_params = ast->data.AST_LAMBDA.len;
  Type **param_types = talloc(sizeof(Type *) * num_params);

  // Process parameters right to left to build up the type
  for (int i = 0; i < num_params; i++) {
    Ast *param = &ast->data.AST_LAMBDA.params[i];
    Ast *def =
        ast->data.AST_LAMBDA.defaults ? ast->data.AST_LAMBDA.defaults[i] : NULL;

    Type *param_type;
    if (def != NULL) {
      param_type = compute_type_expression(def, ctx->env);
    } else if (param->tag == AST_VOID) {
      param_type = &t_void;
    } else if (param->tag == AST_TUPLE) {
      int len = param->data.AST_LIST.len;
      Type **contained = talloc(sizeof(Type *) * len);
      for (int i = 0; i < len; i++) {
        contained[i] = next_tvar();
      }
      param_type = create_tuple_type(len, contained);
    } else {
      param_type = next_tvar();
    }

    param_types[i] = param_type;
    param_type[i].is_fn_param = true;
    lambda_ctx.env = bind_in_env(lambda_ctx.env, param, param_types[i]);
  }

  bool is_named = ast->data.AST_LAMBDA.fn_name.chars != NULL;

  // If this is a named function that can be recursive
  Type *fn_type_var = NULL;
  if (is_named) {
    // Create a type variable for the recursive function
    fn_type_var = next_tvar();
    fn_type_var->is_recursive_fn_ref = true; // Mark as recursive

    Ast rec_fn_name_binding = {
        AST_IDENTIFIER,
        {.AST_IDENTIFIER = {.value = ast->data.AST_LAMBDA.fn_name.chars,
                            .length = ast->data.AST_LAMBDA.fn_name.length}}};
    lambda_ctx.env =
        bind_in_env(lambda_ctx.env, &rec_fn_name_binding, fn_type_var);

    Type *body_type = infer(ast->data.AST_LAMBDA.body, &lambda_ctx);

    if (!body_type)
      return NULL;

    Type *actual_fn_type = body_type;
    for (int i = num_params - 1; i >= 0; i--) {
      Type *new_fn = talloc(sizeof(Type));
      new_fn->kind = T_FN;
      new_fn->data.T_FN.from = param_types[i];
      new_fn->data.T_FN.to = actual_fn_type;
      actual_fn_type = new_fn;
    }

    // Solve constraints to get concrete types
    if (lambda_ctx.constraints) {
      Substitution *subst = solve_constraints(lambda_ctx.constraints);
      if (!subst) {
        return NULL;
      }
      // print_subst(subst);
      actual_fn_type = apply_substitution(subst, actual_fn_type);
    }

    type = actual_fn_type;

  } else {
    type = infer_anonymous_lambda(ast, param_types, num_params, &lambda_ctx);
  }

  if (lambda_ctx.yielded_type) {
    type = coroutine_constructor_type_from_fn_type(type);
  }

  if (lambda_ctx.current_fn_constraints) {
    Substitution *subst = solve_constraints(lambda_ctx.current_fn_constraints);
    if (subst) {
      type = apply_substitution(subst, type);
    }
  }
  // printf("LAMBDA\n");
  // print_type(type);
  // printf("normal constraints: \n");
  // print_constraints(lambda_ctx.constraints);
  // printf("current fn constraints: \n");
  // print_constraints(lambda_ctx.current_fn_constraints);

  return type;
}

Type *infer_yield_expr(Ast *ast, TICtx *ctx) {

  Ast *yield_expr = ast->data.AST_YIELD.expr;

  infer(yield_expr, ctx);
  Type *yield_expr_type = yield_expr->md;

  if (yield_expr->tag == AST_APPLICATION &&
      strcmp(
          yield_expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
          "arithmetic") == 0) {
  }

  if (is_coroutine_type(yield_expr_type)) {
    yield_expr_type = type_of_option(fn_return_type(yield_expr_type));
  }

  if (ctx->yielded_type == NULL) {
    ctx->yielded_type = yield_expr_type;

  } else {
    Type *prev_yield_type = ctx->yielded_type;

    if (!unify_in_ctx(prev_yield_type, yield_expr_type, ctx)) {
      fprintf(stderr, "Error: yielded values must be of the same type!");
      print_type_err(prev_yield_type);
      fprintf(stderr, " != ");
      print_type_err(yield_expr_type);
      return NULL;
    }

    if (is_generic(yield_expr_type) && is_generic(prev_yield_type)) {
      ctx->current_fn_constraints = constraints_extend(
          ctx->current_fn_constraints, yield_expr_type, prev_yield_type);
    }

    ctx->yielded_type = yield_expr_type;
  }
  ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;
  return yield_expr_type;
}
