#include "inference.h"
#include "ht.h"
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
  char *tname = talloc(sizeof(char));
  *tname = (char)type_var_counter;
  *tvar = (Type){T_VAR, {.T_VAR = tname}};
  type_var_counter++;
  return tvar;
}

TICtx push_ctx(TICtx *ctx) {
  return (TICtx){.env = ctx->env, .scope = ctx->scope + 1};
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
    return 1000.;
  }
  TypeClass *T = get_typeclass_by_name(t, tc->name);

  return T->rank;
}

Type *find_highest_rank_type(Type *var, Type *current, TypeConstraint *rest) {
  TypeClass *tc = var->implements;

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
      // printf("%d`%c`, ", type_var_counter, *(type->data.T_VAR) + 48);
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

  // case AST_LET: {
  //   Ast *def = ast->data.AST_LET.expr;
  //   Ast *binding = ast->data.AST_LET.binding;
  //   Ast *body = ast->data.AST_LET.in_expr;
  //
  //   Type *def_type = infer(def, ctx);
  //   if (!def_type) {
  //     return NULL;
  //   }
  //
  //   Type *gen_type;
  //   if (ctx->constraints) {
  //     // Solve constraints collected so far
  //     Substitution *subst = solve_constraints(ctx->constraints);
  //     if (!subst) {
  //       return NULL;
  //     }
  //
  //     // Apply substitution to definition type
  //     Type *solved_type = apply_substitution(subst, def_type);
  //
  //     gen_type = generalize(ctx->env, solved_type);
  //   } else {
  //     gen_type = def_type;
  //   }
  //
  //   if (body != NULL) {
  //     // Add generalized type to environment
  //     TypeEnv *new_env =
  //         env_extend(ctx->env, binding->data.AST_IDENTIFIER.value,
  //         gen_type);
  //
  //     // Create new context with extended environment
  //     TICtx new_ctx = *ctx;
  //     new_ctx.env = new_env;
  //     new_ctx.constraints = NULL; // Clear constraints
  //     new_ctx.scope++;
  //
  //     type = infer(body, &new_ctx);
  //   } else {
  //     ctx->env =
  //         env_extend(ctx->env, binding->data.AST_IDENTIFIER.value,
  //         gen_type);
  //
  //     type = gen_type;
  //   }
  //
  //   break;
  // }
  case AST_LAMBDA: {
    break;
  }

  case AST_EXTERN_FN: {
    break;
  }

  case AST_MATCH: {
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
    fn_type = deep_copy_type(fn_type);

    Type *current_type = fn_type;
    TICtx ctx = {.env = ctx.env};

    // Process each argument
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, &ctx);

      if (!arg_type) {
        fprintf(stderr, "Could not infer argument type in application\n");
        return NULL;
      }

      // Check that current type is a function type TODO: apply cons types too
      if (current_type->kind != T_FN) {
        fprintf(stderr, "Attempting to apply to non-function type\n");
        return NULL;
      }

      // Unification will handle propagating the typeclass constraints
      // because the function's parameter type (current_type->data.T_FN.from)
      // already has the typeclass constraint
      if (!unify(arg_type, current_type->data.T_FN.from, &ctx.constraints)) {
        fprintf(stderr, "Type mismatch in function application\n");
        return NULL;
      }

      // Move to the return type for next argument
      current_type = current_type->data.T_FN.to;
    }
    // After processing all arguments, solve collected constraints
    Substitution *subst = solve_constraints(ctx.constraints);

    if (!subst) {
      fprintf(stderr, "Could not solve type constraints\n");
      return NULL;
    }

    // Apply substitutions to get final type
    type = apply_substitution(subst, current_type);
    break;
  }
  }
  ast->md = type;
  return type;
}
