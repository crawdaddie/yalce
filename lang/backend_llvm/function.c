#include "backend_llvm/function.h"
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
  LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), params, fn_len, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, "tmp", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);
  return func;
}

LLVMValueRef codegen_lambda(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Generate the prototype first.
  LLVMValueRef func = codegen_fn_proto(ast, ctx, module, builder);

  if (func == NULL) {
    return NULL;
  }

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  // Assign arguments to named values lookup.
  int fn_len = ast->data.AST_LAMBDA.len;
  for (int i = 0; i < fn_len; i++) {
    LLVMValueRef param = LLVMGetParam(func, i);
    LLVMSetValueName(param, ast->data.AST_LAMBDA.params[i].chars);
    // add var to hash-table
  }
  // Generate body.
  LLVMValueRef body = codegen(ast->data.AST_LAMBDA.body, ctx, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  return func;
}
