#include "./type_expressions.h"
#include "./builtins.h"
#include "serde.h"
#include "types/common.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include <string.h>

Type *create_sum_type(int len, Type **members) {
  return create_cons_type(TYPE_NAME_VARIANT, len, members);
}
int bind_type_in_ctx(Ast *binding, Type *type, binding_md bmd_type, TICtx *ctx);
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
  }
  case AST_BINOP: {
    token_type op = expr->data.AST_BINOP.op;
    if (op == TOKEN_OF) {
      Ast *container_ast = expr->data.AST_BINOP.left;
      Ast *contained_ast = expr->data.AST_BINOP.right;
      Type *container = compute_type_expression(container_ast, ctx);
      if (!container) {
        return type_error(container_ast, "could not find type");
      }
      Type *contained = compute_type_expression(contained_ast, ctx);

      if (container->kind == T_SCHEME &&
          container->data.T_SCHEME.num_vars == 1) {
        TypeEnv env = {.name = container->data.T_SCHEME.vars->type->data.T_VAR,
                       .type = contained};
        Type *final = instantiate_type_in_env(container, &env);
        return final;
      }
    }
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

Type *infer_type_declaration(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;
  Ast *expr = ast->data.AST_LET.expr;

  TICtx _ctx = *ctx;
  Type *t = tvar(name);
  t->is_recursive_type_ref = true;
  _ctx.env = env_extend(_ctx.env, binding->data.AST_IDENTIFIER.value, t);
  _ctx.env->md = (binding_md){
      BT_RECURSIVE_REF,
  };

  Type *computed = compute_type_expression(expr, &_ctx);
  if (expr->tag == AST_IDENTIFIER) {
    computed = deep_copy_type(computed);
  }

  if (!computed) {
    type_error(ast, "Could not compute type declaration\n");
    return NULL;
  }

  if (is_sum_type(computed)) {
    bind_type_in_ctx(binding, computed, (binding_md){}, ctx);
    for (int i = 0; i < computed->data.T_CONS.num_args; i++) {
      Type *mem = computed->data.T_CONS.args[i];
      const char *mem_name = computed->data.T_CONS.args[i]->data.T_CONS.name;
      // Ast mem_bind = (Ast){AST_IDENTIFIER,
      //                      .data = {.AST_IDENTIFIER = {.value = mem_name}}};
      ctx->env = env_extend(ctx->env, mem_name, computed);
    }
    // print_type(computed);
    return computed;
  }

  if (is_tuple_type(computed)) {
    computed->data.T_CONS.name = name;
  }
  if (is_generic(computed)) {
    computed = generalize(computed, ctx);
  }
  computed->alias = name;
  bind_type_in_ctx(binding, computed, (binding_md){}, ctx);
  return computed;
}
