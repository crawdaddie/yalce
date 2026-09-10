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

  LLVMTypeRef struct_type =
      type_to_llvm_aggregate_type(expected_type, ctx, module);

  LLVMValueRef tuple = LLVMGetUndef(struct_type);
  for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;
    LLVMValueRef item_val = codegen(arg, ctx, module, builder);
    tuple = LLVMBuildInsertValue(builder, tuple, item_val, i, "");
  }

  if (type_uses_boxed_recursive_storage(expected_type)) {
    LLVMValueRef boxed = LLVMBuildMalloc(builder, struct_type, "boxed_record");
    LLVMBuildStore(builder, tuple, boxed);
    return boxed;
  }

  return tuple;
}
