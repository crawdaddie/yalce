#include "./type_expressions.h"
#include "./builtins.h"
#include "serde.h"
#include "types/common.h"
#include "types/inference.h"
#include <string.h>

TypeEnv *lookup_type_ref(TypeEnv *env, const char *name);
Type *compute_type_expression(Ast *expr, TICtx *ctx);
Type *create_sum_type(int len, Type **members) {

  return create_cons_type(TYPE_NAME_VARIANT, len, members);
}

typedef struct {
} TypeDeclCtx;

// forall T : T
// Scheme *create_ts_var(const char *name) {
//   Scheme *sch = talloc(sizeof(Scheme));
//   VarList *vars = talloc(sizeof(VarList));
//   *vars = (VarList){.var = name};
//   sch->vars = vars;
//   sch->type = tvar(name);
//   return sch;
// }

Type *fn_type_decl(Ast *sig, TICtx *ctx) {
  Ast *param_ast = sig->data.AST_LIST.items;
  Type *fn = type_fn(instantiate(compute_typescheme(param_ast, ctx), ctx),
                     fn_type_decl(param_ast + 1, ctx));
  return fn;
}

Type *compute_fn_type(Ast *expr, TICtx *ctx) {
  Ast *sig = expr;

  int num_params = 0;

  while (sig->tag == AST_FN_SIGNATURE || sig->tag == AST_LIST) {
    num_params++;
    sig = sig->data.AST_LIST.items + 1;
  }
  sig = expr;

  Type *param_types[num_params];
  for (int i = 0; i < num_params; i++) {
    Ast *p = sig->data.AST_LIST.items;
    Type *t = compute_type_expression(p, ctx);
    param_types[i] = t;
    sig = sig->data.AST_LIST.items + 1;
  }
  Type *ret = compute_type_expression(sig, ctx);
  Type *f = create_type_multi_param_fn(num_params, param_types, ret);
  return f;
}

Type *compute_type_expression(Ast *expr, TICtx *ctx) {
  switch (expr->tag) {
  case AST_VOID: {
    return &t_void;
  }

  case AST_IDENTIFIER: {

    const char *name = expr->data.AST_IDENTIFIER.value;

    TypeEnv *type_ref = lookup_type_ref(ctx->env, name);
    if (type_ref) {
      return type_ref->type;
    }

    Type *builtin_type = lookup_builtin_type(name);

    if (builtin_type) {
      return builtin_type;
    }
    return tvar(name);
  }

  case AST_TUPLE: {
    int len = expr->data.AST_LIST.len;
    Type **members = t_alloc(sizeof(Type *) * len);
    const char **names = NULL;
    if (expr->data.AST_LIST.items[0].tag == AST_LET) {
      names = t_alloc(sizeof(char *) * len);
    }

    for (int i = 0; i < len; i++) {

      Ast *mem_ast = expr->data.AST_LIST.items + i;

      if (mem_ast->tag == AST_LET) {
        names[i] = mem_ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;
        mem_ast = mem_ast->data.AST_LET.expr;
      }

      Type *mem = compute_type_expression(mem_ast, ctx);

      members[i] = mem;
    }

    Type *tuple_type = create_tuple_type(len, members);

    if (names) {
      tuple_type->data.T_CONS.names = names;
    }

    return tuple_type;
  }

  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    Type **members = t_alloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *mem_ast = expr->data.AST_LIST.items + i;
      if (mem_ast->tag == AST_IDENTIFIER) {
        members[i] =
            create_cons_type(mem_ast->data.AST_IDENTIFIER.value, 0, NULL);
      } else {
        Type *sch = compute_type_expression(expr->data.AST_LIST.items + i, ctx);
        if (!sch) {
          return NULL;
        }
        members[i] = sch;
      }
    }

    Type *sum_type = create_sum_type(len, members);
    return sum_type;
  }

  case AST_FN_SIGNATURE: {
    return compute_fn_type(expr, ctx);
    //
    //                Ast *
    //            sig = expr;
    // int num_params = 0;
    //
    // while (sig->tag == AST_FN_SIGNATURE || sig->tag == AST_LIST) {
    //   num_params++;
    //   sig = sig->data.AST_LIST.items + 1;
    // }
    //
    // Type *it[num_params];
    // Type *ret;
    //
    // int i = 0;
    // sig = expr;
    //
    // while (sig->tag == AST_LIST || sig->tag == AST_FN_SIGNATURE) {
    //   it[i] = instantiate(
    //       compute_type_expression(sig->data.AST_LIST.items, ctx), ctx);
    //   sig = sig->data.AST_LIST.items + 1;
    //   i++;
    // }
    // Scheme *computed_scheme = compute_type_expression(sig, ctx);
    // ret = instantiate(computed_scheme, ctx);
    // Type *f = create_type_multi_param_fn(num_params, it, ret);
    // Scheme *gen = talloc(sizeof(Scheme));
    //
    // *gen = generalize(f, ctx->env);
    //
    // return gen;
  }
  case AST_BINOP: {
    token_type op = expr->data.AST_BINOP.op;
    // if (op == TOKEN_OF) {
    //
    //   Scheme *container = lookup_scheme(
    //       ctx->env, expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
    //
    //   if (!container) {
    //
    //     fprintf(stderr, "Error: could not find type %s\n",
    //             expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
    //     return NULL;
    //   }
    //
    //   Type *contained =
    //       compute_type_expression(expr->data.AST_BINOP.right, ctx);
    //
    //   Type *inst = instantiate_scheme_with_args(
    //       container, expr->data.AST_BINOP.right, ctx);
    //
    //   return inst;
    // }
  }

  default: {
    return NULL;
  }
  }
}

// bool is_sum_type(Type *t) {
//   return t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name,
//   TYPE_NAME_VARIANT);
// }

Type *compute_typescheme(Ast *expr, TICtx *ctx) {
  Type *computed = compute_type_expression(expr, ctx);
  return generalize(computed, ctx->env);
}

/*
Type *type_declaration(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;
  Ast *expr = ast->data.AST_LET.expr;

  TICtx _ctx = *ctx;

  // bind var name in case we have a recursive ref:
  // eg type Tree = (val: Int, children: Array of Tree)
  // Scheme *sch = create_ts_var(name);
  Type *t = tvar(name);
  t->is_recursive_type_ref = true;
  _ctx.env = env_extend(_ctx.env, binding->data.AST_IDENTIFIER.value, NULL, t);

  _ctx.env->md = (binding_md){
      BT_RECURSIVE_REF,
  };

  Scheme *scheme = compute_typescheme(expr, &_ctx);

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

    if (binding->tag == AST_IDENTIFIER) {
      scheme->type->alias = binding->data.AST_IDENTIFIER.value;

      if (scheme->type->kind == T_CONS) {
        scheme->type->data.T_CONS.name = binding->data.AST_IDENTIFIER.value;
      }
    }
    ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value,
                          scheme->vars, scheme->type);
  }

  // printf("type decl scheme: ");
  // print_typescheme(*scheme);
  return scheme->type;
}
*/
