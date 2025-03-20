#include "backend_llvm/array.h"
#include "types.h"
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

  LLVMValueRef size =
      LLVMBuildExtractValue(builder, array_struct, 0, "get_array_size");
  return size;
}
LLVMValueRef create_array_map_function(JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMTypeRef element_type) {

  // Map function type: element_type map(element_type value)
  LLVMTypeRef func_type = LLVMFunctionType(
      element_type,                  // Return type
      (LLVMTypeRef[]){element_type}, // Parameter type (element value)
      1,                             // Number of parameters
      0                              // Not variadic
  );

  // Full map function type: void map(int size, function* func, element_type*
  // input, element_type* output)
  LLVMTypeRef map_type =
      LLVMFunctionType(LLVMVoidType(), // Return type (void)
                       (LLVMTypeRef[]){
                           LLVMInt32Type(),                  // size
                           LLVMPointerType(func_type, 0),    // function pointer
                           LLVMPointerType(element_type, 0), // input array
                           LLVMPointerType(element_type, 0)  // output array
                       },
                       4, // Number of parameters
                       0  // Not variadic
      );

  LLVMValueRef map_func = LLVMAddFunction(module, "array_map", map_type);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(map_func, "entry");
  LLVMBasicBlockRef loop_body = LLVMAppendBasicBlock(map_func, "loop_body");
  LLVMBasicBlockRef loop_end = LLVMAppendBasicBlock(map_func, "loop_end");

  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef size = LLVMGetParam(map_func, 0);
  LLVMValueRef func_ptr = LLVMGetParam(map_func, 1);
  LLVMValueRef input_array = LLVMGetParam(map_func, 2);
  LLVMValueRef output_array = LLVMGetParam(map_func, 3);

  LLVMValueRef counter = LLVMBuildAlloca(builder, LLVMInt32Type(), "i");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter);

  LLVMBuildBr(builder, loop_body);

  LLVMPositionBuilderAtEnd(builder, loop_body);

  LLVMValueRef current_idx =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter, "current_idx");

  // Load element from input array
  LLVMValueRef input_element_ptr =
      LLVMBuildGEP2(builder, element_type, input_array,
                    (LLVMValueRef[]){current_idx}, 1, "input_element_ptr");
  LLVMValueRef element_value =
      LLVMBuildLoad2(builder, element_type, input_element_ptr, "element_value");

  // Call function with the element value
  LLVMValueRef call_args[] = {element_value};
  LLVMValueRef result =
      LLVMBuildCall2(builder, func_type, func_ptr, call_args, 1, "result");

  // Store result in output array
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, output_array,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, result, element_ptr);

  // Increment counter
  LLVMValueRef next_idx = LLVMBuildAdd(
      builder, current_idx, LLVMConstInt(LLVMInt32Type(), 1, 0), "next_idx");
  LLVMBuildStore(builder, next_idx, counter);

  // Check loop condition
  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntSLT, next_idx, size, "loop_cond");

  LLVMBuildCondBr(builder, cond, loop_body, loop_end);

  LLVMPositionBuilderAtEnd(builder, loop_end);
  LLVMBuildRetVoid(builder);

  LLVMDisposeBuilder(builder);

  return map_func;
}

LLVMValueRef create_array_mapi_function(JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMTypeRef element_type) {

  // Mapi function type: element_type mapi(int index, element_type value)
  LLVMTypeRef func_type = LLVMFunctionType(
      element_type, // Return type
      (LLVMTypeRef[]){LLVMInt32Type(),
                      element_type}, // Parameter types (index, value)
      2,                             // Number of parameters
      0                              // Not variadic
  );

  // Full mapi function type: void mapi(int size, function* func, element_type*
  // input, element_type* output)
  LLVMTypeRef mapi_type =
      LLVMFunctionType(LLVMVoidType(), // Return type (void)
                       (LLVMTypeRef[]){
                           LLVMInt32Type(),                  // size
                           LLVMPointerType(func_type, 0),    // function pointer
                           LLVMPointerType(element_type, 0), // input array
                           LLVMPointerType(element_type, 0)  // output array
                       },
                       4, // Number of parameters
                       0  // Not variadic
      );

  LLVMValueRef mapi_func = LLVMAddFunction(module, "array_mapi", mapi_type);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mapi_func, "entry");
  LLVMBasicBlockRef loop_body = LLVMAppendBasicBlock(mapi_func, "loop_body");
  LLVMBasicBlockRef loop_end = LLVMAppendBasicBlock(mapi_func, "loop_end");

  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef size = LLVMGetParam(mapi_func, 0);
  LLVMValueRef func_ptr = LLVMGetParam(mapi_func, 1);
  LLVMValueRef input_array = LLVMGetParam(mapi_func, 2);
  LLVMValueRef output_array = LLVMGetParam(mapi_func, 3);

  LLVMValueRef counter = LLVMBuildAlloca(builder, LLVMInt32Type(), "i");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter);

  LLVMBuildBr(builder, loop_body);

  LLVMPositionBuilderAtEnd(builder, loop_body);

  LLVMValueRef current_idx =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter, "current_idx");

  LLVMValueRef input_element_ptr =
      LLVMBuildGEP2(builder, element_type, input_array,
                    (LLVMValueRef[]){current_idx}, 1, "input_element_ptr");
  LLVMValueRef element_value =
      LLVMBuildLoad2(builder, element_type, input_element_ptr, "element_value");

  LLVMValueRef call_args[] = {current_idx, element_value};
  LLVMValueRef result =
      LLVMBuildCall2(builder, func_type, func_ptr, call_args, 2, "result");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, output_array,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, result, element_ptr);

  LLVMValueRef next_idx = LLVMBuildAdd(
      builder, current_idx, LLVMConstInt(LLVMInt32Type(), 1, 0), "next_idx");
  LLVMBuildStore(builder, next_idx, counter);

  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntSLT, next_idx, size, "loop_cond");

  LLVMBuildCondBr(builder, cond, loop_body, loop_end);

  LLVMPositionBuilderAtEnd(builder, loop_end);
  LLVMBuildRetVoid(builder);

  LLVMDisposeBuilder(builder);

  return mapi_func;
}
LLVMValueRef ArrayFillHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx->env, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr =
      LLVMBuildArrayMalloc(builder, element_type, size_const, "element_ptr");

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");

  LLVMValueRef fill_func =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry_block);
  LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(function, "loop");
  LLVMBasicBlockRef after_block = LLVMAppendBasicBlock(function, "after_loop");

  LLVMValueRef counter = LLVMBuildAlloca(builder, LLVMInt32Type(), "counter");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter);

  LLVMBuildBr(builder, loop_block);

  LLVMPositionBuilderAtEnd(builder, loop_block);

  LLVMValueRef current_idx =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter, "current_idx");

  LLVMValueRef idx_args[] = {current_idx};

  LLVMValueRef element = LLVMBuildCall2(
      builder,
      LLVMFunctionType(element_type, (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0),
      fill_func, idx_args, 1, "fill_element");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, element, element_ptr);

  LLVMValueRef next_idx = LLVMBuildAdd(
      builder, current_idx, LLVMConstInt(LLVMInt32Type(), 1, 0), "next_idx");
  LLVMBuildStore(builder, next_idx, counter);

  LLVMValueRef end_cond =
      LLVMBuildICmp(builder, LLVMIntSLT, next_idx, size_const, "end_cond");

  LLVMBuildCondBr(builder, end_cond, loop_block, after_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  array_struct = LLVMBuildInsertValue(builder, array_struct, data_ptr, 1,
                                      "insert_array_data");
  return array_struct;
}
