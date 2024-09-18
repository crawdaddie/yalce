#include "types/infer_fn_application.h"
#include "inference.h"
#include "types/unification.h"

// forward decl
Type *infer(Ast *ast, TypeEnv **env);
Type *infer_fn_application(Ast *ast, TypeEnv **env) {

  int len = ast->data.AST_APPLICATION.len;
  Type *app_arg_types[len];
  for (int i = 0; i < len; i++) {
    app_arg_types[i] = TRY_MSG(infer(ast->data.AST_APPLICATION.args + i, env),
                               "could not infer application argument");
  }

  Type *_fn_type = ast->data.AST_APPLICATION.function->md;

  Type *fn_type =
      _fn_type->is_recursive_fn_ref ? _fn_type : copy_type(_fn_type);

  // Type *fn_type = copy_type(_fn_type);
  // Type *fn_type = _fn_type;

  TypeEnv *replacement_env = NULL;

  Type *result_fn = fn_type;

  for (int i = 0; i < len; i++) {
    if (app_arg_types[i]->kind == T_FN) {
      app_arg_types[i] = copy_type(app_arg_types[i]);
    }

    Type *unif =
        unify(result_fn->data.T_FN.from, app_arg_types[i], &replacement_env);

    if (!unif) {
      return NULL;
    }

    result_fn = result_fn->data.T_FN.to;
  }

  result_fn = resolve_generic_type(result_fn, replacement_env);

  ast->data.AST_APPLICATION.function->md =
      resolve_generic_type(fn_type, replacement_env);

  result_fn = resolve_tc_rank(result_fn);

  return result_fn;
}

Type *infer_cons(Ast *ast, TypeEnv **env) {
  Type *cons = copy_type(ast->data.AST_APPLICATION.function->md);

  TypeEnv *replacement_env = NULL;
  int len = ast->data.AST_APPLICATION.len;

  for (int i = 0; i < len; i++) {
    Type *arg_type =
        TRY_MSG(infer(ast->data.AST_APPLICATION.args + i, &replacement_env),
                "could not infer cons argument");
    unify(cons->data.T_CONS.args[i], arg_type, &replacement_env);
  }

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
  return res_type;
}
