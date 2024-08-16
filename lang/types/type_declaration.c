#include "types/type_declaration.h"
#include "serde.h"
#include "types/util.h"
#include <stdlib.h>
#include <string.h>

static Type *compute_type_expression(Ast *expr, TypeEnv *env);

static Type *parse_tuple(Ast *expr, TypeEnv *env) {
  int len = expr->data.AST_LIST.len;
  Type **element_types = malloc(sizeof(Type *));
  for (int i = 0; i < len; i++) {
    element_types[i] =
        compute_type_expression(expr->data.AST_LIST.items + i, env);
  }
  return create_tuple_type(element_types, len);
}

static Type *compute_type_expression(Ast *expr, TypeEnv *env) {

  if ((expr->tag == AST_BINOP) &&
      (expr->data.AST_BINOP.left->tag == AST_IDENTIFIER) &&
      (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
              TYPE_NAME_LIST) == 0)) {
    Type **ltype = malloc(sizeof(Type *));
    ltype[0] = compute_type_expression(expr->data.AST_BINOP.right, env);
    Type *t = tcons(TYPE_NAME_LIST, ltype, 1);
    return t;
  }

  switch (expr->tag) {

  case AST_FN_SIGNATURE: {
    int args_len = expr->data.AST_LIST.len - 1;
    Type **params = malloc(sizeof(Type *) * args_len);
    for (int i = 0; i < args_len; i++) {
      params[i] = compute_type_expression(expr->data.AST_LIST.items + i, env);
    }

    Type *fn_sig = create_type_multi_param_fn(
        args_len, params,
        compute_type_expression(expr->data.AST_LIST.items + args_len, env));
    return fn_sig;
  }

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

      if (is_numeric_enum) {
      }

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
    if (expr->data.AST_BINOP.op == TOKEN_OF) {

      Type *cons_type =

          compute_type_expression(expr->data.AST_BINOP.right, env);

      if (cons_type->kind == T_CONS &&
          strcmp(cons_type->data.T_CONS.name, TYPE_NAME_LIST) == 0) {

        Type **ts = malloc(sizeof(Type *));
        ts[0] = cons_type;

        Type *t =
            tcons(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value, ts, 1);

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
  case AST_TUPLE: {
    Type *t = parse_tuple(expr, env);
    return t;
  }
  case AST_VOID: {
    return &t_void;
  }

  case AST_IDENTIFIER: {
    Type *type = get_type(env, expr);
    return type;
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
  printf("type decl %s\n", name);
  print_type(type);
  printf("\n");

  *env = env_extend(*env, name, type);
  Type *lookedup = get_type(*env, binding);
}
