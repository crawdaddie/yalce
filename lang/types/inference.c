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

TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }

  case AST_TUPLE: {
    if (type->kind == T_VAR) {
      Type **types = talloc(sizeof(Type *) * binding->data.AST_LIST.len);
      for (int i = 0; i < binding->data.AST_LIST.len; i++) {
        Ast *item = binding->data.AST_LIST.items + i;
        types[i] = next_tvar();
      }
      const char *type_name = type->data.T_VAR;
      *type =
          *create_cons_type(TYPE_NAME_TUPLE, binding->data.AST_LIST.len, types);
      env = env_extend(env, type_name, type);
    }
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

Type *binding_type(Ast *ast) {
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

typedef struct lambda_ctx_t {
  Ast *lambda;
  Type *yielded_type;
} lambda_ctx_t;

static lambda_ctx_t lambda_ctx = {};
// take a function type a -> b -> c & return a corresponding coroutine type
// a -> b -> (yield_interface: () -> Option of c, params_type: a * b)
static Type *convert_to_coroutine_generator_fn(Type *f, int fn_len) {
  Type *fn = f;
  if (fn_len == 1) {
    Type *params_type = fn->data.T_FN.from;
    Type *instance_type = talloc(sizeof(Type));
    instance_type->kind = T_COROUTINE_INSTANCE;
    instance_type->data.T_COROUTINE_INSTANCE.params_type = params_type;
    instance_type->data.T_COROUTINE_INSTANCE.yield_interface =
        type_fn(&t_void, create_option_type(fn->data.T_FN.to));
    fn->data.T_FN.to = instance_type;

    f->is_coroutine_fn = true;
    return f;
  }

  Type *params_type;
  Type **args = talloc(sizeof(Type *) * fn_len);
  params_type = create_tuple_type(fn_len, args);

  int idx = 0;

  while (fn->data.T_FN.to->kind == T_FN) {
    Type *param_type = fn->data.T_FN.from;
    params_type->data.T_CONS.args[idx] = param_type;
    fn = fn->data.T_FN.to;
    idx++;
  }

  Type *param_type = fn->data.T_FN.from;
  params_type->data.T_CONS.args[idx] = param_type;

  Type *return_type = fn->data.T_FN.to;
  Type *instance_type = talloc(sizeof(Type));
  instance_type->kind = T_COROUTINE_INSTANCE;
  instance_type->data.T_COROUTINE_INSTANCE.params_type = params_type;
  instance_type->data.T_COROUTINE_INSTANCE.yield_interface =
      type_fn(&t_void, create_option_type(return_type));
  fn->data.T_FN.to = instance_type;

  f->is_coroutine_fn = true;

  return f;
}

static Type *infer_lambda(Ast *ast, TypeEnv **env) {

  lambda_ctx_t prev_lambda_ctx = lambda_ctx;

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
      Type *ptype;
      Ast *def = ast->data.AST_LAMBDA.defaults[i];

      if (def) {
        ptype = compute_type_expression(def, *env);
      } else {
        ptype = binding_type(param_ast);
        param_ast->md = ptype;
      }
      param_types[i] = ptype;
      fn_scope_env = add_binding_to_env(fn_scope_env, param_ast, ptype);
    }
    fn = create_type_multi_param_fn(len, param_types, return_type);
  }

  bool is_anon = false;
  ObjString _fn_name = ast->data.AST_LAMBDA.fn_name;
  if (_fn_name.chars == NULL) {
    is_anon = true;
  }

  Type *recursive_ref = NULL;
  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;

  if (!is_anon) {
    recursive_ref = tvar(ast->data.AST_LAMBDA.fn_name.chars);
    recursive_ref->is_recursive_fn_ref = true;
    fn_scope_env = env_extend(fn_scope_env, ast->data.AST_LAMBDA.fn_name.chars,
                              recursive_ref);
  }

  int body_len = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  Ast *body = ast->data.AST_LAMBDA.body;

  Ast *body_stmt;
  Type *body_type;
  lambda_ctx = (lambda_ctx_t){ast, NULL};

  if (body->tag == AST_BODY) {
    for (int i = 0; i < body_len; i++) {
      body_stmt = body->data.AST_BODY.stmts[i];
      Type *t = infer(body_stmt, &fn_scope_env);
      if (!t) {
        fprintf(stderr, "Failure typechecking body statement: ");
        print_location(body_stmt);
        print_ast_err(body_stmt);
        return NULL;
      }
      body_type = t;
    }
  } else {
    body_stmt = body;
    Type *t = infer(body_stmt, &fn_scope_env);
    if (!t) {
      fprintf(stderr, "Failure typechecking body statement: ");
      print_location(body_stmt);
      print_ast_err(body_stmt);
      return NULL;
    }
    body_type = t;
  }

  TypeEnv **_env = &fn_scope_env;
  Type *unified_ret = unify(return_type, body_type, _env);
  *return_type = *unified_ret;

  if (recursive_ref && recursive_ref->kind == T_FN &&
      is_generic(recursive_ref)) {
    TypeEnv *_env = NULL;
    unify_recursive_defs_mut(fn, recursive_ref, &_env);
  }
  if (recursive_ref) {
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
  }

  fn = resolve_generic_type(fn, fn_scope_env);
  if (ast->data.AST_LAMBDA.is_coroutine) {
    Type *co = convert_to_coroutine_generator_fn(fn, len);
    fn = co;
  }

  lambda_ctx = prev_lambda_ctx;
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
    Type *f = compute_type_expression(
        sig->data.AST_LIST.items + sig->data.AST_LIST.len - 1, *env);

    for (int i = sig->data.AST_LIST.len - 2; i >= 0; i--) {
      Type *p = compute_type_expression(sig->data.AST_LIST.items + i, *env);
      f = type_fn(p, f);
    }
    return f;

    // Ast *param_ast = sig->data.AST_LIST.items;
    // Type *fn =
    //     type_fn(infer(param_ast, env), extern_fn_type(param_ast + 1, env));
    // return fn;
  }
  return infer(sig, env);
}

bool yield_is_new_coroutine(Ast *yield_expr) {
  Type *yield_type = yield_expr->md;
  return yield_type->kind == T_COROUTINE_INSTANCE;
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
        print_location(stmt);
        // print_ast_err(stmt);
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
  case AST_BINOP: {
    if (ast->data.AST_BINOP.op == TOKEN_OF) {
      type = compute_type_expression(ast, *env);
    }
    break;
  }
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

    // if ((strcmp(ast->data.AST_IDENTIFIER.value, "note") == 0) ||
    //     (strcmp(ast->data.AST_IDENTIFIER.value, "filter_freq") == 0)) {
    //   // printf("ast identifier\n");
    //   // print_ast(ast);
    //   // print_type_env(*env);
    // }
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
    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      char **names = talloc(sizeof(char *) * arity);
      for (int i = 0; i < arity; i++) {
        Ast *member = ast->data.AST_LIST.items + i;
        names[i] = member->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      }
      type->names = names;
    }

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
    Type *t = TRY_MSG(infer(ast->data.AST_APPLICATION.function, env),
                      "Failure could not infer type of callee ");

    if (t->kind == T_FN) {
      type = infer_fn_application(ast, env);
      break;
    }

    if (t->kind == T_COROUTINE_INSTANCE) {
      type = t->data.T_COROUTINE_INSTANCE.yield_interface->data.T_FN.to;
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
  case AST_YIELD: {
    if (lambda_ctx.lambda == NULL) {
      fprintf(stderr, "Error: yield cannot appear outside a function");
      return NULL;
    }
    lambda_ctx.lambda->data.AST_LAMBDA.is_coroutine = true;
    lambda_ctx.lambda->data.AST_LAMBDA.num_yields++;

    type = infer(ast->data.AST_YIELD.expr, env);
    // printf("type of yield: ");
    // print_ast(ast);
    // print_type(type);
    // print_type_env(*env);

    if (type->kind == T_COROUTINE_INSTANCE) {
      type = type_of_option(
          fn_return_type(type->data.T_COROUTINE_INSTANCE.yield_interface));
    }

    if (lambda_ctx.yielded_type != NULL) {
      Type *unified_type = unify(type, lambda_ctx.yielded_type, env);
      if (!unified_type) {
        fprintf(stderr, "Failed to unify yielded type\n");
        print_location(ast);
        return NULL;
      }
      type = unified_type;
    } else {
      lambda_ctx.yielded_type = type;
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
  case AST_IMPORT: {
    // TODO: handle proper module imports
    type = tvar(ast->data.AST_IMPORT.module_name);
    break;
  }
  case AST_RECORD_ACCESS: {
    Type *rec_type = infer(ast->data.AST_RECORD_ACCESS.record, env);
    if (rec_type->names == NULL) {
      fprintf(stderr, "Error: object has no named members\n");
    }

    const char *name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;
    Type *member_type = get_struct_member_type(name, rec_type);
    if (!member_type) {
      fprintf(stderr, "Error name %s not found in obj\n", name);
      print_type_err(rec_type);
      return NULL;
    }

    type = member_type;
    break;
  }
  }

  ast->md = type;

  return type;
}
