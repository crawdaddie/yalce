#include "inference.h"
#include "ht.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include <stdlib.h>
#include <string.h>

// Type variable set for tracking free variables
typedef struct TypeVarSet {
  char **vars;
  int count;
  int capacity;
} TypeVarSet;

TypeVarSet *type_var_set_new(void) {
  TypeVarSet *set = talloc(sizeof(TypeVarSet));
  set->capacity = 16;
  set->count = 0;
  set->vars = talloc(sizeof(char *) * set->capacity);
  return set;
}

void type_var_set_add(TypeVarSet *set, const char *var) {
  // Check if variable already exists
  for (int i = 0; i < set->count; i++) {
    if (strcmp(set->vars[i], var) == 0) {
      return;
    }
  }

  // Grow array if needed
  if (set->count >= set->capacity) {

    int old_cap = set->capacity;
    set->capacity *= 2;
    char **vars = talloc(sizeof(char *) * set->capacity);
    for (int i = 0; i < old_cap; i++) {
      vars[i] = set->vars[i];
    }

    set->vars = vars;
  }

  set->vars[set->count++] = strdup(var);
}

// Substitution map for type variables
typedef struct Substitution {
  Type *from; // Type variable
  Type *to;   // Replacement type
  struct Substitution *next;
} Substitution;

Type *resolve_type_in_constraints(Type *r, TypeConstraint *env);
TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2);

void typeclasses_extend(Type *t, TypeClass *tc);
Type *copy_type(Type *t);
bool occurs_check(Type *var, Type *t);
Type *substitute_type(Type *t, Type *var, Type *replacement);
bool unify(Type *t1, Type *t2, TypeConstraint **constraints);

Type *apply_substitution(Substitution *subst, Type *t);

Type *resolve_type_in_env(Type *r, TypeEnv *env);
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

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

Type *find_in_ctx(const char *name, TICtx *ctx) {
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

TypeClassResolver arithmetic_resolver(Type *this, TypeConstraint *env) {

  // Type *d1 = this->data.T_TYPECLASS_RESOLVE.dependencies[0];
  // d1 = resolve_type_in_constraints(d1, env);
  //
  // double d1_rank =
  //     get_typeclass_rank(d1, this->data.T_TYPECLASS_RESOLVE.comparison_tc);
  //
  // Type *d2 = this->data.T_TYPECLASS_RESOLVE.dependencies[1];
  // d2 = resolve_type_in_constraints(d2, env);
  // double d2_rank =
  //     get_typeclass_rank(d2, this->data.T_TYPECLASS_RESOLVE.comparison_tc);
  //
  // if (d1->kind == T_VAR && d2->kind == T_VAR) {
  //   return this;
  // } else if (d1->kind == T_VAR) {
  //   this->data.T_TYPECLASS_RESOLVE.dependencies[0] = d1;
  //   this->data.T_TYPECLASS_RESOLVE.dependencies[1] = d2;
  //   return this;
  // } else if (d2->kind == T_VAR) {
  //   this->data.T_TYPECLASS_RESOLVE.dependencies[0] = d1;
  //   this->data.T_TYPECLASS_RESOLVE.dependencies[1] = d2;
  //   return this;
  // } else if (d1_rank >= d2_rank) {
  //   return d1;
  // } else {
  //   return d2;
  // }
  return this;
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
      tuple_mems[i] = param_binding_type(ast->data.AST_LIST.items + i);
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

Type *lookup_builtin_type(const char *name, TICtx *ctx) {
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

void compare_args(Type *free_arg, Type *arg, TypeEnv **env) {

  if (free_arg->kind == T_VAR && !is_generic(arg)) {
    *env = env_extend(*env, free_arg->data.T_VAR, arg);
    *free_arg = *arg;
    return;
  }
}

Type *resolve_type_in_env(Type *r, TypeEnv *env) {
  switch (r->kind) {
  case T_VAR: {
    Type *rr = env_lookup(env, r->data.T_VAR);
    if (rr) {
      *r = *rr;
    }
    return r;
  }

  case T_CONS: {
    for (int i = 0; i < r->data.T_CONS.num_args; i++) {
      r->data.T_CONS.args[i] = resolve_type_in_env(r->data.T_CONS.args[i], env);
    }
    return r;
  }

  case T_FN: {
    r->data.T_FN.from = resolve_type_in_env(r->data.T_FN.from, env);
    r->data.T_FN.to = resolve_type_in_env(r->data.T_FN.to, env);
    return r;
  }

  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING: {
    return r;
  }
  }
  return NULL;
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

// Deep copy a type
Type *copy_type(Type *t) {
  if (!t) {
    return NULL;
  }

  Type *new_t = talloc(sizeof(Type));
  memcpy(new_t, t, sizeof(Type));

  switch (t->kind) {
  case T_VAR:
    new_t->data.T_VAR = strdup(t->data.T_VAR);
    break;

  case T_FN:
    new_t->data.T_FN.from = copy_type(t->data.T_FN.from);
    new_t->data.T_FN.to = copy_type(t->data.T_FN.to);
    break;

  case T_CONS:
    new_t->data.T_CONS.name = strdup(t->data.T_CONS.name);
    new_t->data.T_CONS.args = malloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] = copy_type(t->data.T_CONS.args[i]);
    }
    if (t->data.T_CONS.names) {
      new_t->data.T_CONS.names =
          malloc(sizeof(char *) * t->data.T_CONS.num_args);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        new_t->data.T_CONS.names[i] = strdup(t->data.T_CONS.names[i]);
      }
    }
    break;
  }

  return new_t;
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

  int len = ast->data.AST_LIST.len;
  Type *element_type = infer(ast->data.AST_LIST.items, ctx);

  Type *el_type;
  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    el_type = infer(el, ctx);

    if (!types_equal(element_type, el_type)) {
      fprintf(stderr, "Error typechecking list literal - all elements must "
                      "be of the same type\n");
      print_type_err(element_type);
      fprintf(stderr, " != ");
      print_type_err(el_type);
      return NULL;
    }
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

Type *find_highest_rank_type(Type *var, Type *current, TypeConstraint *rest) {
  TypeClass *tc = var->implements;
  if (!tc) {
    return NULL;
  }

  Type *highest = current;

  // Look through remaining constraints for same type var
  while (rest) {
    if (rest->t1->kind == T_VAR &&
        strcmp(rest->t1->data.T_VAR, var->data.T_VAR) == 0) {
      if (get_type_rank(rest->t2, tc) > get_type_rank(highest, tc)) {
        highest = rest->t2;
      }
    } else if (rest->t2->kind == T_VAR &&
               strcmp(rest->t2->data.T_VAR, var->data.T_VAR) == 0) {
      if (get_type_rank(rest->t1, tc) > get_type_rank(highest, tc)) {
        highest = rest->t1;
      }
    }
    rest = rest->next;
  }

  return highest;
}

bool unify(Type *t1, Type *t2, TypeConstraint **constraints) {
  // Handle type variables
  if (t1->kind == T_VAR) {
    if (t2->kind == T_VAR && strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0) {
      return true; // Same type variable
    }

    // If t1 has a typeclass constraint
    if (t1->implements) {

      // If t2 is a concrete type, check if it satisfies the constraint
      if (t2->kind != T_VAR) {
        if (!satisfies_tc_constraint(t2, t1->implements)) {
          fprintf(stderr, "Type doesn't satisfy typeclass constraint\n");
          return false;
        }
      } else {
        // If t2 is a type var, it inherits the constraint
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

Substitution *solve_constraints(TypeConstraint *constraints) {

  Substitution *subst = NULL;

  while (constraints) {
    Type *t1 = apply_substitution(subst, constraints->t1);
    Type *t2 = apply_substitution(subst, constraints->t2);

    if (t1->kind == T_VAR) {
      // Check for recursive types
      if (occurs_check(t1, t2)) {
        return NULL; // Infinite type error
      }

      // Look ahead for other constraints on this type var
      Type *resolved = find_highest_rank_type(t1, t2, constraints->next);

      if (resolved) {
        subst = substitutions_extend(subst, t1, resolved);
        return subst;
      } else {
        subst = substitutions_extend(subst, t1, t2);
      }

    } else if (t2->kind == T_VAR) {
      if (occurs_check(t2, t1)) {
        return NULL; // Infinite type error
      }

      subst = substitutions_extend(subst, t2, t1);
    } else if (t1->kind != t2->kind) {
      return NULL; // Type mismatch error
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

  if (t->kind == T_CONS) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      if (occurs_check(var, t->data.T_CONS.args[i])) {
        return true;
      }
    }
  }

  return false;
}

Type *apply_substitution(Substitution *subst, Type *t) {
  if (!t)
    return NULL;

  if (t->kind == T_VAR) {
    Substitution *s = subst;
    while (s) {
      if (strcmp(t->data.T_VAR, s->from->data.T_VAR) == 0) {
        return s->to;
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
  case AST_IDENTIFIER:
    // Simple variable binding - just create fresh type var
    return next_tvar();

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
      if (!elem_type)
        return NULL;

      // Create list type containing elem_type
      Type **mems = talloc(sizeof(Type));
      mems[0] = elem_type;
      Type *list_type = create_cons_type(TYPE_NAME_LIST, 1, mems);
      return list_type;
    }
    // Other application patterns...
    break;
  }
  case AST_INT:
  case AST_DOUBLE:
  case AST_BOOL:
  case AST_CHAR:
  case AST_STRING:
  case AST_VOID: {
    return infer(pattern, ctx);
  }
  }

  fprintf(stderr, "Unsupported pattern in let binding\n");
  return NULL;
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

    type = find_in_ctx(ast->data.AST_IDENTIFIER.value, ctx);

    if (type == NULL) {
      type = lookup_builtin_type(ast->data.AST_IDENTIFIER.value, ctx);
    }

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

    for (int i = 0; i < len; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = infer(member, ctx);
      cons_args[i] = mtype;
    }

    type = talloc(sizeof(Type));

    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, cons_args, len}}};

    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      char **names = talloc(sizeof(char *) * len);
      for (int i = 0; i < len; i++) {
        Ast *member = ast->data.AST_LIST.items + i;
        names[i] = member->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      }
      type->data.T_CONS.names = names;
    }
    break;
  }

  case AST_LIST: {
    type = create_list_type(ast, TYPE_NAME_LIST, ctx);
    break;
  }

  case AST_ARRAY: {
    type = create_list_type(ast, TYPE_NAME_ARRAY, ctx);
    break;
  }

  case AST_EXTERN_FN: {
    break;
  }

  case AST_YIELD: {
    break;
  }

  case AST_RECORD_ACCESS: {
    break;
  }
  case AST_APPLICATION: {
    // First infer the function type
    Type *fn_type = infer(ast->data.AST_APPLICATION.function, ctx);

    if (!fn_type) {
      fprintf(stderr, "Could not infer function type in application\n");
      return NULL;
    }

    if (!fn_type->is_recursive_fn_ref) {
      fn_type = deep_copy_type(fn_type);
    }

    Type *current_type = fn_type;

    // Process each argument
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, ctx);

      if (!arg_type) {
        fprintf(stderr, "Could not infer argument type in application\n");
        return NULL;
      }

      // If current_type is a type variable, constrain it to be a function
      if (current_type->kind == T_VAR) {
        Type *ret_type = next_tvar();
        Type *fn_constraint = type_fn(arg_type, ret_type);

        TypeConstraint *constraints = ctx->constraints;
        if (!unify(current_type, fn_constraint, &constraints)) {
          fprintf(stderr,
                  "Could not constrain type variable to function type\n");
          return NULL;
        }
        ctx->constraints = constraints;
        current_type = ret_type;
      } else if (current_type->kind != T_FN) {
        fprintf(stderr, "Attempting to apply to non-function type\n");
        return NULL;
      } else {

        // Regular function type case
        //
        TypeConstraint *constraints = ctx->constraints;
        if (!unify(arg_type, current_type->data.T_FN.from, &constraints)) {
          fprintf(stderr, "Type mismatch in function application\n");
          return NULL;
        }

        ctx->constraints = constraints;

        current_type = current_type->data.T_FN.to;
      }
    }
    // After processing all arguments, solve collected constraints
    Substitution *subst = solve_constraints(ctx->constraints);

    if (!subst) {
      fprintf(stderr, "Could not solve type constraints\n");
      return NULL;
    }

    // Apply substitutions to get final type
    type = apply_substitution(subst, current_type);
    Type *spec_fn = apply_substitution(subst, fn_type);
    ast->data.AST_APPLICATION.function->md = spec_fn;

    break;
  }

  case AST_LET: {
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
    TypeConstraint *constraints = ctx->constraints;
    if (!unify(def_type, binding_type, &constraints)) {
      fprintf(stderr, "Definition type doesn't match binding pattern\n");
      return NULL;
    }
    ctx->constraints = constraints;

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

    break;
  }

  case AST_LAMBDA: {
    // Create new context for lambda body
    TICtx lambda_ctx = *ctx;
    lambda_ctx.scope++;

    // Fresh type vars for each parameter
    int num_params = ast->data.AST_LAMBDA.len;
    Type **param_types = talloc(sizeof(Type *) * num_params);

    // Process parameters right to left to build up the type
    for (int i = 0; i < num_params; i++) {
      Ast *param = &ast->data.AST_LAMBDA.params[i];
      Type *param_type = next_tvar();
      param_types[i] = param_type;
      lambda_ctx.env = bind_in_env(lambda_ctx.env, param, param_type);
    }

    // If this is a named function that can be recursive
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      // Create a type variable for the recursive function
      Type *fn_type_var = next_tvar();
      fn_type_var->is_recursive_fn_ref = true; // Mark as recursive

      // Add to environment before inferring body
      Ast rec_fn_name_binding = {
          AST_IDENTIFIER,
          {.AST_IDENTIFIER = {.value = ast->data.AST_LAMBDA.fn_name.chars,
                              .length = ast->data.AST_LAMBDA.fn_name.length}}};
      lambda_ctx.env =
          bind_in_env(lambda_ctx.env, &rec_fn_name_binding, fn_type_var);

      // Infer body type with recursive reference available
      Type *body_type = infer(ast->data.AST_LAMBDA.body, &lambda_ctx);
      if (!body_type)
        return NULL;

      // Build up the actual function type
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
        if (!subst)
          return NULL;
        actual_fn_type = apply_substitution(subst, actual_fn_type);
      }

      // Only then consider generalization
      type = actual_fn_type; // Or generalize if still has free vars

      // // Unify the recursive type variable with the actual function type
      // if (!unify(fn_type_var, actual_fn_type, &lambda_ctx.constraints)) {
      //   fprintf(stderr, "Recursive function type mismatch\n");
      //   return NULL;
      // }
      //
      // type = actual_fn_type;
      // print_type(type);
      // print_ast(ast);
      break;

    } else {
      // Non-recursive case - same as before
      Type *body_type = infer(ast->data.AST_LAMBDA.body, &lambda_ctx);
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

      type = fn_type;
      break;
    }
  }
  case AST_MATCH: {
    Type *result = next_tvar();

    // Infer type of expression being matched
    Type *expr_type = infer(ast->data.AST_MATCH.expr, ctx);
    if (!expr_type) {
      fprintf(stderr, "Could not infer match expression type\n");
      return NULL;
    }

    // Create overall constraints list
    TypeConstraint *match_constraints = ctx->constraints;

    // Process each branch
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Ast *branch_pattern = &ast->data.AST_MATCH.branches[2 * i];
      Ast *branch_body = &ast->data.AST_MATCH.branches[2 * i + 1];

      // Create new context for this branch
      TICtx branch_ctx = *ctx;
      branch_ctx.scope++;
      branch_ctx.constraints = NULL; // Start with fresh constraints for branch

      // Infer pattern type and unify with match expression type
      Type *pattern_type = infer_pattern(branch_pattern, ctx);
      if (!pattern_type) {
        fprintf(stderr, "Could not infer pattern type in match branch\n");
        return NULL;
      }

      // Add pattern-expression constraint
      if (!unify(expr_type, pattern_type, &match_constraints)) {
        fprintf(stderr, "Pattern type doesn't match expression type\n");
        return NULL;
      }

      // Add pattern bindings to environment
      branch_ctx.env =
          bind_in_env(branch_ctx.env, branch_pattern, pattern_type);

      // Infer branch body type
      Type *branch_type = infer(branch_body, &branch_ctx);
      if (!branch_type) {
        fprintf(stderr, "Could not infer match branch body type\n");
        return NULL;
      }

      // Add result type constraint
      if (!unify(result, branch_type, &match_constraints)) {
        fprintf(stderr, "Inconsistent types in match branches\n");
        return NULL;
      }

      // Merge branch constraints with overall constraints
      // TypeConstraint *c = branch_ctx.constraints;
      // while (c) {
      //   all_constraints = constraints_extend(all_constraints, c->t1, c->t2);
      //   c = c->next;
      // }
    }
    ctx->constraints = match_constraints;

    // Solve all collected constraints
    Substitution *subst = solve_constraints(ctx->constraints);
    if (!subst) {
      fprintf(stderr, "Could not solve match constraints\n");
      return NULL;
    }

    // Apply substitutions to get final result type
    type = apply_substitution(subst, result);
    // Also apply substitutions to the expression type to propagate constraints
    expr_type = apply_substitution(subst, expr_type);
    break;
  }
  }
  ast->md = type;
  return type;
}
