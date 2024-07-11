#include "backend_llvm/codegen_function_currying.h"
#include "backend_llvm/codegen_function.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_curry_fn(Ast *curry, LLVMValueRef func,
                              unsigned int total_params_len, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  int saved_args_len = curry->data.AST_APPLICATION.len;
  int curried_fn_len = total_params_len - saved_args_len;

  Type *fn_type = curry->md;
  LLVMValueRef curried_func = codegen_fn_proto(
      fn_type, curried_fn_len, "curried_fn", ctx, module, builder);

  if (curried_func == NULL) {
    return NULL;
  }

  LLVMValueRef app_vals[total_params_len];
  for (int i = 0; i < saved_args_len; i++) {
    app_vals[i] =
        codegen(curry->data.AST_APPLICATION.args + i, ctx, module, builder);
  }

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(curried_func, "entry");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  JITLangCtx fn_ctx = {.stack = ctx->stack, .stack_ptr = ctx->stack_ptr + 1};

  for (int i = 0; i < curried_fn_len; i++) {
    LLVMValueRef param_val = LLVMGetParam(curried_func, i);
    app_vals[saved_args_len + i] = param_val;
  }

  LLVMValueRef body =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, app_vals,
                     total_params_len, "call_func");

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  return curried_func;
}
