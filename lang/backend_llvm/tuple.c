#include "backend_llvm/tuple.h"
#include "backend_llvm/strings.h"
#include "backend_llvm/types.h"
#include "serde.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

// Function to create an LLVM tuple value
LLVMValueRef codegen_tuple(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMTypeRef tuple_type = type_to_llvm_type(ast->type, ctx, module);
  LLVMValueRef tuple = LLVMGetUndef(tuple_type);

  int len = ast->data.AST_LIST.len;
  int offset = 0;

  for (int i = 0; i < len; i++) {
    Ast *mem_ast = ast->data.AST_LIST.items + i;

    if (mem_ast->tag == AST_SPREAD_OP) {
      Type *mem_type = mem_ast->type;
      mem_ast = mem_ast->data.AST_SPREAD_OP.expr;

      LLVMTypeRef _tuple_type = type_to_llvm_type(mem_type, ctx, module);
      LLVMValueRef _tuple_element = codegen(mem_ast, ctx, module, builder);

      for (int j = 0; j < mem_type->data.T_CONS.num_args; j++) {
        // TODO: improve this with memcpy
        LLVMValueRef __tuple_element =
            codegen_tuple_access(j, _tuple_element, _tuple_type, builder);

        tuple =
            LLVMBuildInsertValue(builder, tuple, __tuple_element, offset, "");
        offset++;
      }
    } else {

      LLVMValueRef tuple_element = codegen(mem_ast, ctx, module, builder);

      tuple = LLVMBuildInsertValue(builder, tuple, tuple_element, offset,
                                   "insertx");

      offset++;
    }
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

// Function to get nth value out of an LLVM tuple value
LLVMValueRef codegen_tuple_gep(int n, LLVMValueRef tuple_ptr,
                               LLVMTypeRef tuple_type, LLVMBuilderRef builder) {

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, tuple_type, tuple_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), n, 0)  // Get nth element
                    },
                    2, "tuple_element_ptr");

  return element_ptr;
}
LLVMValueRef codegen_struct_with_names_to_string(LLVMValueRef value,
                                                 Type *val_type,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder) {

  int len = 2 * val_type->data.T_CONS.num_args;
  LLVMValueRef strings[len];
  for (int i = 0; i < len; i++) {
  }
}

LLVMValueRef codegen_tuple_to_string(LLVMValueRef value, Type *val_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  if (val_type->data.T_CONS.names) {
    return codegen_struct_with_names_to_string(value, val_type, ctx, module,
                                               builder);
  }
  int len = val_type->data.T_CONS.num_args;
  LLVMValueRef strings[len];
  for (int i = 0; i < len; i++) {
  }
}
