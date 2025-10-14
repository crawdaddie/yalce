#include "backend_llvm/application.h"
#include "adt.h"
#include "builtin_functions.h"
#include "closures.h"
#include "coroutines.h"
#include "function.h"
#include "modules.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (to_type->kind == T_CONS && to_type->alias) {

    Ast id = (Ast){
        AST_IDENTIFIER,
        .data = {.AST_IDENTIFIER = {to_type->alias, strlen(to_type->alias)}}};

    // TODO: once all constructors are type symbol (ie first-class) we can add
    // them directly to the type instead of looking them up in the env
    JITSymbol *constructor_sym = lookup_id_ast(&id, ctx);

    if (constructor_sym && constructor_sym->type == STYPE_GENERIC_CONSTRUCTOR) {
      Type f =
          (Type){T_FN, .data = {.T_FN = {.from = from_type, .to = to_type}}};

      LLVMTypeRef fn_type = type_to_llvm_type(&f, ctx, module);
      LLVMValueRef cons_val = specific_fns_lookup(
          constructor_sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns,
          from_type);

      return LLVMBuildCall2(builder, fn_type, cons_val, (LLVMValueRef[]){val},
                            1, "constructor");
    }
  }

  if (!to_type->constructor) {
    return val;
  }

  ConsMethod constructor = to_type->constructor;

  return constructor(val, from_type, module, builder);
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef call_callable(Ast *ast, Type *callable_type, LLVMValueRef callable,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  if (!callable) {
    return NULL;
  }

  int args_len = ast->data.AST_APPLICATION.len;
  int exp_args_len = fn_type_args_len(callable_type);

  LLVMTypeRef llvm_callable_type =
      type_to_llvm_type(callable_type, ctx, module);

  if (!llvm_callable_type) {
    print_ast_err(ast);
    return NULL;
  }

  if (callable_type->kind == T_FN &&
      callable_type->data.T_FN.from->kind == T_VOID) {

    return LLVMBuildCall2(builder, llvm_callable_type, callable, NULL, 0,
                          "call_func");
  }

  LLVMValueRef app_vals[args_len];

  for (int i = 0; i < args_len; i++) {

    Type *expected_type = callable_type->data.T_FN.from;

    Ast *app_arg = ast->data.AST_APPLICATION.args + i;

    Type *app_arg_type = app_arg->md;

    LLVMValueRef app_val;
    if (is_generic(app_arg->md) && expected_type->kind == T_FN) {

      Ast _app_arg = *app_arg;
      _app_arg.md = expected_type;
      app_val = codegen(&_app_arg, ctx, module, builder);
    } else if (!types_equal(app_arg_type, expected_type) &&
               expected_type->alias != NULL) {

      app_val = codegen(app_arg, ctx, module, builder);

      app_val = handle_type_conversions(app_val, app_arg_type, expected_type,
                                        ctx, module, builder);

    } else {
      app_val = codegen(app_arg, ctx, module, builder);
    }

    callable_type = callable_type->data.T_FN.to;

    app_vals[i] = app_val;
  }

  LLVMValueRef c =
      LLVMBuildCall2(builder, llvm_callable_type, callable, app_vals,

                     args_len, "call_func");
  return c;
}

static LLVMValueRef
call_callable_with_args(LLVMValueRef *args, int len, Type *callable_type,
                        LLVMValueRef callable, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  if (!callable) {
    return NULL;
  }

  LLVMTypeRef llvm_callable_type =
      type_to_llvm_type(callable_type, ctx, module);
  if (!llvm_callable_type) {
    return NULL;
  }

  if (callable_type->kind == T_FN &&
      callable_type->data.T_FN.from->kind == T_VOID) {

    return LLVMBuildCall2(builder, llvm_callable_type, callable, NULL, 0,
                          "call_func");
  }

  LLVMValueRef res = LLVMBuildCall2(builder, llvm_callable_type, callable, args,
                                    len, "call_func");
  return res;
}
bool is_closure_symbol(JITSymbol *sym) { return is_closure(sym->symbol_type); }

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  if (is_generic(expected_fn_type)) {
    expected_fn_type = deep_copy_type(expected_fn_type);
    expected_fn_type = resolve_type_in_env(expected_fn_type, ctx->env);
    Type *ex = expected_fn_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len;
         i++, ex = ex->data.T_FN.to) {
      if (ast->data.AST_APPLICATION.args[i].tag == AST_IDENTIFIER) {
        JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.args + i, ctx);
        if (sym->type == STYPE_FUNCTION && is_closure(sym->symbol_type)) {
          ex->data.T_FN.from = sym->symbol_type;
        }
      }
    }
  }
  //
  // // x.mem a ??
  if (ast->data.AST_APPLICATION.function->tag == AST_RECORD_ACCESS &&
      !is_module_ast(
          ast->data.AST_APPLICATION.function->data.AST_RECORD_ACCESS.record)) {

    LLVMValueRef callable =
        codegen(ast->data.AST_APPLICATION.function, ctx, module, builder);

    if (!callable) {
      fprintf(stderr, "Error: could not access record\n");
      return NULL;
    }

    return call_callable(ast, expected_fn_type, callable, ctx, module, builder);
  }

  Type *res_type = ast->md;

  if (is_closure(res_type) && application_is_partial(ast)) {
    return codegen_create_closure(ast, ctx, module, builder);
  }

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (!sym) {
    fprintf(stderr, "Error callable symbol %s not found in scope %d\n",
            sym_name, ctx->stack_ptr);
    return NULL;
  }

  if (is_closure_symbol(sym)) {
    return call_closure_sym(ast, expected_fn_type, sym, ctx, module, builder);
  }

  if (is_sum_type(expected_fn_type) && sym->type == STYPE_VARIANT_TYPE) {
    return codegen_adt_member_with_args(expected_fn_type, sym->llvm_type, ast,
                                        sym_name, ctx, module, builder);
  }

  Type *symbol_type = sym->symbol_type;

  if (sym->type == STYPE_GENERIC_FUNCTION &&
      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {

    return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
        ast, ctx, module, builder);
  }

  int args_len = ast->data.AST_APPLICATION.len;
  int expected_args_len = fn_type_args_len(sym->symbol_type);

  if (is_coroutine_constructor_type(symbol_type)) {

    LLVMValueRef instance_ptr =
        coro_create(sym, expected_fn_type, ast, ctx, module, builder);
    return instance_ptr;

  } else if (is_coroutine_type(symbol_type)) {
    return coro_resume(sym, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_FUNCTION && !is_closure(sym->symbol_type)) {

    LLVMValueRef callable =
        get_specific_callable(sym, expected_fn_type, ctx, module, builder);

    return call_callable(ast, expected_fn_type, callable, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_CONSTRUCTOR) {
    Type *from_type = ast->data.AST_APPLICATION.args->md;
    Type exp = (Type){
        T_FN, .data = {.T_FN = {.from = from_type, .to = expected_fn_type}}};

    LLVMValueRef callable =
        get_specific_callable(sym, &exp, ctx, module, builder);

    return call_callable(ast, &exp, callable, ctx, module, builder);
  }

  if (sym->type == STYPE_FUNCTION) {
    Type *callable_type = sym->symbol_type;

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);
    return res;
  }
  if (sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    Type *callable_type = sym->symbol_type;
    if (sym->val == NULL) {

      LLVMValueRef val =
          codegen_extern_fn(sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast,
                            ctx, module, builder);
      sym->val = val;
    }

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);

    return res;
  }

  if (sym->type == STYPE_LOCAL_VAR && sym->symbol_type->kind == T_FN) {
    Type *callable_type = sym->symbol_type;

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);

    return res;
  }

  return NULL;
}
