#include "./infer_application.h"
#include "./builtins.h"
#include "common.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <string.h>

Type *infer_fn_application(Type *func_type, Ast *ast, TICtx *ctx);

Type *create_fn_from_cons(Type *res, Type *cons) {

  Type *f = res;
  for (int i = cons->data.T_CONS.num_args - 1; i >= 0; i--) {
    f = type_fn(cons->data.T_CONS.args[i], f);
  }
  return f;
}

Type *infer_cons_application(Type *cons, Ast *ast, TICtx *ctx) {

  Type *f;
  if (is_sum_type(cons)) {
    Type *mem =
        extract_member_from_sum_type(cons, ast->data.AST_APPLICATION.function);
    if (!mem) {
      return NULL;
    }
    f = create_fn_from_cons(cons, mem);
  } else {
    f = create_fn_from_cons(cons, cons);
  }

  return infer_fn_application(f, ast, ctx);
}

bool match_arg_lists(int len, Type **a, Type **b) {
  for (int i = 0; i < len; i++) {
    if (!types_match(a[i], b[i])) {
      return false;
    }
  }
  return true;
}

const char *find_constructor_method(Type *cons_mod, int len, Type **inputs,
                                    int *index, Type **method) {

  for (int i = 0; i < cons_mod->data.T_CONS.num_args; i++) {
    Type *t = cons_mod->data.T_CONS.args[i];
    if (t->kind == T_SCHEME) {
      t = t->data.T_SCHEME.type;
    }
    int l = fn_type_args_len(t);
    Type *cons_fn_types[l];
    int j = 0;
    for (Type *tt = t; tt->kind == T_FN; tt = tt->data.T_FN.to, j++) {
      cons_fn_types[j] = tt->data.T_FN.from;
    }

    if (match_arg_lists(len, cons_fn_types, inputs)) {
      *index = i;
      *method = cons_mod->data.T_CONS.args[i];
      return cons_mod->data.T_CONS.names[i];
    }
  }

  return NULL;
}

Type *infer_constructor_application(TypeClass *constructor_tc, Type *cons,
                                    Ast *ast, TICtx *ctx) {

  if (!constructor_tc) {
    return NULL;
  }

  int len = ast->data.AST_APPLICATION.len;
  Type *fn_types[len];

  for (int i = 0; i < len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;
    fn_types[i] = infer(arg, ctx);
  }

  Type *cons_mod = constructor_tc->module;
  Type *constructor_method_tscheme = NULL;

  int index;
  char *cons_method_name = find_constructor_method(
      cons_mod, len, fn_types, &index, &constructor_method_tscheme);

  // for (int i = 0; i < cons_mod->data.T_CONS.num_args; i++) {
  //   Type *t = cons_mod->data.T_CONS.args[i];
  //   if (t->kind == T_SCHEME) {
  //     t = t->data.T_SCHEME.type;
  //   }
  //   int l = fn_type_args_len(t);
  //   Type *cons_fn_types[l];
  //   int j = 0;
  //   for (Type *tt = t; tt->kind == T_FN; tt = tt->data.T_FN.to, j++) {
  //     cons_fn_types[j] = tt->data.T_FN.from;
  //   }
  //
  //   if (match_arg_lists(len, cons_fn_types, fn_types)) {
  //     constructor_method_tscheme = cons_mod->data.T_CONS.args[i];
  //     cons_method_name = cons_mod->data.T_CONS.names[i];
  //     break;
  //   }
  // }

  if (!cons_method_name) {
    return NULL;
  }
  if (!constructor_method_tscheme) {
    return NULL;
  }

  if (constructor_method_tscheme->kind == T_SCHEME) {
    constructor_method_tscheme = instantiate(constructor_method_tscheme, ctx);
  }

  Type *res = infer_fn_application(constructor_method_tscheme, ast, ctx);
  Type *expected_type = ast->data.AST_APPLICATION.function->type;

  ast->data.AST_APPLICATION.function = ast_record_access(
      ast->data.AST_APPLICATION.function,
      ast_identifier((ObjString){.chars = cons_method_name,
                                 .length = strlen(cons_method_name)}));

  ast->data.AST_APPLICATION.function->data.AST_RECORD_ACCESS.record->type =
      cons_mod;
  ast->data.AST_APPLICATION.function->type = expected_type;
  return res;
}

// T-App: Γ ⊢ e₁ : τ₁    Γ ⊢ e₂ : τ₂    α fresh    S = unify(τ₁, τ₂ → α)
//        ──────────────────────────────────────────────────────────────
//                            Γ ⊢ e₁ e₂ : S(α)
Type *infer_application(Ast *ast, TICtx *ctx) {
  Ast *func = ast->data.AST_APPLICATION.function;

  // Step 1: Infer function type
  Type *func_type = infer(func, ctx);

  // printf("infer cons application??\n");
  // print_ast(ast);
  // print_type(func_type);

  if (!func_type) {
    return type_error(ast, "Cannot infer type of applicable");
  }

  if (is_coroutine_type(func_type) &&
      ast->data.AST_APPLICATION.args->tag == AST_VOID) {

    Type f = MAKE_FN_TYPE_2(&t_void,
                            create_option_type(func_type->data.T_CONS.args[0]));
    return infer_fn_application(&f, ast, ctx);
  }

  TypeClass *cons_tc = get_typeclass_by_name(func_type, "Constructor");
  if (cons_tc) {
    return infer_constructor_application(cons_tc, func_type, ast, ctx);
  }

  if (is_coroutine_constructor_type(func_type)) {
    func_type = func_type->data.T_CONS.args[0];

    Type *t = infer_fn_application(func_type, ast, ctx);
    if (is_coroutine_type(t->data.T_CONS.args[0])) {
      t = t->data.T_CONS.args[0];
    }
    return t;
  }

  if (func_type->kind == T_CONS) {
    return infer_cons_application(func_type, ast, ctx);
  }
  if (IS_PRIMITIVE_TYPE(func_type)) {
    infer(ast->data.AST_APPLICATION.args, ctx);
    return func_type;
    // if (func_type->kind == T_CHAR) {
    //   // printf("primitive type with constructor???\n");
    //   // print_type(func_type);
    //   // return NULL;
    // }
  }

  return infer_fn_application(func_type, ast, ctx);
}

Type *infer_fn_application(Type *func_type, Ast *ast, TICtx *ctx) {

  Ast *func = ast->data.AST_APPLICATION.function;

  int expected_args_len = fn_type_args_len(func_type);

  Ast *args = ast->data.AST_APPLICATION.args;
  int num_args = ast->data.AST_APPLICATION.len;

  // Step 2: Infer argument types
  Type **arg_types = t_alloc(sizeof(Type *) * num_args);
  for (int i = 0; i < num_args; i++) {
    arg_types[i] = infer(args + i, ctx);

    if (!arg_types[i]) {
      return type_error(ast, "Cannot infer argument %d type", i + 1);
    }
  }

  // Step 3: Create expected function type
  Type *result_type = next_tvar();
  Type *expected_type = result_type;

  // Build expected type: arg1 -> arg2 -> ... -> result
  for (int i = num_args - 1; i >= 0; i--) {
    expected_type = type_fn(arg_types[i], expected_type);
  }

  // Step 4: Unify function type with expected type
  TICtx unify_ctx = {};
  if (unify(func_type, expected_type, &unify_ctx)) {
    type_error(ast, "Function application type mismatch : ");
    print_type_err(func_type);
    fprintf(stderr, "  != \n");
    print_type_err(expected_type);
    // print_constraints(unify_ctx.constraints);
    return NULL;
  }

  ctx->constraints = merge_constraints(ctx->constraints, unify_ctx.constraints);

  // Step 5: Solve constraints and apply substitutions
  Subst *solution = solve_constraints(unify_ctx.constraints);

  ctx->subst = compose_subst(solution, ctx->subst);

  if (is_closure(func_type)) {
    expected_type->closure_meta = deep_copy_type(func_type->closure_meta);
  }
  expected_type = apply_substitution(solution, expected_type);
  ast->data.AST_APPLICATION.function->type = expected_type;

  Type *res = expected_type;

  for (int n = num_args; n; n--) {
    res = res->data.T_FN.to;
  }

  if (expected_args_len > num_args) {
    res = deep_copy_type(res);
    // printf("curried???\n");
    // print_type(func_type);
    Type **_arg_types = t_alloc(sizeof(Type *) * num_args);
    memcpy(_arg_types, arg_types, sizeof(Type *) * num_args);
    Type *closure_meta = create_tuple_type(num_args, _arg_types);
    res->closure_meta = closure_meta;
    // print_type(res);
  }
  // if (CHARS_EQ(fn_name, "array_at")) {
  //   printf("res type: \n");
  //   print_type(res);
  // }
  return res;
}
