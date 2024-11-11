#include "backend_llvm/tuple.h"
#include "backend_llvm/types.h"
#include "serde.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

// Function to create an LLVM tuple value
LLVMValueRef codegen_tuple(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  // print_type_env(ctx->env);

  LLVMTypeRef tuple_type = type_to_llvm_type(ast->md, ctx->env, module);
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

  // Check if the tuple is a pointer type
  if (LLVMGetTypeKind(LLVMTypeOf(tuple)) != LLVMPointerTypeKind) {
    // If it's not a pointer, use LLVMBuildExtractValue - extracts value from a
    // direct value rather than a ptr which needs a GEP2 instruction
    return LLVMBuildExtractValue(builder, tuple, n, "struct_element");
  }

  // LLVMValueRef element_ptr =
  //     LLVMBuildGEP2(builder, tuple_type, tuple, n, "get_tuple_element");
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, tuple_type, tuple,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), n, 0)  // Get nth element
                    },
                    2, "tuple_element_ptr");

  LLVMTypeRef element_type = LLVMStructGetTypeAtIndex(tuple_type, n);

  return LLVMBuildLoad2(builder, element_type, element_ptr,
                        "tuple_element_load");
}

// Function to get nth type out of an LLVM tuple value
LLVMTypeRef codegen_tuple_field_type(int n, LLVMValueRef tuple,
                                     LLVMTypeRef tuple_type,
                                     LLVMBuilderRef builder) {

  LLVMValueRef element_ptr =
      LLVMBuildStructGEP2(builder, tuple_type, tuple, n, "get_tuple_element");

  LLVMTypeRef element_type = LLVMStructGetTypeAtIndex(tuple_type, n);
  return element_type;
}
