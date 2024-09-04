#include "backend_llvm/function.h"
#include "backend_llvm/types.h"
#include "common.h"
#include "serde.h"
#include "symbols.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
LLVMTypeRef fn_proto_type(Type *fn_type, int fn_len, TypeEnv *env) {

  LLVMTypeRef llvm_param_types[fn_len];

  for (int i = 0; i < fn_len; i++) {
    llvm_param_types[i] = type_to_llvm_type(fn_type->data.T_FN.from, env);
    fn_type = fn_type->data.T_FN.to;
  }

  Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
  LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(return_type, env);

  // Create function type with return.
  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
  return llvm_fn_type;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);
  int params_count = ast->data.AST_EXTERN_FN.len - 1;

  Ast *signature_types = ast->data.AST_EXTERN_FN.signature_types;
  if (params_count == 0) {

    LLVMTypeRef ret_type = llvm_type_of_identifier(signature_types, ctx->env);
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, NULL, 0, false);

    LLVMValueRef func = LLVMAddFunction(module, name, fn_type);
    return func;
  }

  LLVMTypeRef llvm_param_types[params_count];
  for (int i = 0; i < params_count; i++) {
    llvm_param_types[i] =
        llvm_type_of_identifier(signature_types + i, ctx->env);
  }

  LLVMTypeRef ret_type =
      llvm_type_of_identifier(signature_types + params_count, ctx->env);
  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, llvm_param_types, params_count, false);

  return get_extern_fn(name, fn_type, module);
}

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  printf("codegen fn: ");
  print_ast(ast);
  return NULL;
}

LLVMValueRef codegen_constructor(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef call_symbol(JITSymbol *sym, Ast *args, int args_len,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  LLVMValueRef callable;
  int expected_args;
  Type *callable_type = sym->symbol_type;
  switch (sym->type) {
  case STYPE_FUNCTION: {
    callable = sym->val;
    expected_args = fn_type_args_len(sym->symbol_type);
    printf("found regular function callable args %d\n", expected_args);
  }
  }

  if (args_len < expected_args) {
    printf("not enough args - return curried fn\n");
    return NULL;
  }

  if (args_len == expected_args) {
    LLVMValueRef app_vals[args_len];

    for (int i = 0; i < args_len; i++) {
      Ast *app_val_ast = args + i;

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);

      if (!types_equal(app_val_ast->md, callable_type->data.T_FN.from)) {

        app_val = attempt_value_conversion(app_val, app_val_ast->md,
                                           callable_type->data.T_FN.from,
                                           module, builder);

        if (!app_val) {
          fprintf(stderr, "Error: attempted type conversion failed\n");
          return NULL;
        }
      }
      app_vals[i] = app_val;
      callable_type = callable_type->data.T_FN.to;
    }

    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable), callable,
                          app_vals, args_len, "call_func");
  }

  // printf("\ncallable: \n");
  // LLVMDumpValue(callable);
  return NULL;
}
LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  if (((Type *)ast->data.AST_APPLICATION.function->md)->kind == T_CONS) {
    return codegen_constructor(ast, ctx, module, builder);
  }

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);
  if (!sym) {
    fprintf(stderr, "codegen identifier failed symbol not found in scope %d\n",
            ctx->stack_ptr);
    return NULL;
  }
  return call_symbol(sym, ast->data.AST_APPLICATION.args,
                     ast->data.AST_APPLICATION.len, ctx, module, builder);
}
