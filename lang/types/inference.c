#include "inference.h"
#include "serde.h"
#include "types/infer_fn_application.h"
#include "types/infer_match_expr.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include "types/unification.h"
#include <stdlib.h>
#include <string.h>

#define TRY(expr)                                                              \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

#define TRY_EXCEPT(expr, except)                                               \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      (except);                                                                \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

static const char *fresh_tvar_name() {
  char *new_name = talloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

Type *next_tvar() {
  Type *tvar = talloc(sizeof(Type));
  *tvar = (Type){T_VAR, {.T_VAR = fresh_tvar_name()}};
  return tvar;
}

static TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {
  // printf("res binding: ");
  // print_ast(binding);

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }

  case AST_TUPLE: {
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      env = add_binding_to_env(env, binding->data.AST_LIST.items + i,
                               type->data.T_CONS.args[i]);
    }
    return env;
  }
  case AST_BINOP: {
    token_type op = binding->data.AST_BINOP.op;
    Ast *left = binding->data.AST_BINOP.left;
    Ast *right = binding->data.AST_BINOP.right;
    if (op == TOKEN_DOUBLE_COLON && is_list_type(type)) {
      env = add_binding_to_env(env, left, type->data.T_CONS.args[0]);
      env = add_binding_to_env(env, right, type);
    }
    return env;
  }

  case AST_APPLICATION: {
    if (type->kind == T_CONS) {
      for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
        env = add_binding_to_env(env, binding->data.AST_APPLICATION.args + i,
                                 type->data.T_CONS.args[i]);
      }
    } else if (type->kind == T_FN) {
      Type *f = type;
      for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
        env = add_binding_to_env(env, binding->data.AST_APPLICATION.args + i,
                                 f->data.T_FN.from);
        f = f->data.T_FN.to;
      }
    }
    return env;
  }
  }
  return env;
}

static Type *binding_type(Ast *ast) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    return next_tvar();
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = malloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      tuple_mems[i] = binding_type(ast->data.AST_LIST.items + i);
    }
    Type *tup = empty_type();
    *tup = (Type){T_CONS,
                  {.T_CONS = {.name = TYPE_NAME_TUPLE,
                              .args = tuple_mems,
                              .num_args = len}}};
    return tup;
  }

  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}

static Type *infer_lambda(Ast *ast, TypeEnv **env) {

  int len = ast->data.AST_LAMBDA.len;
  TypeEnv *fn_scope_env = *env;

  Type *return_type = next_tvar();

  Type *fn;
  if (len == 1 && ast->data.AST_LAMBDA.params->tag == AST_VOID) {
    fn = &t_void;
    fn = type_fn(fn, return_type);
  } else {
    Type *param_types[len];

    for (int i = len - 1; i >= 0; i--) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      Type *ptype = binding_type(param_ast);
      param_types[i] = ptype;

      fn_scope_env = add_binding_to_env(fn_scope_env, param_ast, ptype);
    }
    fn = create_type_multi_param_fn(len, param_types, return_type);
  }
  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;
  Type *recursive_ref = tvar(ast->data.AST_LAMBDA.fn_name.chars);
  if (fn_name != NULL) {
    recursive_ref->is_recursive_fn_ref = true;
    fn_scope_env = env_extend(fn_scope_env, ast->data.AST_LAMBDA.fn_name.chars,
                              recursive_ref);
  }

  Type *body_type = TRY(infer(ast->data.AST_LAMBDA.body, &fn_scope_env));

  TypeEnv **_env = env;
  Type *unified_ret = unify(return_type, body_type, _env);

  *return_type = *unified_ret;

  if (recursive_ref->kind == T_FN && is_generic(recursive_ref)) {
    TypeEnv *_env = NULL;
    unify_recursive_defs_mut(fn, recursive_ref, &_env);
  }

  Type *r = recursive_ref;
  Type *f = fn;
  while (r->kind == T_FN) {
    Type *rf = r->data.T_FN.from;
    Type *ff = f->data.T_FN.from;
    *rf = *ff;

    r = r->data.T_FN.to;
    f = f->data.T_FN.to;
  }
  *r = *f;

  return fn;
}

Type *replace_in(Type *type, Type *tvar, Type *replacement);
static Type *resolve_generic_variant(Type *t, TypeEnv *env) {
  while (env) {
    const char *key = env->name;
    Type tvar = {T_VAR, .data = {.T_VAR = key}};
    Type *replaced = replace_in(t, &tvar, env->type);
    t = replaced;

    env = env->next;
  }

  return t;
}
Type *extern_fn_type(Ast *sig, TypeEnv **env) {
  if (sig->tag == AST_FN_SIGNATURE) {
    Ast *param_ast = sig->data.AST_LIST.items;
    Type *fn =
        type_fn(infer(param_ast, env), extern_fn_type(param_ast + 1, env));
    return fn;
  }
  return infer(sig, env);
}

Type *infer(Ast *ast, TypeEnv **env) {

  Type *type = NULL;
  switch (ast->tag) {
  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {

      stmt = ast->data.AST_BODY.stmts[i];
      Type *t = infer(stmt, env);
      if (!t) {
        fprintf(stderr, "Failure typechecking body statement: ");
        print_ast_err(stmt);
        return NULL;
      }
      type = t;
    }
    // type = stmt->md;
    break;
  }
  case AST_INT: {
    type = &t_int;
    break;
  }

  case AST_DOUBLE: {
    type = &t_num;
    break;
  }

  case AST_BOOL: {
    type = &t_bool;
    break;
  }

  case AST_VOID: {
    type = &t_void;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }
  case AST_STRING: {
    type = &t_string;
    break;
  }
  // case AST_BINOP: {
  //   type = TRY_MSG(infer_binop(ast, env), "Error: binop infer failed");
  //   break;
  // }
  case AST_LET: {
    Ast *expr = ast->data.AST_LET.expr;
    Type *expr_type = TRY_MSG(infer(expr, env), NULL);
    Ast *binding = ast->data.AST_LET.binding;

    *env = add_binding_to_env(*env, binding, expr_type);

    Ast *in_expr = ast->data.AST_LET.in_expr;
    if (in_expr) {
      Type *in_expr_type = TRY_MSG(infer(in_expr, env), NULL);
      type = in_expr_type;
    } else {
      type = expr_type;
    }

    break;
  }

  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(ast)) {
      type = next_tvar();

      break;
    }

    type = find_type_in_env(*env, ast->data.AST_IDENTIFIER.value);
    if (type == NULL) {
      type = next_tvar();
    }
    break;
  }

  case AST_TYPE_DECL: {
    type = type_declaration(ast, env);
    break;
  }

  case AST_LIST: {
    int len = ast->data.AST_LIST.len;
    if (len == 0) {
      Type *el_type = next_tvar();
      Type **args = talloc(sizeof(Type *));
      args[0] = el_type;
      type = talloc(sizeof(Type));
      *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, args, 1}}};
      break;
    }

    Type *element_type = TRY_MSG(
        infer(ast->data.AST_LIST.items, env),
        "Could not infer type of list literal elements (first element)");

    Type *el_type;
    for (int i = 1; i < len; i++) {
      Ast *el = ast->data.AST_LIST.items + i;
      el_type =
          TRY_MSG(infer(el, env), "Failure typechecking list literal element");

      if (!types_equal(element_type, el_type)) {
        fprintf(stderr, "Error typechecking list literal - all elements must "
                        "be of the same type\n");
        print_type_err(element_type);
        fprintf(stderr, " != ");
        print_type_err(el_type);
        return NULL;
      }
    }
    type = talloc(sizeof(Type));
    Type **contained = talloc(sizeof(Type *));
    contained[0] = el_type;
    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, contained, 1}}};

    break;
  }

  case AST_ARRAY: {
    int len = ast->data.AST_LIST.len;
    if (len == 0) {
      Type *el_type = next_tvar();
      type = create_array_type(el_type, 0);
      // Type **args = talloc(sizeof(Type *) + sizeof(int));
      // args[0] = el_type;
      // type = talloc(sizeof(Type));
      // *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_ARRAY, args, 1}}};
      break;
    }

    Type *element_type = TRY_MSG(
        infer(ast->data.AST_LIST.items, env),
        "Could not infer type of array literal elements (first element)");

    Type *el_type;

    for (int i = 1; i < len; i++) {
      Ast *el = ast->data.AST_LIST.items + i;

      el_type =
          TRY_MSG(infer(el, env), "Failure typechecking array literal element");

      if (!types_equal(element_type, el_type)) {

        fprintf(stderr, "Error typechecking array literal - all elements must "
                        "be of the same type\n");

        print_type_err(element_type);
        fprintf(stderr, " != ");
        print_type_err(el_type);
        return NULL;
      }
    }
    type = create_array_type(el_type, len);

    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;

    Type **cons_args = talloc(sizeof(Type *) * arity);
    for (int i = 0; i < arity; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype =
          TRY_MSG(infer(member, env), "Error typechecking tuple item");
      cons_args[i] = mtype;
    }
    type = talloc(sizeof(Type));
    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, cons_args, arity}}};

    break;
  }
  case AST_FMT_STRING: {
    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      infer(member, env);
    }
    type = &t_string;
    break;
  }
  case AST_LAMBDA: {
    type = infer_lambda(ast, env);
    break;
  }

  case AST_EXTERN_FN: {
    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;
    type = extern_fn_type(sig, env);
    break;
  }

  case AST_APPLICATION: {
    // Ast *custom_binop = ast->data.AST_APPLICATION.args;
    // Ast *l = ast->data.AST_APPLICATION.function;
    // Ast *r = ast->data.AST_APPLICATION.args + 1;
    //
    // Type *binop = env_lookup(*env, custom_binop->data.AST_IDENTIFIER.value);
    // if (binop && binop->kind == T_FN && fn_type_args_len(binop) == 2) {
    //   printf("custom binop: ");
    //   print_ast(l);
    //   print_ast(r);
    //   ast->data.AST_APPLICATION.function =
    //   ast_identifier((ObjString){custom_binop->data.AST_IDENTIFIER.value,
    //   custom_binop->data.AST_IDENTIFIER.length});
    //   ast->data.AST_APPLICATION.args[0] = *l;
    //   ast->data.AST_APPLICATION.args[1] = *r;
    //   return infer(ast, env);
    // }

    Type *t = TRY_MSG(infer(ast->data.AST_APPLICATION.function, env),
                      "Failure could not infer type of callee ");

    if (t->kind == T_FN) {
      type = infer_fn_application(ast, env);
      break;
    }

    if (t->kind == T_CONS) {
      type = infer_cons(ast, env);
      break;
    }

    if (t->kind == T_VAR) {
      type = infer_unknown_fn_signature(ast, env);
      break;
    }

    break;
  }

  case AST_MATCH: {
    type = infer_match(ast, env);
    break;
  }
  case AST_MATCH_GUARD_CLAUSE: {
    type = TRY_MSG(infer(ast->data.AST_MATCH_GUARD_CLAUSE.test_expr, env),
                   "Could not infer test expression in match guard");

    TRY_MSG(infer(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, env),
            "Could not infer guard expression in match guard");
    break;
  }
  }

  ast->md = type;
  // if (type && (type->kind == T_VAR) && strcmp(type->data.T_VAR, "t0") == 0) {
  //   print_type(type);
  //   print_ast(ast);
  // }

  return type;
}
