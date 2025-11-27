#include "./inference.h"
#include "../arena_allocator.h"
#include "../parse.h"
#include "./builtins.h"
#include "closures.h"
#include "common.h"
#include "modules.h"
#include "serde.h"
#include "types/infer_application.h"
#include "types/infer_lambda.h"
#include "types/infer_match_expression.h"
#include "types/type.h"
#include "types/type_expressions.h"
#include "types/type_ser.h"
#include "types/typeclass_resolve.h"
#include "types/unification.h"
#include <stdarg.h>
#include <string.h>

TypeList *free_vars_type(TypeList *vars, Type *t);
Type *generalize(Type *t, TICtx *ctx) {

  if (!is_generic(t)) {
    return t;
  }
  Type *scheme = t_alloc(sizeof(Type));

  *scheme = (Type){T_SCHEME,
                   {.T_SCHEME = {.vars = free_vars_type(NULL, t), .type = t}}};
  int i = 0;
  for (TypeList *tl = scheme->data.T_SCHEME.vars; tl; tl = tl->next) {
    i++;
  }
  scheme->data.T_SCHEME.num_vars = i;
  return scheme;
}
Type *apply_substitution(Subst *subst, Type *t);

Type *instantiate_type_in_env(Type *sch, TypeEnv *env) {
  if (sch->kind != T_SCHEME) {
    return sch;
  }
  Subst substs[sch->data.T_SCHEME.num_vars];
  Subst *subst = substs;
  int i = 0;
  for (TypeList *v = sch->data.T_SCHEME.vars; v; v = v->next, i++, subst++) {
    Type *env_type = env_lookup(env, v->type->data.T_VAR);

    *subst =
        (Subst){.var = v->type->data.T_VAR, .type = env_type, .next = NULL};
    if (i < sch->data.T_SCHEME.num_vars - 1) {
      subst->next = subst + 1;
    }
  }

  Type *stype = deep_copy_type(sch->data.T_SCHEME.type);
  Type *s = apply_substitution(substs, stype);
  return s;
}
Type *instantiate(Type *sch, TICtx *ctx) {

  if (sch->kind != T_SCHEME) {
    return sch;
  }

  Subst substs[sch->data.T_SCHEME.num_vars];
  Subst *subst = substs;
  int i = 0;
  for (TypeList *v = sch->data.T_SCHEME.vars; v; v = v->next, i++, subst++) {
    // Type *env_type = env_lookup(ctx->env, v->type->data.T_VAR);

    // if (!env_type) {
    //   env_type = fresh_type;
    // }

    Type *fresh_type = next_tvar();
    fresh_type->implements = v->type->implements;
    *subst =
        (Subst){.var = v->type->data.T_VAR, .type = fresh_type, .next = NULL};
    if (i < sch->data.T_SCHEME.num_vars - 1) {
      subst->next = subst + 1;
    }
  }

  Type *stype = deep_copy_type(sch->data.T_SCHEME.type);
  Type *s = apply_substitution(substs, stype);

  return s;
}

Type *empty_type() { return t_alloc(sizeof(Type)); }

static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }
Type *next_tvar() {
  Type *tvar = t_alloc(sizeof(Type));
  char *tname = t_alloc(sizeof(char) * 5);

  sprintf(tname, "`%d", type_var_counter);
  *tvar = (Type){T_VAR, {.T_VAR = tname}};
  type_var_counter++;
  return tvar;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  TypeEnv *type_ref = lookup_type_ref(env, name);
  if (type_ref) {
    return type_ref->type;
  }
  return NULL;
}

void *type_error(Ast *ast, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "Type Error: ");
  vfprintf(stderr, fmt, args);

  va_end(args);
  fprintf(stderr, " ");
  print_location(ast);
  return NULL;
}

Type *resolve_type_in_env(Type *r, TypeEnv *env) {

  if (r->closure_meta) {
    r->closure_meta = resolve_type_in_env(r->closure_meta, env);
  }

  switch (r->kind) {
  case T_VAR: {

    if (r->is_recursive_type_ref) {
      // TODO??? wtf
      return r;
    }

    Type *rr = env_lookup(env, r->data.T_VAR);

    if (rr && rr->kind == T_VAR) {
      return resolve_type_in_env(rr, env);
    }

    if (rr) {
      *r = *rr;
    }

    return r;
  }

  case T_TYPECLASS_RESOLVE: {
    bool still_generic = false;
    for (int i = 0; i < r->data.T_CONS.num_args; i++) {
      r->data.T_CONS.args[i] = resolve_type_in_env(r->data.T_CONS.args[i], env);
      if (r->data.T_CONS.args[i]->kind == T_VAR) {
        still_generic = true;
      }
    }
    if (!still_generic) {
      return resolve_tc_rank(r);
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
bool is_custom_binop_app(Ast *app, custom_binops_t *binops) {
  if (app->data.AST_APPLICATION.args->tag == AST_IDENTIFIER) {
    custom_binops_t *b = binops;
    while (b) {
      if (CHARS_EQ(app->data.AST_APPLICATION.args->data.AST_IDENTIFIER.value,
                   b->binop)) {
        return true;
      }
      b = b->next;
    }
  }
  return false;
}

Type *create_list_type(Ast *ast, const char *cons_name, TICtx *ctx) {

  if (ast->data.AST_LIST.len == 0) {
    Type *t = t_alloc(sizeof(Type));
    Type **contained = t_alloc(sizeof(Type *));
    contained[0] = next_tvar();
    *t = (Type){T_CONS, {.T_CONS = {cons_name, contained, 1}}};
    //
    // Type *t = next_tvar();
    return t;
  }

  int len = ast->data.AST_LIST.len;
  Type *el_type = infer(ast->data.AST_LIST.items, ctx);

  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *_el_type = infer(el, ctx);

    if (is_generic(_el_type)) {
      unify(_el_type, el_type, ctx);
    } else if (is_generic(el_type)) {

      unify(el_type, _el_type, ctx);
    } else if (!types_equal(el_type, _el_type)) {
      print_location(el);
      return NULL;
    }
    el_type = _el_type;
  }

  Type *type = create_list_type_of_type(el_type);
  type->data.T_CONS.name = cons_name;
  return type;
}

void add_constraint(TICtx *result, Type *var, Type *type) {

  for (Constraint *c = result->constraints; c; c = c->next) {
    if (types_equal(c->var, var) && types_equal(c->type, type)) {
      return;
    }
  }

  // printf("adding constraint???\n");
  // print_type(var);
  // print_type(type);
  Constraint *constraint = t_alloc(sizeof(Constraint));
  *constraint =
      (Constraint){.var = var, .type = type, .next = result->constraints};
  result->constraints = constraint;
}

bool occurs_check(const char *var, Type *ty) {

  if (ty == NULL) {
    return false;
  }

  switch (ty->kind) {
  case T_VAR: {
    bool chars_eq = CHARS_EQ(ty->data.T_VAR, var);
    if (chars_eq && ty->is_recursive_type_ref) {
      return false;
    }
    return chars_eq;
  }
  case T_FN: {
    return occurs_check(var, ty->data.T_FN.from) ||
           occurs_check(var, ty->data.T_FN.to);
  }
  case T_TYPECLASS_RESOLVE:
  case T_CONS: {
    for (int i = 0; i < ty->data.T_CONS.num_args; i++) {
      if (occurs_check(var, ty->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  }
  default: {
    return false;
  }
  }
}
// Add a constraint to the result
Constraint *constraints_extend(Constraint *constraints, Type *var, Type *type) {
  Constraint *constraint = t_alloc(sizeof(Constraint));
  *constraint = (Constraint){.var = var, .type = type, .next = constraints};
  return constraint;
}

// Simple constraint list merging
Constraint *merge_constraints(Constraint *list1, Constraint *list2) {
  if (!list1) {
    return list2;
  }
  if (!list2) {
    return list1;
  }

  // Find end of list1 and append list2
  Constraint *current = list1;
  while (current->next) {
    current = current->next;
  }
  current->next = list2;

  return list1;
}

void print_typeclass(TypeClass *tc) {
  printf("Type Class %s:\n", tc->name);
  if (tc->module) {
    printf("module:\n");
    print_type(tc->module);
  }
}

bool implements(Type *t, TypeClass *tc) {

  if (CHARS_EQ(tc->name, "Constructor") && tc->module) {
    for (int i = 0; i < tc->module->data.T_CONS.num_args; i++) {
      Type *m = tc->module->data.T_CONS.args[i];
      m = m->data.T_FN.from;
      if (types_equal(t, m)) {
        // printf("t can be passed to constructor\n");
        // print_type(t);
        // print_typeclass(tc);
        return true;
      }
    }
    return false;
  }

  for (TypeClass *ttc = t->implements; ttc; ttc = ttc->next) {
    if (CHARS_EQ(ttc->name, tc->name)) {
      return true;
    }
  }
  return false;
}

int unify(Type *t1, Type *t2, TICtx *unify_res) {

  if (types_equal(t1, t2)) {
    return 0;
  }

  if (IS_PRIMITIVE_TYPE(t1) && t2->kind == T_TYPECLASS_RESOLVE) {
    TypeList *free_r = free_vars_type(NULL, t2);
    for (TypeList *fr = free_r; fr; fr = fr->next) {
      add_constraint(unify_res, fr->type, t1);
    }

    return 0;
  }

  if (IS_PRIMITIVE_TYPE(t1)) {

    add_constraint(unify_res, t2, t1);
    return 0;
  }
  if (t1->implements && t2->kind != T_VAR) {

    for (TypeClass *tc = t1->implements; tc; tc = tc->next) {

      if ((!CHARS_EQ(tc->name, "Constructor")) && !implements(t2, tc)) {

        if (t2->kind == T_TYPECLASS_RESOLVE) {
          TypeList *free_vars = free_vars_type(NULL, t2);
          if (free_vars) {
            for (TypeList *l = free_vars; l; l = l->next) {
              typeclasses_extend(t2, tc);
            }
            return 0;
          } else {
            return 1;
          }
        } else {
          fprintf(stderr, "Unification Error ");
          print_type_err(t2);
          fprintf(stderr, " does not implement %s\n ", tc->name);
          return 1;
        }
      }
    }
  }
  if (t1->kind == T_VAR && t1->is_recursive_type_ref && is_sum_type(t2) &&
      (t2->alias && CHARS_EQ(t1->data.T_VAR, t2->alias))) {
    return 0;
    // printf("t2 -> alias %s", t2->alias);

    // int len = binding->data.AST_LIST.len;
    // for (int i = 0; i < len; i++) {
    //   Ast *mem = binding->data.AST_LIST.items + i;
    //   bind_type_in_ctx(mem, pattern_type->data.T_CONS.args[i], bmd_type,
    //   ctx);
    // }
    //
    // unify(type, pattern_type, ctx);
    //
    // return 0;
  }

  if (t1->kind == T_VAR && t1->is_recursive_type_ref && t2->kind == T_VAR) {

    if (occurs_check(t2->data.T_VAR, t1)) {

      return 1; // Occurs check failure
    }

    add_constraint(unify_res, t2, t1);

    return 0;
  }

  if (t1->kind == T_VAR) {

    if (occurs_check(t1->data.T_VAR, t2)) {

      return 1; // Occurs check failure
    }

    add_constraint(unify_res, t1, t2);

    return 0;
  }

  if (t2->kind == T_VAR && t1->kind == T_TYPECLASS_RESOLVE) {
    add_constraint(unify_res, t2, t1);
    return 0;
  }

  if (t2->kind == T_VAR) {
    for (TypeClass *tc = t1->implements; tc != NULL; tc = tc->next) {
      typeclasses_extend(t2, tc);
    }

    if (occurs_check(t2->data.T_VAR, t1)) {

      return 1; // Occurs check failure
    }

    add_constraint(unify_res, t2, t1);
    return 0;
  }

  // Case 3: Function types - recurse and merge constraints
  if (t1->kind == T_FN && t2->kind == T_FN) {
    // Unify parameter types
    TICtx ur1 = {};
    if (unify(t1->data.T_FN.from, t2->data.T_FN.from, &ur1) != 0) {
      // printf("fn 1st arg mismatch\n");
      // print_type(t1->data.T_FN.from);
      // print_type(t2->data.T_FN.from);

      return 1;
    }

    // Unify return types
    TICtx ur2 = {};

    if (unify(t1->data.T_FN.to, t2->data.T_FN.to, &ur2) != 0) {

      return 1;
    }

    // Merge all constraints (don't solve them)
    unify_res->constraints =
        merge_constraints(unify_res->constraints, ur1.constraints);
    unify_res->constraints =
        merge_constraints(unify_res->constraints, ur2.constraints);

    return 0;
  }

  // Case 4: Constructor types - recurse and merge constraints
  if (t1->kind == T_CONS && t2->kind == T_CONS) {

    if (is_pointer_type(t1) && is_pointer_type(t2)) {
      return 0;
    }

    if (is_pointer_type(t1) && (t1->data.T_CONS.num_args == 1) &&
        !unify(t1->data.T_CONS.args[0], t2, unify_res)) {
      return 0;
    }

    // NB: don't worry about comparing the cons names - as long as the contained
    // types match it doesn't really matter
    // if (is_list_type(t1)) {
    //   print_type(t1);
    //   print_type(t2);
    // }

    // if (t1->alias
    if (t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {

      return 1;
    }

    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      TICtx ur = {};

      if (unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], &ur) != 0) {
        return 1;
      }

      // Merge constraints from this argument
      unify_res->constraints =
          merge_constraints(unify_res->constraints, ur.constraints);
    }
    return 0;
  }

  if (t1->kind == T_TYPECLASS_RESOLVE && t2->kind != T_VAR) {
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      if (unify(t1->data.T_CONS.args[i], t2, unify_res)) {

        return 1;
      }
    }
    return 0;
  }

  // Case 5: Two concrete types - this will be handled by constraint solver
  // later
  //
  if (t1->kind != T_VAR && t2->kind != T_VAR) {

    return 0;
  }

  return 1; // Unification failure
}

Type *find_in_subst(Subst *subst, const char *name) {
  for (Subst *sx = subst; sx; sx = sx->next) {
    if (CHARS_EQ(name, sx->var)) {
      return sx->type;
    }
  }
  return NULL;
}
bool typelist_contains(TypeList *t, Type *var) {
  for (TypeList *tl = t; tl; tl = tl->next) {
    if (types_equal(tl->type, var)) {
      return true;
    }
  }
  return false;
}

Subst *subst_extend(Subst *subst, const char *key, Type *type);

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env) {
  for (TypeEnv *e = env; e; e = e->next) {
    e->type = apply_substitution(subst, e->type);
  }
  return env;
}

Type *apply_substitution(Subst *subst, Type *t) {
  if (!subst) {
    return t;
  }

  if (!t) {
    return NULL;
  }

  if (t->closure_meta) {
    t->closure_meta = apply_substitution(subst, t->closure_meta);
  }

  switch (t->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING: {
    return t;
  }

  case T_VAR: {

    if (t->is_recursive_type_ref) {
      return t;
    }
    Type *x = find_in_subst(subst, t->data.T_VAR);

    if (x) {
      // if (t->implements) {
      //   for (TypeClass *tc = t->implements; tc; tc = tc->next) {
      //     if (!type_implements(x, tc)) {
      //       return NULL;
      //     }
      //   }
      // }

      if (x->kind != T_VAR && is_generic(x)) {
        return apply_substitution(subst, x);
      }

      return x;
    }
    return t;
  }
  case T_FN: {
    Type *fr = apply_substitution(subst, t->data.T_FN.from);
    t->data.T_FN.from = fr;
    Type *to = apply_substitution(subst, t->data.T_FN.to);

    t->data.T_FN.to = to;
    // print_type(t->data.T_FN.from);
    //   printf("attributes %llu\n", t->data.)

    return t;
  }

  case T_TYPECLASS_RESOLVE: {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      Type *s = apply_substitution(subst, t->data.T_CONS.args[i]);
      if (!s) {
        return NULL;
      }
      t->data.T_CONS.args[i] = s;
    }

    if (!is_generic(t)) {
      return tc_resolve(t);
    }
    return cleanup_tc_resolve(t);
  }
  case T_CONS: {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      Type *s = apply_substitution(subst, t->data.T_CONS.args[i]);
      if (!s) {
        return NULL;
      }
      t->data.T_CONS.args[i] = s;
    }
    return t;
  }
  case T_SCHEME: {

    // for (TypeList *tl = t->data.T_SCHEME.vars; tl; tl = tl->next) {
    //   tl->type = apply_substitution(subst, tl->type);
    // }

    Type *n = apply_substitution(subst, t->data.T_SCHEME.type);
    return generalize(n, NULL);
  }
  }
  return t;
}

TypeEnv *lookup_type_ref(TypeEnv *env, const char *name) {
  for (TypeEnv *e = env; e; e = e->next) {
    if (e->name && CHARS_EQ(e->name, name)) {
      return e;
    }
  }

  return NULL;
}
TypeClass *find_typeclass(TypeClass *impls, const char *name) {
  for (TypeClass *tc = impls; tc; tc = tc->next) {
    if (CHARS_EQ(tc->name, name)) {
      return tc;
    }
  }
  return NULL;
}
Type *find_promoted_type(Type *var, Type *existing, Type *other_type) {
  if (types_equal(existing, other_type)) {
    return existing;
  }

  // Simple type promotion - prefer concrete types over variables
  if (existing->kind == T_VAR && other_type->kind != T_VAR) {
    return other_type;
  }
  if (other_type->kind == T_VAR && existing->kind != T_VAR) {
    return existing;
  }

  // Both are concrete - use typeclass ranking
  if (existing->kind != T_VAR && other_type->kind != T_VAR) {
    TypeClass *ex_tc = find_typeclass(existing->implements, "Arithmetic");
    TypeClass *other_tc = find_typeclass(other_type->implements, "Arithmetic");

    if (ex_tc && other_tc) {
      printf("existing rank %f other rank %f\n", ex_tc->rank, other_tc->rank);
      if (ex_tc->rank >= other_tc->rank) {
        return existing;
      } else {
        return other_type;
      }
    }
  }

  return other_type;
}

Type *merge_typeclass_resolve(Type *t1, Type *t2) {

  TypeClass *tc = t1->implements;
  const char *tc_name = t1->data.T_CONS.name;
  Type *m = t1->data.T_CONS.args[1];
  t1->data.T_CONS.args[1] = create_tc_resolve(tc, m, t2);

  return t1;
}
bool subst_contains(Subst *subst, const char *k, Type *t) {
  Type *found = find_in_subst(subst, k);
  return (found != NULL) && (types_equal(t, found));
}

Subst *subst_extend(Subst *s, const char *key, Type *type) {
  if (type->kind == T_VAR && CHARS_EQ(key, type->data.T_VAR)) {
    return s;
  }

  if (subst_contains(s, key, type)) {
    return s;
  }

  Subst *n = t_alloc(sizeof(Subst));
  *n = (Subst){.var = key, .type = type, .next = s};
  return n;
}

Subst *compose_subst(Subst *s1, Subst *s2) {
  for (Subst *kv = s2; kv; kv = kv->next) {
    const char *k = kv->var;

    Type *type = kv->type;
    type = apply_substitution(s1, type);
    if (!subst_contains(s1, k, type)) {
      s1 = subst_extend(s1, k, type);
    }
  }
  return s1;
}
Subst *update_substitution(Subst *subst, const char *var, Type *new_type) {
  if (new_type->kind == T_VAR) {
    return subst;
  }
  // Remove old binding if it exists
  Subst *new_subst = NULL;

  for (Subst *s = subst; s; s = s->next) {
    if (!CHARS_EQ(s->var, var)) {
      // Keep bindings for other variables
      new_subst = subst_extend(new_subst, s->var, s->type);
    }
  }

  new_subst = subst_extend(new_subst, var, new_type);
  return new_subst;
}

TypeList *typelist_extend(TypeList *tlist, Type *t) {
  TypeList *new = t_alloc(sizeof(TypeList));
  *new = (TypeList){.type = t, .next = tlist};
  return new;
}

TypeList *free_vars_type(TypeList *vars, Type *t) {

  switch (t->kind) {

  case T_VAR: {
    if (t->is_recursive_type_ref) {
      return vars;
    }
    if (!typelist_contains(vars, t)) {
      vars = typelist_extend(vars, t);
    }
    return vars;
  }

  case T_FN: {
    TypeList *from_vars = free_vars_type(vars, t->data.T_FN.from);
    TypeList *to_vars = free_vars_type(from_vars, t->data.T_FN.to);
    return to_vars;
  }

  case T_TYPECLASS_RESOLVE:
  case T_CONS: {

    TypeList *arg_vars = vars;
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      arg_vars = free_vars_type(arg_vars, t->data.T_CONS.args[i]);
    }
    return arg_vars;
  }
  default:
    return vars;
  }
}

Subst *solve_constraints___(Constraint *constraints) {
  Subst *subst = NULL;

  while (constraints) {
    Constraint *current = constraints;
    constraints = constraints->next;

    const char *var_name = current->var->data.T_VAR;

    Type *new_type = apply_substitution(subst, current->type);

    Type *existing = find_in_subst(subst, var_name);

    if (new_type->kind == T_TYPECLASS_RESOLVE) {

      for (int i = 0; i < new_type->data.T_CONS.num_args; i++) {
        new_type->data.T_CONS.args[i] =
            apply_substitution(subst, new_type->data.T_CONS.args[i]);
      }

      // If no concrete resolution possible, keep the constraint for later
      if (!existing) {
        subst = subst_extend(subst, var_name, new_type);
        continue;
      }
    }

    if (!existing) {
      subst = subst_extend(subst, var_name, new_type);
      continue;
    }

    if (existing->kind == T_TYPECLASS_RESOLVE && !is_generic(new_type)) {
      for (int i = 0; i < existing->data.T_CONS.num_args; i++) {
        if (existing->data.T_CONS.args[i]->kind == T_VAR) {
          subst = subst_extend(subst, existing->data.T_CONS.args[i]->data.T_VAR,
                               new_type);
        }
      }
      continue;
    }

    Type *existing_subst = apply_substitution(subst, existing);
    if (types_equal(existing_subst, new_type)) {
      continue;
    }

    if (existing_subst->kind == T_TYPECLASS_RESOLVE &&
        IS_PRIMITIVE_TYPE(new_type)) {

      VarList *frees = free_vars_type(NULL, existing_subst);
      for (VarList *f = frees; f; f = f->next) {
        subst = update_substitution(subst, f->var, new_type);
      }
      continue;
    }

    // Handle merging T_TYPECLASS_RESOLVE with other constraints
    if (existing_subst->kind == T_TYPECLASS_RESOLVE) {

      Type *merged_resolve = merge_typeclass_resolve(existing_subst, new_type);
      subst = update_substitution(subst, var_name, merged_resolve);
      continue;
    }

    if (new_type->kind == T_TYPECLASS_RESOLVE) {
      Type *merged_resolve = merge_typeclass_resolve(new_type, existing_subst);
      subst = update_substitution(subst, var_name, merged_resolve);
      continue;
    }

    TICtx ur = {.constraints = constraints};

    if (unify(existing_subst, new_type, &ur) != 0) {
      Type *promoted =
          find_promoted_type(current->var, existing_subst, new_type);

      if (promoted) {
        subst = update_substitution(subst, var_name, promoted);
        continue;
      } else {
        return NULL;
      }
    }

    // Apply direct substitutions
    if (ur.subst) {
      for (Subst *s = ur.subst; s; s = s->next) {
        subst = subst_extend(subst, s->var, s->type);
      }
    }

    constraints = ur.constraints;
    subst = update_substitution(subst, var_name, new_type);
    continue;
  }

  return subst;
}

Subst *solve_constraints(Constraint *constraints) {
  Subst *subst = NULL;

  while (constraints) {

    Constraint *current = constraints;
    constraints = constraints->next;

    if (current->var->kind != T_VAR) {
      continue;
    }

    const char *var_name = current->var->data.T_VAR;

    Type *new_type = apply_substitution(subst, current->type);

    Type *existing = find_in_subst(subst, var_name);

    if (new_type->kind == T_TYPECLASS_RESOLVE) {
      if (occurs_check(var_name, new_type)) {
        continue;
      }

      for (int i = 0; i < new_type->data.T_CONS.num_args; i++) {
        new_type->data.T_CONS.args[i] =
            apply_substitution(subst, new_type->data.T_CONS.args[i]);
      }

      // If no concrete resolution possible, keep the constraint for later
      if (!existing) {
        subst = subst_extend(subst, var_name, new_type);
        continue;
      }
    }

    if (!existing) {

      subst = subst_extend(subst, var_name, new_type);
      continue;
    }

    if (existing->kind == T_TYPECLASS_RESOLVE && !is_generic(new_type)) {
      for (int i = 0; i < existing->data.T_CONS.num_args; i++) {
        if (existing->data.T_CONS.args[i]->kind == T_VAR) {
          subst = subst_extend(subst, existing->data.T_CONS.args[i]->data.T_VAR,
                               new_type);
        }
      }
      continue;
    }

    Type *existing_subst = apply_substitution(subst, existing);
    if (types_equal(existing_subst, new_type)) {
      continue;
    }

    if (existing_subst->kind == T_TYPECLASS_RESOLVE &&
        IS_PRIMITIVE_TYPE(new_type)) {

      TypeList *frees = free_vars_type(NULL, existing_subst);
      for (TypeList *f = frees; f; f = f->next) {
        subst = update_substitution(subst, f->type->data.T_VAR, new_type);
      }

      continue;
    }

    // Handle merging T_TYPECLASS_RESOLVE with other constraints
    if (existing_subst->kind == T_TYPECLASS_RESOLVE) {

      Type *merged_resolve = merge_typeclass_resolve(existing_subst, new_type);
      subst = update_substitution(subst, var_name, merged_resolve);
      continue;
    }

    if (new_type->kind == T_TYPECLASS_RESOLVE) {
      Type *merged_resolve = merge_typeclass_resolve(new_type, existing_subst);
      subst = update_substitution(subst, var_name, merged_resolve);
      continue;
    }

    TICtx ur = {.constraints = constraints};

    if (unify(existing_subst, new_type, &ur) != 0) {
      Type *promoted =
          find_promoted_type(current->var, existing_subst, new_type);

      if (promoted) {

        subst = update_substitution(subst, var_name, promoted);
        continue;
      } else {
        return NULL;
      }
    }

    // Apply direct substitutions
    if (ur.subst) {
      for (Subst *s = ur.subst; s; s = s->next) {
        subst = subst_extend(subst, s->var, s->type);
      }
    }

    constraints = ur.constraints;
    subst = update_substitution(subst, var_name, new_type);
    continue;
  }

  return subst;
}
void print_constraints(Constraint *constraints) {
  printf("Collected constraints:\n");
  if (!constraints) {
    printf("  (none)\n");
    return;
  }

  for (Constraint *c = constraints; c; c = c->next) {
    printf("  %s := ", c->var->data.T_VAR);
    print_type(c->type);
  }
}
void print_subst(Subst *subst) {
  if (!subst) {
    return;
  }
  printf("substitutions:\n");
  for (Subst *s = subst; s; s = s->next) {
    printf("  %s : ", s->var);
    print_type(s->type);
  }
  printf("\n");
}

Type *extract_member_from_sum_type(Type *cons, Ast *id) {
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *mem = cons->data.T_CONS.args[i];
    if (CHARS_EQ(id->data.AST_IDENTIFIER.value, mem->data.T_CONS.name)) {
      return mem;
    }
  }
  return NULL;
}

Type *extract_member_from_sum_type_idx(Type *cons, Ast *id, int *idx) {
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *mem = cons->data.T_CONS.args[i];
    if (CHARS_EQ(id->data.AST_IDENTIFIER.value, mem->data.T_CONS.name)) {
      *idx = i;
      return mem;
    }
  }
  return NULL;
}

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {

  TypeEnv *new_env = t_alloc(sizeof(TypeEnv));
  *new_env = (TypeEnv){
      .name = name,
      .type = type,
      .next = env,
  };
  return new_env;
}

bool is_list_cons_operator(Ast *ast) {
  return (ast->tag == AST_APPLICATION) &&
         (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
          CHARS_EQ(
              ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
              "::"));
}

int bind_type_in_ctx(Ast *binding, Type *type, binding_md bmd_type,
                     TICtx *ctx) {
  switch (binding->tag) {
  case AST_INT:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_CHAR:
  case AST_BOOL: {
    if (binding->type == NULL) {
      binding->type = infer(binding, ctx);
    }
    return 0;
  }

  case AST_VOID: {
    return 0;
  }

  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(binding)) {
      binding->type = type;
      return 0;
    }

    if (bmd_type.type == BT_FN_PARAM) {
      binding->type = type;
      ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value, type);
      ctx->env->md = bmd_type;
      return 0;
    }

    Type *builtin_type =
        lookup_builtin_type(binding->data.AST_IDENTIFIER.value);

    if (builtin_type) {
      Type *existing = instantiate(builtin_type, ctx);
      binding->type = existing;
      return 0;
    }

    Type *existing = env_lookup(ctx->env, binding->data.AST_IDENTIFIER.value);

    if (existing) {
      binding->type = existing;
      return 0;
    }

    binding->type = type;
    ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value, type);
    ctx->env->md = bmd_type;
    return 0;
  }

  case AST_TUPLE: {
    Type *pattern_type = infer(binding, ctx);

    if (type->kind == T_CONS &&
        binding->data.AST_LIST.len != type->data.T_CONS.num_args) {
      // error - can't have mismatched tuple arity!
      return 1;
    }

    if (type->kind == T_CONS &&
        binding->data.AST_LIST.len == type->data.T_CONS.num_args) {

      binding->type = type;

      for (int i = 0; i < binding->data.AST_LIST.len; i++) {
        Ast *mem = binding->data.AST_LIST.items + i;
        bind_type_in_ctx(mem, type->data.T_CONS.args[i], bmd_type, ctx);
      }
      return 0;
    }

    if (type->kind == T_VAR) {

      int len = binding->data.AST_LIST.len;
      for (int i = 0; i < len; i++) {
        Ast *mem = binding->data.AST_LIST.items + i;
        bind_type_in_ctx(mem, pattern_type->data.T_CONS.args[i], bmd_type, ctx);
      }

      unify(type, pattern_type, ctx);

      return 0;
    }
    type_error(binding, "Could not create tuple binding");
    return 1;
  }

  case AST_APPLICATION: {

    if (is_list_cons_operator(binding)) {
      Ast *head = binding->data.AST_APPLICATION.args;
      Ast *rest = binding->data.AST_APPLICATION.args + 1;

      if (is_list_type(type)) {
        binding->type = type;
        bind_type_in_ctx(head, type->data.T_CONS.args[0], bmd_type, ctx);
        bind_type_in_ctx(rest, type, bmd_type, ctx);
        return 0;
      }

      if (type->kind == T_VAR) {
        Type *list_el = next_tvar();
        Type *list_type = create_list_type_of_type(list_el);
        unify(type, list_type, ctx);
        binding->type = list_type;
        bind_type_in_ctx(head, list_el, bmd_type, ctx);
        bind_type_in_ctx(rest, list_type, bmd_type, ctx);

        return 0;
      }

      type_error(binding, "Could not create list destructure binding");
      return 1;
    }
    Type *btype;

    if (binding->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
      Type *app_type = infer(binding, ctx);

      btype = app_type;

      if (is_sum_type(btype)) {
        btype = extract_member_from_sum_type(
            btype, binding->data.AST_APPLICATION.function);
        if (btype->kind == T_CONS && btype->data.T_CONS.num_args == 1) {
          btype = btype->data.T_CONS.args[0];
        }
      }

      if (btype->kind == T_CONS && binding->data.AST_APPLICATION.len == 1 &&
          (binding->data.AST_APPLICATION.args->tag == AST_TUPLE)) {
        bind_type_in_ctx(binding->data.AST_APPLICATION.args, btype, bmd_type,
                         ctx);
      } else if (btype->kind == T_CONS) {
        for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
          bind_type_in_ctx(binding->data.AST_APPLICATION.args + i,
                           btype->data.T_CONS.args[i], bmd_type, ctx);
        }
      } else {

        bind_type_in_ctx(binding->data.AST_APPLICATION.args, btype, bmd_type,
                         ctx);
      }

      binding->type = app_type;

      return 0;
    } else if (binding->data.AST_APPLICATION.function->tag ==
               AST_RECORD_ACCESS) {

      Type *app_type = infer(binding->data.AST_APPLICATION.function, ctx);

      btype = app_type;

      if (is_sum_type(btype)) {
        btype = extract_member_from_sum_type(
            btype, binding->data.AST_APPLICATION.function->data
                       .AST_RECORD_ACCESS.member);
      }

      if (btype->kind == T_CONS && binding->data.AST_APPLICATION.len == 1 &&
          (binding->data.AST_APPLICATION.args->tag == AST_TUPLE)) {
        bind_type_in_ctx(binding->data.AST_APPLICATION.args, btype, bmd_type,
                         ctx);
      } else if (btype->kind == T_CONS) {
        for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
          bind_type_in_ctx(binding->data.AST_APPLICATION.args + i,
                           btype->data.T_CONS.args[i], bmd_type, ctx);
        }
      } else {

        bind_type_in_ctx(binding->data.AST_APPLICATION.args, btype, bmd_type,
                         ctx);
      }

      binding->type = app_type;

      return 0;
    }

    return 1;
  }

  case AST_LIST: {
    if (binding->data.AST_LIST.len == 0) {
      // printf("empty list pattern\n");
      // print_type(type);
      binding->type = type;
      return 0;
    }
  }
  case AST_RANGE_EXPRESSION: {
    Ast *from = binding->data.AST_RANGE_EXPRESSION.from;
    Ast *to = binding->data.AST_RANGE_EXPRESSION.to;
    bind_type_in_ctx(from, type, bmd_type, ctx);
    bind_type_in_ctx(to, type, bmd_type, ctx);

    // print_ast(binding);
    //
    // if (type->kind == T_VAR) {
    //   printf("unify type: ");
    //   print_type(type);
    //   print_type(from->type);
    //   unify(type, from->type, ctx);
    // }

    binding->type = from->type;
    return 0;
  }

  // case AST_RECORD_ACCESS: {
  //   print_ast(binding);
  //   return 0;
  // }
  default: {
    type_error(binding, "Cannot appear in a binding");
    return 1;
  }
  }
  return 0;
}
//
//
// Let:    Γ ⊢ e₁ : τ₁    σ = gen(Γ, τ₁)    Γ, x : σ ⊢ e₂ : τ₂
//
//        ──────────────────────────────────────────────────────
//
//                     Γ ⊢ let x = e₁ in e₂ : τ₂
Type *infer_let_binding(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *body = ast->data.AST_LET.in_expr;

  Type *val_type = infer(expr, ctx);

  if (is_generic(val_type) && val_type->kind == T_FN) {
    val_type = generalize(val_type, ctx);
  }
  int binding_scope = ctx->scope;

  if (body) {
    binding_scope++;
  }

  binding_md bmd = (binding_md){
      BT_VAR,
      {.VAR = {.scope = binding_scope,
               .yield_boundary_scope =
                   (ctx->current_fn_ast &&
                    ctx->current_fn_ast->data.AST_LAMBDA.num_yields) ||
                   0}}};

  if (expr->tag == AST_EXTERN_FN) {
    bmd.type = BT_EXTERN_FN;
  }

  // if (expr->tag == AST_YIELD && ctx->current_fn_ast &&
  //     ctx->current_fn_ast->data.AST_LAMBDA.num_yields == 1) {
  //   bmd.data.VAR.yield_boundary_scope = 0;
  // }

  if (body) {
    TICtx body_ctx = *ctx;
    body_ctx.scope = binding_scope;
    if (bind_type_in_ctx(binding, val_type, bmd, &body_ctx)) {
      return NULL;
    }
    return infer(body, &body_ctx);
  }

  bind_type_in_ctx(binding, val_type, bmd, ctx);

  return val_type;
}

void handle_yield_boundary_crossing(binding_md binding_info, Ast *ast,
                                    TICtx *ctx) {
  if (!ctx->current_fn_ast) {
    return;
  }
  int binding_scope = binding_info.data.VAR.scope;
  if (binding_scope < ctx->current_fn_base_scope) {
    // not defined within current function, ignore
    return;
  }

  int yield_boundary = binding_info.data.VAR.yield_boundary_scope;
  int crosses_yield_boundary =
      ctx->current_fn_ast &&
      ctx->current_fn_ast->data.AST_LAMBDA.num_yields >
          yield_boundary; // there is a yield between the creation of this
                          // binding and its use
  if (!crosses_yield_boundary) {

    return;
  }

  // scan boundary xer list
  for (AstList *l =
           ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers;
       l; l = l->next) {
    Ast *a = l->ast;
    if (CHARS_EQ(a->data.AST_IDENTIFIER.value,
                 ast->data.AST_IDENTIFIER.value)) {
      return;
    }
  }

  ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers =
      ast_list_extend_left(
          ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers, ast);
  ctx->current_fn_ast->data.AST_LAMBDA.num_yield_boundary_crossers++;

  return;
}

// Identifier: x : σ ∈ Γ    τ = inst(σ)
//
//            ─────────────────────────
//
//                  Γ ⊢ x : τ
Type *infer_identifier(Ast *ast, TICtx *ctx) {
  const char *name = ast->data.AST_IDENTIFIER.value;
  TypeEnv *type_ref = lookup_type_ref(ctx->env, name);

  if (!type_ref) {
    Type *builtin_type = lookup_builtin_type(name);

    if (builtin_type) {
      return instantiate(builtin_type, ctx);
    }

    // return type_error(ast, "%s not found in scope\n", name);
    return next_tvar();
  }

  if (type_ref->md.type == BT_RECURSIVE_REF) {
    if (CHARS_EQ(ast->data.AST_IDENTIFIER.value, "compile")) {
      print_ast(ast);
      print_type(type_ref->type);
    }
    return type_ref->type;
  }

  if (type_ref->md.type == BT_VAR || type_ref->md.type == BT_FN_PARAM) {
    handle_yield_boundary_crossing(type_ref->md, ast, ctx);
    handle_closed_over_value(type_ref->md, ast, ctx);
  }

  return instantiate(type_ref->type, ctx);
}
bool find_trait_impl_rank(Ast *impl, double *rank) {
  if (impl->data.AST_LAMBDA.body->tag != AST_BODY) {
    return false;
  }

  Ast *r = impl->data.AST_LAMBDA.body->data.AST_BODY.stmts->ast;
  if (r->tag == AST_LET && r->data.AST_LET.binding->tag == AST_IDENTIFIER &&
      CHARS_EQ(r->data.AST_LET.binding->data.AST_IDENTIFIER.value, "rank")) {
    *rank = r->data.AST_LET.expr->data.AST_DOUBLE.value;
    return true;
  }
  return false;
}

Type *infer_yield_expr(Ast *ast, TICtx *ctx) {
  Ast *expr = ast->data.AST_YIELD.expr;

  Type *expr_type = infer(expr, ctx);
  if (!expr_type) {
    return NULL;
  }

  if (is_coroutine_type(expr_type)) {
    expr_type = expr_type->data.T_CONS.args[0];
  }

  if (ctx->yielded_type != NULL) {
    if (unify(expr_type, ctx->yielded_type, ctx)) {
      fprintf(stderr, "Error: could not unify yield expressions in function - "
                      "all yields must be of the same type\n");
      return NULL;
    }
  }
  ctx->yielded_type = expr_type;
  ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;
  return expr_type;
}

bool is_constant_closure(Ast *ast, TICtx *ctx);

// returns whether the partial-application expression can be compiled to a
// regular function rather than a closure object
//
// this is true if it's an
// application and each supplied parameter is a constant value, or another
// constant expression, or a reference to a value in the global scope
bool is_constant_expr(Ast *expr, TICtx *ctx) {
  if (expr->tag == AST_APPLICATION) {
    for (int i = 0; i < expr->data.AST_APPLICATION.len; i++) {
      Ast *arg = expr->data.AST_APPLICATION.args + i;
      if (!is_constant_expr(arg, ctx)) {
        return false;
      }
    }
    return true;
  }
  if (expr->tag >= AST_INT && expr->tag <= AST_BOOL) {
    return true;
  }

  if (expr->tag == AST_IDENTIFIER) {

    TypeEnv *type_ref =
        lookup_type_ref(ctx->env, expr->data.AST_IDENTIFIER.value);

    // printf("constant expr?");
    // print_ast(expr);
    // printf("ref meta: %d %d\n", type_ref->md.type,
    // type_ref->md.data.VAR.scope);

    if (type_ref && type_ref->md.type == BT_VAR &&
        type_ref->md.data.VAR.scope == 0) {
      return true;
    }

    return false;
  }
  return false;
}

Type *handle_closure_constants(Ast *ast, Type *type, TICtx *ctx) {
  if (!is_constant_expr(ast, ctx)) {
    return type;
  }

  int i = 0;
  Type *f = ast->data.AST_APPLICATION.function->type;
  for (; f->kind == T_FN && !is_closure(f); f = f->data.T_FN.to) {
    i++;
  }

  if (ast->data.AST_APPLICATION.len == i) {
    return type;
  }

  ast->data.AST_APPLICATION.is_curried_with_constants = true;
  type->closure_meta = NULL;
  return type;
}

Type *infer(Ast *ast, TICtx *ctx) {
  Type *type = NULL;
  switch (ast->tag) {
  case AST_BODY: {
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    Ast *stmt = l->ast;
                    Type *res = infer(stmt, ctx);
                    if (res == NULL) {
                      return type_error(stmt, "Error: typecheck failed at ");
                    }
                    type = res;
                  }));
    break;
  }

    // -------
    // Γ ⊢ Int
    //
    // --------
    // Γ ⊢ Bool
    //
    // --------
    // Γ ⊢ Double
    // ...
  case AST_INT: {
    type = &t_int;
    break;
  }

  // case AST_UINT64: {
  //   type = &t_uint64;
  //   break;
  // }
  //
  // case AST_FLOAT: {
  //   type = &t_float;
  //   break;
  // }
  case AST_DOUBLE: {
    type = &t_num;
    break;
  }
  case AST_VOID: {
    type = &t_void;
    break;
  }
  case AST_BOOL: {
    type = &t_bool;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }
  case AST_STRING: {
    type = &t_string;
    break;
  }
  case AST_FMT_STRING: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      Ast *item = ast->data.AST_LIST.items + i;
      if (infer(item, ctx) == NULL) {
        return NULL;
      }
    }
    type = &t_string;
    break;
  }
  case AST_ARRAY: {
    type = create_list_type(ast, TYPE_NAME_ARRAY, ctx);
    if (!type) {
      return NULL;
    }
    break;
  }

  case AST_LIST: {
    type = create_list_type(ast, TYPE_NAME_LIST, ctx);
    if (!type) {
      return NULL;
    }
    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;
    Type **args = t_alloc(sizeof(Type *) * arity);
    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      const char **names = t_alloc(sizeof(char *) * arity);

      // named tuple
      for (int i = 0; i < arity; i++) {
        if (ast->data.AST_LIST.items[i].data.AST_LET.binding->tag !=
            AST_IDENTIFIER) {
          return NULL;
        }
        names[i] = ast->data.AST_LIST.items[i]
                       .data.AST_LET.binding->data.AST_IDENTIFIER.value;
        TICtx _ctx = *ctx;
        _ctx.scope++;
        args[i] = infer(ast->data.AST_LIST.items[i].data.AST_LET.expr, &_ctx);
      }
      type = create_tuple_type(arity, args);
      type->data.T_CONS.names = names;
      break;
    }

    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;

      Type *member_type;
      member_type = infer(member, ctx);

      args[i] = member_type;
      if (!member_type) {
        return NULL;
      }
    }

    type = create_tuple_type(arity, args);
    break;
  }
  case AST_APPLICATION: {
    if (is_custom_binop_app(ast, ctx->custom_binops)) {

      Ast binop = *ast->data.AST_APPLICATION.args;
      Ast arg = *ast->data.AST_APPLICATION.function;

      *ast->data.AST_APPLICATION.function = binop;
      ast->data.AST_APPLICATION.args[0] = arg;
    }

    type = infer_application(ast, ctx);

    if (type && is_closure(type)) {
      handle_closure_constants(ast, type, ctx);
    }
    break;
  }
  case AST_IDENTIFIER: {
    type = infer_identifier(ast, ctx);
    break;
  }
  case AST_LET: {
    type = infer_let_binding(ast, ctx);
    break;
  }
  case AST_LAMBDA: {
    type = infer_lambda(ast, ctx);
    break;
  }
  case AST_EXTERN_FN: {
    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;

    if (sig->tag == AST_FN_SIGNATURE) {
      type = compute_type_expression(sig, ctx);
      // if (is_generic(type)) {
      //   type = generalize(type, ctx);
      // }
    }
    break;
  }
  case AST_MATCH: {
    type = infer_match_expression(ast, ctx);
    break;
  }
  case AST_TYPE_DECL: {
    type = infer_type_declaration(ast, ctx);
    break;
  }
  case AST_MODULE: {
    type = infer_inline_module(ast, ctx);
    break;
  }
  case AST_RECORD_ACCESS: {

    Type *rec_type = infer(ast->data.AST_RECORD_ACCESS.record, ctx);

    const char *member_name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;

    if (rec_type->kind == T_FN && !is_generic(rec_type)) {
      // TODO: this is dodgy - fix
      rec_type = fn_return_type(rec_type);
      ast->data.AST_RECORD_ACCESS.record->type = rec_type;
    }

    if (rec_type->kind != T_CONS) {
      fprintf(stderr, "Error: record type not cons\n");
      return NULL;
    }

    if (rec_type->kind == T_CONS && rec_type->data.T_CONS.names == NULL) {

      fprintf(stderr, "Error: record type does not have names\n");
      return NULL;
    }

    for (int i = 0; i < rec_type->data.T_CONS.num_args; i++) {

      if (CHARS_EQ(rec_type->data.T_CONS.names[i], member_name)) {
        type = rec_type->data.T_CONS.args[i];
        // printf("found type @ %d??\n", i);
        // print_type(type);
        ast->data.AST_RECORD_ACCESS.index = i;
        break;
      }
    }

    // print_type(type);
    if (type->kind == T_SCHEME) {
      type = instantiate(type, ctx);
    }

    break;
  }

  case AST_LOOP: {
    Ast let = *ast;
    // if (is_loop_of_iterable(ast)) {
    //   type = for_loop_binding(let.data.AST_LET.binding,
    //   let.data.AST_LET.expr,
    //                           let.data.AST_LET.in_expr, ctx);
    //
    //   break;
    // }
    //
    let.tag = AST_LET;
    type = infer(&let, ctx);
    ast->type = let.type;
    break;
  }
  case AST_RANGE_EXPRESSION: {
    Type *from = infer(ast->data.AST_RANGE_EXPRESSION.from, ctx);
    Type *to = infer(ast->data.AST_RANGE_EXPRESSION.to, ctx);
    unify(from, &t_int, ctx);
    unify(to, &t_int, ctx);
    type = &t_int;
    break;
  }
  case AST_YIELD: {
    type = infer_yield_expr(ast, ctx);
    break;
  }

  case AST_IMPORT: {
    const char *key = ast->data.AST_IMPORT.fully_qualified_name;
    YLCModule *mod = get_module(key);

    if (!mod) {
      fprintf(stderr, "mod %s not found\n", key);
      return NULL;
    }

    if (!mod->type) {
      type = init_import(mod)->type;
    } else {
      type = mod->type;
    }

    if (ast->data.AST_IMPORT.import_all) {
      TypeEnv *mod_env = mod->env;

      while (mod_env) {
        ctx->env = env_extend(ctx->env, mod_env->name, mod_env->type);
        ctx->env->is_opened_var = true;
        mod_env = mod_env->next;
      }

      custom_binops_t *b = mod->custom_binops;
      while (b) {
        custom_binops_t *bb = t_alloc(sizeof(custom_binops_t));
        *bb = (custom_binops_t){};

        *bb = *b;
        bb->next = ctx->custom_binops;
        ctx->custom_binops = bb;
        b = b->next;
      }
    } else {
      ctx->env = env_extend(ctx->env, ast->data.AST_IMPORT.identifier, type);
    }

    break;
  }
  case AST_TRAIT_IMPL: {
    ObjString type_name = ast->data.AST_TRAIT_IMPL.type;
    ObjString trait_name = ast->data.AST_TRAIT_IMPL.trait_name;

    TypeEnv *tref = lookup_type_ref(ctx->env, type_name.chars);
    TypeEnv _tref = {};
    if (!tref) {
      Type *x = lookup_builtin_type(type_name.chars);
      if (x) {
        _tref = (TypeEnv){.name = type_name.chars, .type = x};
        tref = &_tref;
      }
    }
    if (!tref) {
      fprintf(stderr, "Error: could not find type %s\n", type_name.chars);
      return NULL;
    }
    Type *t = tref->type;

    double rank;
    int has_rank = find_trait_impl_rank(ast->data.AST_TRAIT_IMPL.impl, &rank);

    Ast impl = *ast->data.AST_TRAIT_IMPL.impl;

    if (impl.tag == AST_LAMBDA) {
      impl.data.AST_LAMBDA.type_annotations->ast = ast_identifier(type_name);
    }

    type = infer(&impl, ctx);
    ast->data.AST_TRAIT_IMPL.impl->type = type;

    if (has_rank) {
      Type *ti = t;
      if (t->kind == T_SCHEME) {
        ti = t->data.T_SCHEME.type;
      }
      // if (ti->prototype) {
      //   ti = ti->prototype;
      // }
      TypeClass *tc = t_alloc(sizeof(TypeClass));
      *tc = (TypeClass){.rank = rank, .name = trait_name.chars, .module = type};
      tc->next = ti->implements;

      ti->implements = tc;
      tref->type = t;
      break;
    }

    Type *ti = t;
    if (t->kind == T_SCHEME) {
      ti = t->data.T_SCHEME.type;
    }
    // if (ti->prototype) {
    //   ti = ti->prototype;
    // }
    TypeClass *tc = t_alloc(sizeof(TypeClass));
    *tc = (TypeClass){.name = trait_name.chars, .module = type};
    tc->next = ti->implements;
    ti->implements = tc;
    tref->type = t;

    break;
  }

  default: {
    break;
  }
  }
  ast->type = type;
  return ast->type;
}
