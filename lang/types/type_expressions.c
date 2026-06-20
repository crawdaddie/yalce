#include "./type_expressions.h"
#include "./builtins.h"
#include "./inference.h"
#include "serde.h"
#include "types/type_ser.h"
#include <string.h>

static const char *current_type_decl_name = NULL;
static TypeEnv *current_type_decl_env = NULL;

Type *create_sum_type(int len, Type **members) {
  Type *sum = empty_type();
  sum->kind = T_SUM;
  sum->data.T_CONS.name =
      current_type_decl_name ? current_type_decl_name : TYPE_NAME_VARIANT;
  sum->data.T_CONS.args = members;
  sum->data.T_CONS.num_args = len;
  return sum;
}
int bind_type_in_ctx(Ast *binding, Type *type, binding_md bmd_type, TICtx *ctx);

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
  case AST_FN_SIGNATURE: {
    return compute_fn_type(expr, ctx);
    break;
  }
  case AST_VOID: {
    return &t_void;
  }

  case AST_IDENTIFIER: {
    const char *name = expr->data.AST_IDENTIFIER.value;
    if (current_type_decl_name && strcmp(name, current_type_decl_name) == 0) {
      return trec(name, current_type_decl_env);
    }
    // if (CHARS_EQ(name, TYPE_NAME_PTR)) {
    //   return deep_copy_type(&t_ptr);
    // }

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
    // print_ast("compute tuple??\n");
    // print_ast(expr);
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
    const char **names = malloc(sizeof(char *) * len);
    for (int i = 0; i < len; i++) {
      const char *name;
      Ast *mem_ast = expr->data.AST_LIST.items + i;
      if (mem_ast->tag == AST_IDENTIFIER) {
        name = mem_ast->data.AST_IDENTIFIER.value;
        members[i] =
            create_cons_type(mem_ast->data.AST_IDENTIFIER.value, 0, NULL);
      } else {
        Ast *item = mem_ast;

        if (item->tag == AST_BINOP &&
            item->data.AST_BINOP.left->tag == AST_IDENTIFIER) {

          name = item->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
        }

        Type *sch = compute_type_expression(expr->data.AST_LIST.items + i, ctx);
        if (!sch) {
          return NULL;
        }
        members[i] = sch;
      }

      names[i] = name;

      if (members[i]->kind == T_CONS || members[i]->kind == T_SUM) {
        members[i]->data.T_CONS.name = name;
      }
    }

    Type *sum_type = create_sum_type(len, members);
    sum_type->data.T_CONS.names = names;

    // computed->data.T_CONS.names =
    //     malloc(sizeof(char *) * computed->data.T_CONS.num_args);
    // for (int i = 0; i < expr->data.AST_LIST.len; i++) {
    //   Ast *name = i
    // }
    return sum_type;
  }

  case AST_BINOP: {
    token_type op = expr->data.AST_BINOP.op;
    if (op == TOKEN_OF) {
      Ast *container_ast = expr->data.AST_BINOP.left;
      Ast *contained_ast = expr->data.AST_BINOP.right;

      Type *container = compute_type_expression(container_ast, ctx);
      if (container->kind == T_VAR) {
        container->kind = T_CONS;
      }

      if (!container) {
        return type_error(container_ast, "could not find type");
      }

      Type *contained = compute_type_expression(contained_ast, ctx);

      if (is_pointer_type(container)) {
        container = deep_copy_type(container);
        container->data.T_CONS.args = t_alloc(sizeof(Type *));
        container->data.T_CONS.args[0] = contained;
        container->data.T_CONS.num_args = 1;
        return container;
      }

      container->data.T_CONS.args = t_alloc(sizeof(Type *));
      container->data.T_CONS.args[0] = contained;
      container->data.T_CONS.num_args = 1;

      return container;
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
    break;
  }
  }

  return NULL;
}

int bind_pattern(Ast *pattern, Type *value_type, TICtx *ctx);

Type *infer_type_declaration(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  const char *saved_type_decl_name = current_type_decl_name;
  TypeEnv *saved_type_decl_env = current_type_decl_env;
  TypeEnv *decl_env = NULL;
  if (binding->tag == AST_IDENTIFIER) {
    current_type_decl_name = binding->data.AST_IDENTIFIER.value;
    decl_env = t_alloc(sizeof(TypeEnv));
    memset(decl_env, 0, sizeof(TypeEnv));
    decl_env->name = current_type_decl_name;
    decl_env->md.type = BT_TYPE_DECL;
    current_type_decl_env = decl_env;
  }
  Type *computed = compute_type_expression(expr, ctx);
  current_type_decl_name = saved_type_decl_name;
  current_type_decl_env = saved_type_decl_env;

  if (!computed) {
    return NULL;
  }

  if (decl_env) {
    decl_env->type = computed;
  }

  if (binding->tag == AST_IDENTIFIER && computed->kind == T_CONS) {
    const char *type_name = binding->data.AST_IDENTIFIER.value;
    computed->alias = type_name;
    if (!is_sum_type(computed)) {
      computed->data.T_CONS.name = type_name;
    }
  }

  if (binding->tag == AST_IDENTIFIER && computed->kind == T_SUM) {
    const char *type_name = binding->data.AST_IDENTIFIER.value;
    computed->data.T_CONS.name = type_name;
  }

  if (bind_pattern(binding, computed, ctx)) {
    type_error(ast, "Unsupported let binding shape");
    return NULL;
  }
  if (binding->tag == AST_IDENTIFIER && ctx->env &&
      strcmp(ctx->env->name, binding->data.AST_IDENTIFIER.value) == 0) {
    ctx->env->md.type = BT_TYPE_DECL;
  }
  if (is_sum_type(computed)) {
    for (int i = 0; i < computed->data.T_CONS.num_args; i++) {
      Type *mem = computed->data.T_CONS.args[i];
      Type *ctor_type = computed;
      if (mem->kind == T_CONS && mem->data.T_CONS.num_args > 0) {
        ctor_type = create_type_multi_param_fn(mem->data.T_CONS.num_args,
                                               mem->data.T_CONS.args, computed);
      }
      ctx->env = env_extend(ctx->env, mem->data.T_CONS.name, ctor_type);
      ctx->env->md.type = BT_TYPE_CONSTRUCTOR;
    }
  }

  return computed;
}
