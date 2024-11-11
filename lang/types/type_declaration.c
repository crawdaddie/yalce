#include "type_declaration.h"
#include "serde.h"
#include <string.h>

Type *compute_type_expression(Ast *expr, TypeEnv *env);

Type *type_var_of_id(Ast *expr) {
  const char *id_chars = expr->data.AST_IDENTIFIER.value;
  Type *type = talloc(sizeof(Type));
  type->kind = T_VAR;
  type->data.T_VAR = id_chars;
  return type;
}

Type *fn_type_decl(Ast *sig, TypeEnv *env) {
  if (sig->tag == AST_FN_SIGNATURE) {
    Ast *param_ast = sig->data.AST_LIST.items;
    Type *fn = type_fn(compute_type_expression(param_ast, env),
                       fn_type_decl(param_ast + 1, env));
    return fn;
  }
  return compute_type_expression(sig, env);
}

Type *compute_concrete_type(Type *generic, Type *contained) {
  TypeEnv *env = env_extend(NULL, "t", contained);
  return resolve_generic_type(generic, env);
}

Type *coroutine_instance_type_parameter(Ast *expr) {
  Ast *components = expr->data.AST_BINOP.right;
  if (components->tag != AST_BINOP) {
    fprintf(stderr,
            "Invalid input parameters for parametrized type CorInstance");
    return NULL;
  }

  Ast *params_type_id = components->data.AST_BINOP.left;
  Type *params_type = type_var_of_id(params_type_id);
  Ast *ret_type_id = components->data.AST_BINOP.right;
  Type *ret_type = type_var_of_id(ret_type_id);

  Type *ret_opt = create_option_type(ret_type);

  Type *inst = empty_type();
  inst->kind = T_COROUTINE_INSTANCE;
  inst->data.T_COROUTINE_INSTANCE.params_type = params_type;
  inst->data.T_COROUTINE_INSTANCE.yield_interface = type_fn(&t_void, ret_opt);

  return inst;
}

Type *next_tvar();
Type *compute_type_expression(Ast *expr, TypeEnv *env) {
  switch (expr->tag) {
  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    if (len == 1) {
      return compute_type_expression(expr->data.AST_LIST.items, env);
    }

    Type *variant = empty_type();
    variant->kind = T_CONS;
    variant->data.T_CONS.name = TYPE_NAME_VARIANT;
    variant->data.T_CONS.args = talloc(sizeof(Type *) * len);
    variant->data.T_CONS.num_args = len;

    for (int i = 0; i < len; i++) {
      Ast *item = expr->data.AST_LIST.items + i;

      Type *member;
      if (item->tag == AST_IDENTIFIER) {
        member = empty_type();
        member->kind = T_CONS;
        member->data.T_CONS.args = NULL;
        member->data.T_CONS.num_args = 0;
        member->data.T_CONS.name = item->data.AST_IDENTIFIER.value;
      } else {
        member = compute_type_expression(item, env);
      }
      variant->data.T_CONS.args[i] = member;
    }
    return variant;
  }
  case AST_VOID: {
    return &t_void;
  }

  case AST_IDENTIFIER: {
    Type *type = find_type_in_env(env, expr->data.AST_IDENTIFIER.value);
    if (!type) {
      return type_var_of_id(expr);
    }
    return type;
  }

  case AST_LAMBDA: {
    TypeEnv *_env = env;
    int len = expr->data.AST_LAMBDA.len;
    Type **param_types = talloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *param = expr->data.AST_LAMBDA.params + i;
      Type *param_type = compute_type_expression(param, env);
      param_types[i] = param_type;
      _env = env_extend(_env, param_type->data.T_VAR, param_type);
    }

    Type *t = compute_type_expression(expr->data.AST_LAMBDA.body, _env);

    expr->md = create_type_multi_param_fn(len, param_types, t);
    return t;
  }
  case AST_TUPLE: {
    int arity = expr->data.AST_LIST.len;
    Type **contained_types = talloc(sizeof(Type *) * arity);
    for (int i = 0; i < arity; i++) {
      contained_types[i] =
          compute_type_expression(expr->data.AST_LIST.items + i, env);
    }
    Type *t = empty_type();
    t->kind = T_CONS;
    t->data.T_CONS.name = TYPE_NAME_TUPLE;
    t->data.T_CONS.args = contained_types;
    t->data.T_CONS.num_args = arity;
    return t;
  }
  case AST_FN_SIGNATURE: {
    Type *fn = fn_type_decl(expr, env);
    return fn;
  }

  case AST_BINOP: {
    if (expr->data.AST_BINOP.op == TOKEN_OF) {

      if (expr->data.AST_BINOP.left->tag == AST_IDENTIFIER &&
          strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                 "CorInstance") == 0) {
        return coroutine_instance_type_parameter(expr);
      }

      Type *contained_type =
          compute_type_expression(expr->data.AST_BINOP.right, env);

      const char *name = expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
      Type *lookup = env_lookup(env, name);

      if (lookup && is_generic(lookup)) {
        Type *t = copy_type(lookup);
        t = compute_concrete_type(t, contained_type);
        return t;
      }

      Type *cons = empty_type();
      cons->kind = T_CONS;
      cons->data.T_CONS.name =
          strdup(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
      cons->data.T_CONS.num_args = 1;
      cons->data.T_CONS.args = talloc(sizeof(Type *));
      cons->data.T_CONS.args[0] = contained_type;
      expr->data.AST_BINOP.right->md = contained_type;
      expr->data.AST_BINOP.right->md = contained_type;

      expr->data.AST_BINOP.left->md = type_fn(contained_type, cons);
      expr->md = cons;
      return cons;
    }
  }
  }
  return NULL;
}

Type *type_declaration(Ast *ast, TypeEnv **env) {

  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  Ast *type_expr_ast = ast->data.AST_LET.expr;
  Type *type = compute_type_expression(type_expr_ast, *env);
  if (!type) {
    fprintf(stderr, "Error computing type declaration");
    return NULL;
  }

  type->alias = name;

  *env = env_extend(*env, name, type);
  return type;
}
