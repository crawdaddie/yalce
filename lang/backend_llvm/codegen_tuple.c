#include "backend_llvm/codegen_tuple.h"
#include "codegen_types.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

// Function to create an LLVM tuple value
LLVMValueRef codegen_tuple(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMTypeRef tuple_type = type_to_llvm_type(ast->md);
  LLVMValueRef tuple = LLVMGetUndef(tuple_type);

  for (int i = 0; i < ast->data.AST_LIST.len; i++) {
    // Convert each element's AST node to its corresponding LLVM value
    LLVMValueRef tuple_element =
        codegen(ast->data.AST_LIST.items + i, ctx, module, builder);
    tuple = LLVMBuildInsertValue(builder, tuple, tuple_element, i, "");
  }
  return tuple;
}

// Function to get nth value out of an LLVM tuple value
LLVMValueRef codegen_tuple_access(int n, LLVMValueRef tuple,
                                  LLVMTypeRef tuple_type,
                                  LLVMBuilderRef builder) {

  LLVMValueRef element_ptr =
      LLVMBuildStructGEP2(builder, tuple_type, tuple, n, "get_tuple_element");

  LLVMTypeRef element_type = LLVMStructGetTypeAtIndex(tuple_type, n);

  return LLVMBuildLoad2(builder, element_type, element_ptr, "tuple_element_load");
}
