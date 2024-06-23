#include "backend_llvm/function.h"
#include "backend_llvm/symbols.h"
#include "serde.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_fn_proto(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  int fn_len = ast->data.AST_LAMBDA.len;

  // Create argument list.
  LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * fn_len);
  for (int i = 0; i < fn_len; i++) {
    params[i] = LLVMInt32Type();
  }

  // Create function type.
  LLVMTypeRef fn_type = LLVMFunctionType(LLVMInt32Type(), params, fn_len, 0);

  // Create function.
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  LLVMValueRef func = LLVMAddFunction(module, fn_name.chars, fn_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);
  return func;
}

static LLVMTypeRef param_type() { return LLVMInt32Type(); }

LLVMValueRef codegen_lambda(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Generate the prototype first.
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  LLVMValueRef func = codegen_fn_proto(ast, ctx, module, builder);

  if (func == NULL) {
    return NULL;
  }

  // Create basic block.
  JITLangCtx fn_scope = {.stack = ctx->stack, .stack_ptr = ctx->stack_ptr + 1};
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  int fn_len = ast->data.AST_LAMBDA.len;
  for (int i = 0; i < fn_len; i++) {
    ObjString param_name = ast->data.AST_LAMBDA.params[i];
    LLVMValueRef param_val = LLVMGetParam(func, i);
    LLVMSetValueName2(param_val, param_name.chars, param_name.length);

    JITSymbol *v = malloc(sizeof(JITSymbol));
    *v = (JITSymbol){.llvm_type = param_type(),
                     .val = param_val,
                     .symbol_type = STYPE_FN_PARAM};

    // add param value to hash-table
    ht_set_hash(fn_scope.stack + fn_scope.stack_ptr, param_name.chars,
                param_name.hash, v);
  }

  // add function as recursive ref
  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){
      .llvm_type = param_type(), .val = func, .symbol_type = STYPE_FUNCTION};

  ht_set_hash(fn_scope.stack + fn_scope.stack_ptr, fn_name.chars, fn_name.hash,
              v);

  // Generate body.
  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_scope, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  return func;
}

static bool is_void_fn(LLVMValueRef fn) { return LLVMCountParams(fn) == 0; }
static bool is_void_application(LLVMValueRef fn, Ast *ast) {
  int app_len = ast->data.AST_APPLICATION.len;
  return is_void_fn(fn) && app_len == 1 &&
         ast->data.AST_APPLICATION.args[0].tag == AST_VOID;
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  LLVMValueRef func = codegen_identifier(ast->data.AST_APPLICATION.function,
                                         ctx, module, builder);
  if (!func) {
    return NULL;
  }

  int app_len = ast->data.AST_APPLICATION.len;

  if (is_void_application(func, ast)) {
    LLVMValueRef args[] = {};
    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, args, 0,
                          "call_func");
  }
  unsigned int args_len = LLVMCountParams(func);
  if (app_len == args_len) {
    LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * app_len);
    for (int i = 0; i < app_len; i++) {
      args[i] =
          codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);
    }
    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, args,
                          app_len, "call_func");
  }

  if (app_len < args_len) {
    return NULL;
  }
  return NULL;
}
