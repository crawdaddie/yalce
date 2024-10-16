#include "yield.h"

LLVMValueRef codegen(Ast *, JITLangCtx *, LLVMModuleRef, LLVMBuilderRef);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  return codegen(ast, ctx, module, builder);
}
