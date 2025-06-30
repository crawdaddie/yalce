#include "./unification.h"
#include "types/common.h"
#include "types/type.h"
#include <string.h>
void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...);
bool constraints_match(TypeConstraint *constraints, Type *t1, Type *t2) {
  for (TypeConstraint *c = constraints; c; c = c->next) {
    Type *_t1 = c->t1;
    Type *_t2 = c->t2;
    if (types_equal(t1, _t1) && types_equal(t2, _t2)) {
      return true;
    }
  }
  return false;
}

TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2) {
  if (constraints_match(constraints, t1, t2)) {
    return constraints;
  }

  TypeConstraint *c = talloc(sizeof(TypeConstraint));
  c->t1 = t1;
  c->t2 = t2;
  c->next = constraints;
  return c;
}

void print_constraints(TypeConstraint *c) {
  if (!c) {
    return;
  }
  printf("constraints:\n");
  for (TypeConstraint *con = c; con != NULL; con = con->next) {

    if (con->t1->kind == T_VAR) {
      printf("%s : ", con->t1->data.T_VAR);
      print_type(con->t2);
    } else {
      print_type(con->t1);
      print_type(con->t2);
    }
    // if (con->src) {
    //   print_ast(con->src);
    // };
  }
}

Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node) {

  if (types_equal(t1, t2)) {
    return t1;
  }
  // if (t1->kind == T_VAR && t2->kind != T_VAR) {
  //   ctx->constraints = constraints_extend(ctx->constraints, t2, t1);
  //   ctx->constraints->src = node;
  //   return t2;
  // }

  if (IS_PRIMITIVE_TYPE(t1)) {
    ctx->constraints = constraints_extend(ctx->constraints, t2, t1);
    ctx->constraints->src = node;
    return t1;
  }

  if (!is_generic(t2)) {
    for (TypeClass *tc = t1->implements; tc; tc = tc->next) {
      if (!type_implements(t2, tc)) {
        // print_type(t1);
        // print_type(t2);
        return type_error(
            ctx, node,
            "Typecheck error type %s does not implement typeclass '%s' \n",
            t2->alias, tc->name);
      }
    }
  }
  if (is_list_type(t2) && t2->data.T_CONS.args[0]->kind == T_VAR &&
      is_list_type(t1)) {

    ctx->constraints = constraints_extend(
        ctx->constraints, t2->data.T_CONS.args[0], t1->data.T_CONS.args[0]);
    ctx->constraints->src = node;
    return t1;
  }

  if (t2->kind == T_VAR) {
    for (TypeClass *tc = t1->implements; tc; tc = tc->next) {
      typeclasses_extend(t2, tc);
    }
  }

  if (is_pointer_type(t1) && is_pointer_type(t2)) {
    return t1;
  }

  if (t1->kind == T_CONS && t2->kind == T_CONS &&
      (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) == 0)) {

    int num_args = t1->data.T_CONS.num_args;
    for (int i = 0; i < num_args; i++) {

      Type *_t1 = t1->data.T_CONS.args[i];
      Type *_t2 = t2->data.T_CONS.args[i];

      if (!unify_in_ctx(_t1, _t2, ctx, node)) {
        return NULL;
      }
    }
  } else if (t1->kind == T_FN && t2->kind == T_FN) {

    unify_in_ctx(t1->data.T_FN.from, t2->data.T_FN.from, ctx, node);
    return unify_in_ctx(t1->data.T_FN.to, t2->data.T_FN.to, ctx, node);

  } else if (t1->kind == T_CONS && IS_PRIMITIVE_TYPE(t2)) {
    // printf("unify fail\n");
    // print_type(t1);
    // print_type(t2);
    return t2;
  } else if (t2->kind == T_VAR && t1->kind != T_VAR) {
    return unify_in_ctx(t2, t1, ctx, node);
  } else {
    ctx->constraints = constraints_extend(ctx->constraints, t1, t2);
    ctx->constraints->src = node;
  }

  return t1;
}

void print_subst(Substitution *c) {
  if (!c) {
    return;
  }
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

bool occurs_check(Type *var, Type *t) {
  if (!t) {
    return false;
  }

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

  if (!t) {
    return NULL;
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
    Substitution *s = subst;

    while (s) {
      if (types_equal(s->from, t)) {
        // printf("\n\n");
        // print_type(t);
        // printf("apply subst\n");
        // print_type(subst->from);
        // printf(" -> ");
        // print_type(subst->to);
        Type *to = s->to;
        // if (to->kind == T_CONS &&
        //     CHARS_EQ(t->data.T_VAR, to->data.T_CONS.name)) {
        //   return to;
        // }

        if (is_generic(to)) {
          Type *t = apply_substitution(subst, to);
          // printf("returning t\n");
          // print_type(t);
          return t;
        } else {
          return to;
        }
      }
      s = s->next;
    }
    break;
  }
  case T_TYPECLASS_RESOLVE: {
    // printf("apply subst tc resolve\n");
    // print_type(t);
    // print_subst(subst);

    Type *new_t = talloc(sizeof(Type));

    *new_t = *t;

    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return resolve_tc_rank(new_t);
  }

  case T_CONS: {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return new_t;
  }
  case T_FN: {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_FN.from = apply_substitution(subst, t->data.T_FN.from);
    new_t->data.T_FN.to = apply_substitution(subst, t->data.T_FN.to);
    return new_t;
  }
  default: {
    // printf("apply subst\n");
    // print_type(t);
    // print_subst(subst);
    break;
  }
  }
  return t;
}

Type *tcr_make_non_degenerate(Type *t) {
  if (t->kind == T_TYPECLASS_RESOLVE &&
      types_equal(t->data.T_CONS.args[0], t->data.T_CONS.args[1])) {
    return t->data.T_CONS.args[0];
  }
  return t;
}

Substitution *substitutions_extend(Substitution *subst, Type *t1, Type *t2) {
  t2 = tcr_make_non_degenerate(t2);

  if (types_equal(t1, t2)) {
    return subst;
  }
  if (!is_generic(t1) && is_generic(t2)) {

    return substitutions_extend(subst, t2, t1);
  }
  if (IS_PRIMITIVE_TYPE(t1) && !IS_PRIMITIVE_TYPE(t2)) {
    return substitutions_extend(subst, t2, t1);
  }

  Substitution *new_subst = talloc(sizeof(Substitution));
  new_subst->from = t1;
  new_subst->to = t2;
  new_subst->next = subst;

  return new_subst;
}

bool cons_types_match(Type *t1, Type *t2) {

  return (t1->kind == T_CONS) && (t2->kind == T_CONS) &&
         (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) == 0);
}

Substitution *solve_constraints(TypeConstraint *constraints) {
  Substitution *subst = NULL;
  // if (constraints && constraints->t1->kind == T_VAR &&
  //     CHARS_EQ(constraints->t1->data.T_VAR, "`44")) {
  //   printf("solve constraints\n");
  //   print_type(constraints->t1);
  //   print_type(constraints->t2);
  // }

  while (constraints) {

    Type *t1 = apply_substitution(subst, constraints->t1);
    Type *t2 = apply_substitution(subst, constraints->t2);

    if (!t1 || !t2) {
      constraints = constraints->next;
      continue;
    }

    if (t1->kind == T_CONS && ((1 << t2->kind) & TYPE_FLAGS_PRIMITIVE)) {

      TICtx _ctx = {.err_stream = NULL};
      return type_error(
          &_ctx, constraints->src,
          "Cannot constrain cons type to primitive simple type\n");
    }

    if (t1->kind == t2->kind && IS_PRIMITIVE_TYPE(t1)) {
      constraints = constraints->next;
      continue;
    }

    if (t1->kind == T_VAR) {
      if (occurs_check(t1, t2)) {
        constraints = constraints->next;
        continue;
      }

      subst = substitutions_extend(subst, t1, t2);
    } else if (t2->kind == T_VAR) {
      if (occurs_check(t2, t1)) {
        constraints = constraints->next;
        continue;
      }

      subst = substitutions_extend(subst, t2, t1);

    } else if (IS_PRIMITIVE_TYPE(t1) && t2->kind == T_TYPECLASS_RESOLVE) {
      for (int i = 0; i < t2->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t2->data.T_CONS.args[i], t1);
      }
    } else if (IS_PRIMITIVE_TYPE(t2) && t1->kind == T_TYPECLASS_RESOLVE) {

      for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t1->data.T_CONS.args[i], t2);
      }
    } else if (cons_types_match(t1, t2)) {
      if (is_variant_type(t1) && is_variant_type(t2)) {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          Type *mem1 = t1->data.T_CONS.args[i];
          Type *mem2 = t2->data.T_CONS.args[i];

          if (mem1->kind == T_CONS && mem1->data.T_CONS.num_args > 0) {
            TypeConstraint *next = talloc(sizeof(TypeConstraint));
            next->next = constraints->next;
            next->t1 = mem1;
            next->t2 = mem2;
            next->src = constraints->src;
            constraints->next = next;
            continue;
          }
        }
      } else if (is_generic(t1) && (!is_generic(t2))) {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          subst = substitutions_extend(subst, t1->data.T_CONS.args[i],
                                       t2->data.T_CONS.args[i]);
        }
      } else {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          if (!is_pointer_type(t2)) {
            subst = substitutions_extend(subst, t1->data.T_CONS.args[i],
                                         t2->data.T_CONS.args[i]);
          }
        }
      }
    } else if (t1->kind == T_FN && t2->kind == T_FN) {
      Type *f1 = t1;
      Type *f2 = t2;
      while (f1->kind == T_FN && f2->kind == T_FN) {
        subst =
            substitutions_extend(subst, f1->data.T_FN.from, f2->data.T_FN.from);

        f1 = f1->data.T_FN.to;
        f2 = f2->data.T_FN.to;
      }

      subst = substitutions_extend(subst, f1, f2);
    } else if (t1->kind == T_EMPTY_LIST && is_list_type(t2)) {
      *t1 = *t2;
    } else if (is_pointer_type(t1) && t2->kind == T_FN) {
    } else if (is_pointer_type(t1) && t2->kind == T_CONS) {
    } else if (is_coroutine_type(t1) && t2->kind == T_VOID) {
    } else if (is_coroutine_type(t1) && is_pointer_type(t2)) {
    } else {

      TICtx _ctx = {.err_stream = NULL};
      type_error(&_ctx, constraints->src, "Constraint solving type mismatch\n");
      print_type_err(t1);
      fprintf(stderr, " != ");
      print_type_err(t2);
    }

    constraints = constraints->next;
  }

  return subst;
}
