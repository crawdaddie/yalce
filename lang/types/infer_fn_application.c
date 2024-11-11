#include "types/infer_fn_application.h"
#include "inference.h"
#include "serde.h"
#include "types/unification.h"
#include <stdlib.h>
#include <string.h>

// forward decl
Type *infer(Ast *ast, TypeEnv **env);

void print_unification_err(Ast *ast, Type *t1, Type *t2) {
  fprintf(stderr, "unification fail: ");
  print_location(ast);
  print_type_err(t1);
  fprintf(stderr, " != ");
  print_type_err(t2);
  fprintf(stderr, "\n");
}

Type *binding_type(Ast *ast);
TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type);

Type *infer_anonymous_lambda_arg(Ast *ast, Type *expected_type, TypeEnv **env) {

  int len = ast->data.AST_LAMBDA.len;
  TypeEnv *fn_scope_env = *env;

  Type *exp = expected_type;
  for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
    Ast *arg_ast = ast->data.AST_LAMBDA.params + i;
    Type *t = exp->data.T_FN.from;
    fn_scope_env = add_binding_to_env(fn_scope_env, arg_ast, t);
    exp = exp->data.T_FN.to;
  }

  Type *return_type = next_tvar();
  Type *fn;

  if (len == 1 && ast->data.AST_LAMBDA.params->tag == AST_VOID) {
    fn = &t_void;
    fn = type_fn(fn, return_type);
  } else {
    Type *param_types[len];

    for (int i = len - 1; i >= 0; i--) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      if (param_ast->tag == AST_IDENTIFIER) {
        Type *lookup =
            env_lookup(fn_scope_env, param_ast->data.AST_IDENTIFIER.value);
        if (lookup) {
          param_types[i] = lookup;
          continue;
        }
      }
      Type *ptype = binding_type(param_ast);
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

  Type *body_type = infer(ast->data.AST_LAMBDA.body, &fn_scope_env);

  TypeEnv **_env = env;
  Type *unified_ret = unify(return_type, body_type, _env);

  *return_type = *unified_ret;
  fn = resolve_generic_type(fn, fn_scope_env);
  ast->md = fn;
  return fn;
}

Type *infer_fn_application(Ast *ast, TypeEnv **env) {
  int len = ast->data.AST_APPLICATION.len;

  Type *_fn_type = ast->data.AST_APPLICATION.function->md;

  Type *fn_type =
      _fn_type->is_recursive_fn_ref ? _fn_type : copy_type(_fn_type);

  TypeEnv *replacement_env = NULL;

  Type *result_fn = fn_type;

  Type *app_arg_types[len];
  for (int i = 0; i < len; i++) {

    Ast *arg_ast = ast->data.AST_APPLICATION.args + i;
    Type *arg_type = infer(arg_ast, env);


    if (!arg_type) {
      fprintf(stderr, "could not infer application argument [%s:%d]\n",
              __FILE__, __LINE__);
      print_location(arg_ast);
      return NULL;
    }

    app_arg_types[i] = arg_type;

    if (app_arg_types[i]->kind == T_FN && arg_ast->tag != AST_LAMBDA) {
      app_arg_types[i] = copy_type(app_arg_types[i]);
    }

    Type *unif =
        unify(result_fn->data.T_FN.from, app_arg_types[i], &replacement_env);


    if (!unif && (!is_pointer_type(result_fn->data.T_FN.from))) {
      print_unification_err(ast->data.AST_APPLICATION.args + i,
                            result_fn->data.T_FN.from, app_arg_types[i]);
      return NULL;
    }

    result_fn = result_fn->data.T_FN.to;
  }

  result_fn = resolve_generic_type(result_fn, replacement_env);

  ast->data.AST_APPLICATION.function->md =
      resolve_generic_type(fn_type, replacement_env);

  result_fn = resolve_tc_rank(result_fn);
  const char *fn_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  if (strcmp(fn_name, "deref") == 0) {
    return app_arg_types[0]->data.T_CONS.args[0];
  }

  return result_fn;
}

Type *infer_cons(Ast *ast, TypeEnv **env) {

  Type *cons = copy_type(ast->data.AST_APPLICATION.function->md);

  TypeEnv *replacement_env = *env;
  int len = ast->data.AST_APPLICATION.len;

  for (int i = 0; i < len; i++) {
    Type *arg_type =
        TRY_MSG(infer(ast->data.AST_APPLICATION.args + i, &replacement_env),
                "could not infer cons argument");

    unify(cons->data.T_CONS.args[i], arg_type, &replacement_env);
  }

  ast->data.AST_APPLICATION.function->md = cons;
  return cons;
}

Type *infer_unknown_fn_signature(Ast *ast, TypeEnv **env) {

  const char *fn_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
  Type *res_type = next_tvar();

  Type *fn_type = res_type;
  int len = ast->data.AST_APPLICATION.len;

  for (int i = len - 1; i >= 0; i--) {
    Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, env);
    fn_type = type_fn(arg_type, fn_type);
  }
  fn_type->is_recursive_fn_ref = true;

  Type *fn_var = ast->data.AST_APPLICATION.function->md;
  *fn_var = *fn_type;
  // printf("fn %s type: ", fn_name);
  // print_type(fn_type);
  return res_type;
}
