#include "type_declaration.h"
#include "serde.h"
#include "types/common.h"
#include "types/inference.h"
#include "types/type.h"
#include <stdarg.h>
#include <string.h>

void *_type_error(Ast *node, const char *fmt, ...) {
  FILE *err_stream = stderr;
  va_list args;
  va_start(args, fmt);

  vfprintf(err_stream, fmt, args);
  if (node && node->loc_info) {
    _print_location(node, err_stream);
  } else if (node) {
    print_ast_err(node);
  }
  va_end(args);
  return NULL;
}

Type *env_lookup(TypeEnv *env, const char *name);

Type *type_var_of_id(const char *id_chars) {
  Type *type = talloc(sizeof(Type));
  type->kind = T_VAR;
  type->data.T_VAR = id_chars;
  return type;
}

Type *fn_type_decl(Ast *sig, TDCtx *ctx) {
  if (sig->tag == AST_FN_SIGNATURE) {
    Ast *param_ast = sig->data.AST_LIST.items;
    Type *fn = type_fn(compute_type_expression(param_ast, ctx),
                       fn_type_decl(param_ast + 1, ctx));
    return fn;
  }
  return compute_type_expression(sig, ctx);
}

Type *compute_concrete_type(Type *generic, Type *contained) {
  TypeEnv *env = env_extend(NULL, "t", contained);
  return resolve_generic_type(generic, env);
}

Type *option_of(Ast *expr) {}

Type *next_tvar();
const char *binding_name;
const char *last_ptr_type;

bool can_hold_recursive_ref(const char *container_type_name) {
  if (CHARS_EQ(container_type_name, TYPE_NAME_LIST)) {
    return true;
  }

  if (CHARS_EQ(container_type_name, TYPE_NAME_ARRAY)) {
    return true;
  }
  return false;
}

Type *_rec_generic_cons(Type *t, TypeEnv **env);

Type *compute_type_expression(Ast *expr, TDCtx *ctx) {
  // TypeEnv _env;
  // TypeEnv *__env = &_env;
  // if (env == NULL) {
  //   env = __env;
  // }

  switch (expr->tag) {
  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    if (len == 1) {
      return compute_type_expression(expr->data.AST_LIST.items, ctx);
    }
    Type *variant = empty_type();
    variant->kind = T_CONS;
    variant->data.T_CONS.name = TYPE_NAME_VARIANT;
    variant->data.T_CONS.args = talloc(sizeof(Type *) * len);
    variant->data.T_CONS.num_args = len;
    last_ptr_type = TYPE_NAME_VARIANT;

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
        member = compute_type_expression(item, ctx);
      }
      variant->data.T_CONS.args[i] = member;
    }
    return variant;
  }
  case AST_VOID: {
    return &t_void;
  }
  case AST_RECORD_ACCESS: {
    Ast *rec = expr->data.AST_RECORD_ACCESS.record;
    Ast *mem = expr->data.AST_RECORD_ACCESS.member;
    Type *rec_type = env_lookup(ctx->env, rec->data.AST_IDENTIFIER.value);

    if (!rec_type) {
      fprintf(stderr, "Error - no record with name %s\n",
              rec->data.AST_IDENTIFIER.value);
      return NULL;
    }
    for (int i = 0; i < rec_type->data.T_CONS.num_args; i++) {
      Type *m = rec_type->data.T_CONS.args[i];
      if (m->kind == T_CONS &&
          CHARS_EQ(m->data.T_CONS.name, mem->data.AST_IDENTIFIER.value)) {
        return m;
      }
    }

    // Type *t = infer(expr, ctx)

    break;
  }

  case AST_IDENTIFIER: {

    if (binding_name &&
        CHARS_EQ(expr->data.AST_IDENTIFIER.value, binding_name)) {

      if (!can_hold_recursive_ref(last_ptr_type)) {
        return _type_error(
            expr,
            "Type %s cannot have a direct recursive reference to itself "
            "- try wrapping in a container type like List or Array",
            binding_name);
      }

      const char *id_chars = expr->data.AST_IDENTIFIER.value;
      Type *new_var_type = type_var_of_id(id_chars);
      new_var_type->is_recursive_type_ref = true;
      return new_var_type;
      // Type *rec = empty_type();
      // rec->kind = T_RECURSIVE_TYPE_REF;
      // return rec;
    }

    Type *type = env_lookup(ctx->env, expr->data.AST_IDENTIFIER.value);
    if (!type) {
      const char *id_chars = expr->data.AST_IDENTIFIER.value;
      Type *new_var_type = type_var_of_id(id_chars);
      return new_var_type;
    }
    return type;
  }

  case AST_LAMBDA: {
    int len = expr->data.AST_LAMBDA.len;
    Type **param_types = talloc(sizeof(Type *) * len);

    AST_LIST_ITER(expr->data.AST_LAMBDA.params, ({
                    Ast *param = l->ast;
                    Type *param_type = compute_type_expression(param, ctx);
                    param_types[i] = param_type;
                    ctx->env = env_extend(ctx->env, param_type->data.T_VAR,
                                          param_type);
                  }));

    Type *t = compute_type_expression(expr->data.AST_LAMBDA.body, ctx);

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
            compute_type_expression(item->data.AST_LET.expr, ctx);

        names[i] = item->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      } else {
        contained_types[i] = compute_type_expression(item, ctx);
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
    Type *fn = fn_type_decl(expr, ctx);
    return fn;
  }

  case AST_BINOP: {
    if (expr->data.AST_BINOP.op == TOKEN_OF) {

      if (expr->data.AST_BINOP.left->tag != AST_IDENTIFIER) {
        fprintf(stderr, "Error - wrong lhs of Of binop\n");
        return NULL;
      }

      Type *cons = empty_type();
      cons->kind = T_CONS;
      cons->data.T_CONS.name =
          strdup(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
      const char *_prev_last_ptr_type = last_ptr_type;
      last_ptr_type = cons->data.T_CONS.name;

      if (expr->data.AST_BINOP.left->tag == AST_IDENTIFIER) {
        if (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                   "Option") == 0) {
          Type *contained =
              compute_type_expression(expr->data.AST_BINOP.right, ctx);

          Type *opt = create_option_type(contained);

          return opt;
        }

        if (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                   "Coroutine") == 0) {

          Type *r = compute_type_expression(expr->data.AST_BINOP.right, ctx);

          Type *cor = create_coroutine_instance_type(r);

          return cor;
        }

        if (strcmp(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value,
                   TYPE_NAME_PTR) == 0) {

          Type *r = compute_type_expression(expr->data.AST_BINOP.right, ctx);
          Type **args = talloc(sizeof(Type *));
          *args = r;

          Type *ptr_of = create_cons_type(TYPE_NAME_PTR, 1, args);

          return ptr_of;
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
          compute_type_expression(expr->data.AST_BINOP.right, ctx);

      const char *name = expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
      Type *lookup = env_lookup(ctx->env, name);

      if (lookup && lookup->kind == T_CREATE_NEW_GENERIC &&
          lookup->data.T_CREATE_NEW_GENERIC.fn &&
          lookup->data.T_CREATE_NEW_GENERIC.template) {

        Type *tpl = lookup->data.T_CREATE_NEW_GENERIC.template;
        tpl = deep_copy_type(tpl);

        // tpl = _rec_generic_cons(tpl, up);
        return tpl;
      }
      // printf("do something special with %s ", name);
      // print_type(lookup);
      // print_type(gen);
      // print_type(tpl);
      // return gen;

      // if (tpl->kind == T_CONS) {
      //   for (int i = 0; i < tpl->data.T_CONS.num_args; i++) {
      //     print_type(tpl->data.T_CONS.args[i]);
      //   }
      // }

      // } else
      if (lookup && is_generic(lookup) &&
          lookup->kind != T_CREATE_NEW_GENERIC) {

        Type *t = deep_copy_type(lookup);
        t = compute_concrete_type(t, contained_type);
        return t;
      }

      cons->data.T_CONS.num_args = 1;
      cons->data.T_CONS.args = talloc(sizeof(Type *));
      cons->data.T_CONS.args[0] = contained_type;

      expr->data.AST_BINOP.right->md = contained_type;
      expr->data.AST_BINOP.left->md = type_fn(contained_type, cons);
      expr->md = cons;

      last_ptr_type = _prev_last_ptr_type;
      return cons;
    }
  }
  }
  return NULL;
}

// Type *_rec_generic_cons(Type *t, TypeEnv **env) {
//   switch (t->kind) {
//   case T_CONS: {
//     for (int i = 0; i < t->data.T_CONS.num_args; i++) {
//       t->data.T_CONS.args[i] = _rec_generic_cons(t->data.T_CONS.args[i],
//       env);
//     }
//     break;
//   }
//   case T_VAR: {
//     Type *l = env_lookup(*env, t->data.T_VAR);
//     if (l) {
//       *t = *l;
//     } else {
//       Type *n = next_tvar();
//       *env = env_extend(*env, t->data.T_VAR, n);
//       *t = *n;
//     }
//     break;
//   }
//   case T_FN: {
//     while (t->kind == T_FN) {
//       t->data.T_FN.from = _rec_generic_cons(t->data.T_FN.from, env);
//       t = t->data.T_FN.to;
//     }
//     *t = *_rec_generic_cons(t->data.T_FN.from, env);
//     break;
//   }
//   }
//   return t;
// }

// Type *create_generic_cons_from_declaration(Type *_t) {
//   Type *t = deep_copy_type(_t);
//   TypeEnv *env = NULL;
//   return _rec_generic_cons(t, &env);
// }

Type *type_declaration(Ast *ast, TypeEnv **env) {

  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;
  binding_name = name;

  Ast *type_expr_ast = ast->data.AST_LET.expr;
  Type *type;

  if (type_expr_ast != NULL) {
    Type *var = empty_type();
    var->kind = T_VAR;
    var->data.T_VAR = name;
    TypeEnv *_env = env_extend(*env, name, var);
    TDCtx tdctx = {.env = _env};
    type = compute_type_expression(type_expr_ast, &tdctx);

    if (type->kind == T_CONS &&
        CHARS_EQ(type->data.T_CONS.name, TYPE_NAME_TUPLE)) {
      type->data.T_CONS.name = name;
    }
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

  // if (is_generic(type) && type->kind == T_CONS) {
  //   Type *gen_tmpl = empty_type();
  //   *gen_tmpl = (Type){T_CREATE_NEW_GENERIC,
  //                      .data = {.T_CREATE_NEW_GENERIC = {
  //                                   .fn = (CreateNewGenericTypeFn)
  //                                       create_generic_cons_from_declaration,
  //                                   .template = type}}};
  //
  //   type = gen_tmpl;
  // }

  *env = env_extend(*env, name, type);

  if (is_variant_type(type)) {
    // print_ast(ast);
    // printf("adding variant type members to env\n");
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *member = type->data.T_CONS.args[i];
      *env = env_extend(*env, member->data.T_CONS.name, type);
    }
    // print_type_env(*env);
  }

  binding_name = NULL;

  return type;
}
