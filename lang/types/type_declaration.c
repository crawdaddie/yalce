#include "type_declaration.h"
#include "types/type.h"
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
Type *option_of(Ast *expr) {}

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
    Type *type = env_lookup(env, expr->data.AST_IDENTIFIER.value);
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
    char **names = talloc(sizeof(char *) * arity);
    bool has_names = false;

    for (int i = 0; i < arity; i++) {
      Ast *item = expr->data.AST_LIST.items + i;
      if (item->tag == AST_LET) {
        has_names = true;
        contained_types[i] =
            compute_type_expression(item->data.AST_LET.expr, env);
        names[i] = item->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      } else {
        contained_types[i] = compute_type_expression(item, env);
      }
    }
    Type *t = empty_type();
    t->kind = T_CONS;
    t->data.T_CONS.name = TYPE_NAME_TUPLE;
    t->data.T_CONS.args = contained_types;
    t->data.T_CONS.num_args = arity;

    if (has_names) {
      t->data.T_CONS.names = names;
    }
    return t;
  }
  case AST_FN_SIGNATURE: {
    Type *fn = fn_type_decl(expr, env);
    return fn;
  }

  case AST_BINOP: {
    if (expr->data.AST_BINOP.op == TOKEN_OF) {

      if (expr->data.AST_BINOP.left->tag == AST_IDENTIFIER) {
        if (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                   "Option") == 0) {
          Type *contained =
              compute_type_expression(expr->data.AST_BINOP.right, env);

          Type *opt = create_option_type(contained);

          return opt;
        }

        if (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                   "Coroutine") == 0) {

          Type *r = compute_type_expression(expr->data.AST_BINOP.right, env);

          Type *cor = create_coroutine_instance_type(r);

          return cor;
        }

        // if ((strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
        //             "Ptr") == 0) &&
        //     (strcmp(expr->data.AST_BINOP.right->data.AST_IDENTIFIER.value,
        //             "_") == 0)) {
        //   Type *cons = deep_copy_type(&t_ptr);
        //   cons->data.T_CONS.args[0] = &t_char;
        //   return cons;
        // }
      }

      Type *contained_type =
          compute_type_expression(expr->data.AST_BINOP.right, env);

      const char *name = expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
      Type *lookup = env_lookup(env, name);

      if (lookup && is_generic(lookup)) {
        Type *t = deep_copy_type(lookup);
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
  Type *type;
  if (type_expr_ast != NULL) {
    type = compute_type_expression(type_expr_ast, *env);
  } else {
    // Type **cont = talloc(sizeof(Type *));
    // *cont = &t_char;
    type = create_cons_type(name, 0, NULL);
  }

  if (!type) {
    fprintf(stderr, "Error computing type declaration");
    return NULL;
  }

  // if (is_pointer_type(type)) {
  //   type->data.T_CONS.name = name;
  // }

  type->alias = name;

  *env = env_extend(*env, name, type);
  if (is_variant_type(type)) {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *member = type->data.T_CONS.args[i];
      *env = env_extend(*env, member->data.T_CONS.name, type);
    }
  }
  return type;
}
