#include "backend_llvm/constructors.h"
#include "backend_llvm/types.h"
#include "llvm-c/Core.h"
#include <stdio.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_cons_type_constructor(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  Type *expected_type = ast->type;
  if (expected_type->kind != T_CONS) {
    fprintf(stderr,
            "Not Implemented error - constructor handler for non cons types\n");
    return NULL;
  }

  LLVMTypeRef struct_type = named_struct_type(expected_type->data.T_CONS.name,
                                              expected_type, ctx, module);

  LLVMValueRef tuple = LLVMGetUndef(struct_type);
  for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;
    LLVMValueRef item_val = codegen(arg, ctx, module, builder);
    tuple = LLVMBuildInsertValue(builder, tuple, item_val, i, "");
  }

  return tuple;
}
