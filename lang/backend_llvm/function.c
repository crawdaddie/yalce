#include "backend_llvm/function.h"
#include "backend_llvm/types.h"
#include "common.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
LLVMTypeRef fn_prototype(Type *fn_type, int fn_len, TypeEnv *env) {

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

static void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func,
                                 Type *fn_type, JITLangCtx *fn_ctx) {

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  // Generate the prototype first.
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  Type *fn_type = ast->md;
  int fn_len = ast->data.AST_LAMBDA.len;

  LLVMTypeRef prototype = fn_prototype(ast->md, fn_len, ctx->env);

  LLVMValueRef func = LLVMAddFunction(module, fn_name.chars, prototype);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  add_recursive_fn_ref(fn_name, func, fn_type, &fn_ctx);

  for (int i = 0; i < fn_len; i++) {
    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    Type *param_type = fn_type->data.T_FN.from;

    LLVMValueRef param_val = LLVMGetParam(func, i);
    LLVMValueRef _true = LLVMConstInt(LLVMInt1Type(), 1, 0);
    match_values(param_ast, param_val, param_type, &fn_ctx, module, builder);
    fn_type = fn_type->data.T_FN.to;
  }

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  if (body == NULL) {
    fprintf(stderr, "Error compiling function body\n");
    print_ast_err(ast);
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  return func;
}

LLVMValueRef codegen_constructor(Ast *cons_id, Ast *args, int len,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {

  int member_idx;
  Type *variant = variant_lookup(ctx->env, cons_id->md, &member_idx);

  if (variant) {
    printf("constructor of variant? ");
    print_type(variant);
  }

  return NULL;
}

LLVMValueRef call_symbol(JITSymbol *sym, Ast *args, int args_len,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  LLVMValueRef callable;
  int expected_args_len;
  Type *callable_type = sym->symbol_type;
  switch (sym->type) {
  case STYPE_FUNCTION: {
    callable = sym->val;
    expected_args_len = fn_type_args_len(sym->symbol_type);
    break;
  }
  case STYPE_GENERIC_FUNCTION: {
  }
  }

  if (args_len < expected_args_len) {
    return NULL;
  }

  if (args_len == expected_args_len) {
    LLVMValueRef app_vals[args_len];

    for (int i = 0; i < args_len; i++) {
      Ast *app_val_ast = args + i;
      Type *app_val_type = app_val_ast->md;
      app_val_type = resolve_tc_rank(app_val_type);

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);

      if (!types_equal(app_val_type, callable_type->data.T_FN.from)) {

        app_val = attempt_value_conversion(app_val, app_val_type,
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

  JITSymbol sym;

  if (lookup_id_ast_in_place(ast->data.AST_APPLICATION.function, ctx, &sym)) {
    fprintf(stderr, "codegen identifier failed symbol not found in scope %d:\n",
            ctx->stack_ptr);
    print_ast_err(ast->data.AST_APPLICATION.function);
    return NULL;
  }

  return call_symbol(&sym, ast->data.AST_APPLICATION.args,
                     ast->data.AST_APPLICATION.len, ctx, module, builder);
}
