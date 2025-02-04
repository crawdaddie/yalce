#include "backend_llvm/array.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
// Creates an array type: { i32, T* }
LLVMTypeRef codegen_array_type(LLVMTypeRef element_type) {
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),                 // size
          LLVMPointerType(element_type, 0) // data pointer
      },
      2, 0); // 2 elements, not packed
}

LLVMValueRef __get_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                                 LLVMValueRef index, LLVMTypeRef element_type) {
  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array, 1, "get_array_data_ptr");
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");

  // Load and return element
  return LLVMBuildLoad2(builder, element_type, element_ptr, "element");
}

LLVMValueRef get_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMTypeRef element_type) {
  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  // Extract the data pointer from the struct
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "get_array_data_ptr");

  // Get element pointer using the index
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");
  // Load and return element
  return LLVMBuildLoad2(builder, element_type, element_ptr, "element");
}

LLVMValueRef set_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMValueRef value,
                               LLVMTypeRef element_type) {

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  // Extract the data pointer from the struct
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "get_array_data_ptr");

  // Get element pointer using the index
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");

  // Load and return element
  return LLVMBuildStore(builder, value, element_ptr);
}

LLVMValueRef codegen_create_array(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  int array_size = ast->data.AST_LIST.len;

  // First generate the first element to get its type
  LLVMValueRef first_element = NULL;
  if (array_size > 0) {
    first_element = codegen(ast->data.AST_LIST.items, ctx, module, builder);
  } else {
    return NULL;
  }

  LLVMTypeRef element_type = LLVMTypeOf(first_element);
  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const = LLVMConstInt(LLVMInt32Type(), array_size, 0);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  // Allocate the data array
  LLVMValueRef data_ptr =
      LLVMBuildArrayMalloc(builder, element_type, size_const, "element_ptr");

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");

  // Generate and store remaining elements
  for (int i = 0; i < array_size; i++) {
    LLVMValueRef element =
        codegen(ast->data.AST_LIST.items + i, ctx, module, builder);

    LLVMValueRef element_ptr =
        LLVMBuildGEP2(builder, element_type, data_ptr,
                      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), i, 0)}, 1,
                      "element_ptr");
    LLVMBuildStore(builder, element, element_ptr);
  }

  array_struct = LLVMBuildInsertValue(builder, array_struct, data_ptr, 1,
                                      "insert_array_data");
  return array_struct;
}

LLVMValueRef codegen_get_array_size(LLVMBuilderRef builder, LLVMValueRef array,
                                    LLVMTypeRef element_type) {

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  // Extract the data pointer from the struct
  LLVMValueRef size =
      LLVMBuildExtractValue(builder, array_struct, 0, "get_array_size");
  return size;
}
