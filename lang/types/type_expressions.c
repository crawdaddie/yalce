#include "./type_expressions.h"
#include "./builtins.h"
#include "serde.h"
#include "types/common.h"
#include "types/inference.h"
#include <string.h>

Type *create_sum_type(int len, Type **members) {
  return create_cons_type(TYPE_NAME_VARIANT, len, members);
}

// forall T : T
Scheme *create_ts_var(const char *name) {
  Scheme *sch = talloc(sizeof(Scheme));
  VarList *vars = talloc(sizeof(VarList));
  *vars = (VarList){.var = name};
  sch->vars = vars;
  sch->type = tvar(name);
  return sch;
}

Type *fn_type_decl(Ast *sig, TICtx *ctx) {
  Ast *param_ast = sig->data.AST_LIST.items;
  Type *fn = type_fn(instantiate(compute_type_expression(param_ast, ctx), ctx),
                     fn_type_decl(param_ast + 1, ctx));
  return fn;
}

Scheme *compute_type_expression(Ast *expr, TICtx *ctx) {
  switch (expr->tag) {
  case AST_VOID: {
    return &void_scheme;
  }
  case AST_IDENTIFIER: {

    Scheme *ts = lookup_scheme(ctx->env, expr->data.AST_IDENTIFIER.value);
    if (!ts) {
      return create_ts_var(expr->data.AST_IDENTIFIER.value);
    }
    return ts;
  }

  case AST_TUPLE: {
    int len = expr->data.AST_LIST.len;
    Type **members = talloc(sizeof(Type *) * len);

    VarList *vars = NULL;
    const char **names = NULL;
    if (expr->data.AST_LIST.items[0].tag == AST_LET) {
      names = talloc(sizeof(char *) * len);
    }

    for (int i = 0; i < len; i++) {

      Ast *mem_ast = expr->data.AST_LIST.items + i;

      if (mem_ast->tag == AST_LET) {
        names[i] = mem_ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;
        mem_ast = mem_ast->data.AST_LET.expr;
      }

      Scheme *mem = compute_type_expression(mem_ast, ctx);

      members[i] = mem->type;

      for (VarList *mv = mem->vars; mv; mv = mv->next) {
        varlist_add(vars, mv->var);
      }
    }

    Type *tuple_type = create_tuple_type(len, members);
    if (names) {
      tuple_type->data.T_CONS.names = names;
    }

    Scheme *sch = talloc(sizeof(Scheme));
    *sch = (Scheme){.vars = vars, .type = tuple_type};
    return sch;
  }

  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    Type **members = talloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *mem_ast = expr->data.AST_LIST.items + i;
      if (mem_ast->tag == AST_IDENTIFIER) {
        members[i] =
            create_cons_type(mem_ast->data.AST_IDENTIFIER.value, 0, NULL);
      } else {
        Scheme *sch =
            compute_type_expression(expr->data.AST_LIST.items + i, ctx);
        if (!sch) {
          return NULL;
        }
        members[i] = sch->type;
      }
    }
    Type *sum_type = create_sum_type(len, members);
    Scheme *gen = talloc(sizeof(Scheme));
    *gen = generalize(sum_type, ctx->env);
    return gen;
  }

  case AST_FN_SIGNATURE: {

    Ast *sig = expr;
    int num_params = 0;
    while (sig->tag == AST_FN_SIGNATURE || sig->tag == AST_LIST) {
      num_params++;
      sig = sig->data.AST_LIST.items + 1;
    }

    Type *it[num_params];
    Type *ret;

    int i = 0;
    sig = expr;
    while (sig->tag == AST_LIST || sig->tag == AST_FN_SIGNATURE) {
      it[i] = instantiate(
          compute_type_expression(sig->data.AST_LIST.items, ctx), ctx);
      sig = sig->data.AST_LIST.items + 1;
      i++;
    }

    ret = instantiate(compute_type_expression(sig, ctx), ctx);
    Type *f = create_type_multi_param_fn(num_params, it, ret);
    Scheme *gen = talloc(sizeof(Scheme));

    *gen = generalize(f, ctx->env);

    return gen;
  }
  case AST_BINOP: {
    token_type op = expr->data.AST_BINOP.op;
    if (op == TOKEN_OF) {

      Scheme *container = lookup_scheme(
          ctx->env, expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
      if (!container) {

        fprintf(stderr, "Error: could not find type %s\n",
                expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
        return NULL;
      }

      Type *inst =
          instantiate_with_args(container, expr->data.AST_BINOP.right, ctx);

      Scheme *sch = talloc(sizeof(Scheme));
      *sch = generalize(inst, ctx->env);
      return sch;
    }
  }

  default: {
    return NULL;
  }
  }
}

bool is_sum_type(Type *t) {
  return t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, TYPE_NAME_VARIANT);
}

Type *type_declaration(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;

  Scheme *scheme = compute_type_expression(expr, ctx);

  if (!scheme) {
    fprintf(stderr, "Error: type declaration failed\n");
    return NULL;
  }

  if (is_sum_type(scheme->type)) {
    Type *t = scheme->type;
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      const char *mem_name = t->data.T_CONS.args[i]->data.T_CONS.name;
      ctx->env = env_extend(ctx->env, mem_name, scheme->vars, scheme->type);
    }

    ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value,
                          scheme->vars, scheme->type);

  } else {
    ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value,
                          scheme->vars, scheme->type);
  }

  return scheme->type;
}
