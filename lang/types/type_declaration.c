#include "types/type_declaration.h"
#include "serde.h"
#include "types/util.h"
#include <stdlib.h>
#include <string.h>

static Type *compute_type_expression(Ast *expr, TypeEnv *env);

static Type *parse_tuple(Ast *binop, TypeEnv *env) {
  Ast *left = binop->data.AST_BINOP.left;
  Ast *right = binop->data.AST_BINOP.right;

  if (left->tag != AST_BINOP) {
    Type **types = malloc(sizeof(Type *) * 2);
    types[0] = compute_type_expression(left, env);
    types[1] = compute_type_expression(right, env);
    Type *tuple = create_tuple_type(types, 2);
    return tuple;
  }

  Type *tuple = parse_tuple(left, env);
  Type *extra = compute_type_expression(right, env);

  size_t len = tuple->data.T_CONS.num_args;
  tuple->data.T_CONS.args = realloc(tuple->data.T_CONS.args, len + 1);
  tuple->data.T_CONS.num_args++;
  tuple->data.T_CONS.args[len] = extra;

  return tuple;
}

static Type *compute_type_expression(Ast *expr, TypeEnv *env) {
  switch (expr->tag) {

  case AST_LIST: {
    int len = expr->data.AST_LIST.len;

    if (len == 1) {
      return compute_type_expression(expr->data.AST_LIST.items, env);
    }

    Type *var = malloc(sizeof(Type));
    var->kind = T_VARIANT;
    var->data.T_VARIANT.num_args = len;
    var->data.T_VARIANT.args = malloc(sizeof(Type *) * len);

    bool is_numeric_enum = false;
    if (expr->data.AST_LIST.items[0].tag == AST_LET) {
      is_numeric_enum = true;
    }

    int enum_max = -1;

    for (int i = 0; i < len; i++) {
      Ast *item = expr->data.AST_LIST.items + i;

      Type *t;
      if (item->tag == AST_LET) {
        int enum_val = item->data.AST_LET.expr->data.AST_INT.value;

        if (enum_val <= enum_max) {
          fprintf(stderr, "enum literal values must be non-negative & "
                          "incrementing\n");
          return NULL;
        };

        enum_max = enum_val;
        item = item->data.AST_LET.binding;

      } else {
        enum_max++;
      }

      if (item->tag != AST_IDENTIFIER) {
        t = compute_type_expression(item, env);
      } else {
        t = tvar(item->data.AST_IDENTIFIER.value);
      }

      // if (is_numeric_enum) {
      // }

      var->data.T_VARIANT.args[i] = t;
    }

    return var;
  }

  case AST_APPLICATION: {
    Type *tc_type =
        compute_type_expression(expr->data.AST_APPLICATION.function, env);

    if (!tc_type || !(tc_type->kind == T_TYPECLASS)) {
      return NULL;
    }

    Ast *type_ast = expr->data.AST_APPLICATION.args;
    Type *type = compute_type_expression(type_ast, env);
    add_typeclass_impl(type, tc_type->data.T_TYPECLASS);
    return type;
  }

  case AST_BINOP: {

    if ((expr->data.AST_BINOP.left->tag == AST_IDENTIFIER) &&
        (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                TYPE_NAME_LIST) == 0)) {
      Type **ltype = malloc(sizeof(Type *));
      ltype[0] = compute_type_expression(expr->data.AST_BINOP.right, env);
      Type *t = tcons(TYPE_NAME_LIST, ltype, 1);
      return t;
    }

    if (expr->data.AST_BINOP.op == TOKEN_OF) {
      Type *cons_type =
          compute_type_expression(expr->data.AST_BINOP.right, env);

      if (cons_type->kind == T_VAR) {
        Type *t = tcons(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                        &cons_type, 1);
        return t;
      }

      if (cons_type->kind == T_CONS) {
        Type *t =
            tcons(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                  cons_type->data.T_CONS.args, cons_type->data.T_CONS.num_args);

        return t;
      }
    }
  }

  case AST_IDENTIFIER: {
    Type *type = get_type(env, expr);
    return type;
  }
  case AST_TUPLE: {
    int len = expr->data.AST_LIST.len;
    Type **types = malloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *it = expr->data.AST_LIST.items + i;
      types[i] = compute_type_expression(it, env);
    }
    return create_tuple_type(types, len);
  }
  case AST_LAMBDA: {
    TypeEnv *_env = env;
    for (int i = 0; i < expr->data.AST_LAMBDA.len; i++) {
      Ast *param_ast = expr->data.AST_LAMBDA.params + i;
      const char *param_name = param_ast->data.AST_IDENTIFIER.value;
      Type *param_type = tvar(param_ast->data.AST_IDENTIFIER.value);
      _env = env_extend(_env, param_name, param_type);
    }
    return compute_type_expression(expr->data.AST_LAMBDA.body, _env);
  }
  }
  return NULL;
}

void type_declaration(Ast *ast, TypeEnv **env) {
  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  Ast *type_expr_ast = ast->data.AST_LET.expr;
  Type *type = compute_type_expression(type_expr_ast, *env);
  type->alias = name;

  *env = env_extend(*env, name, type);
}
