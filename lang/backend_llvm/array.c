#include "backend_llvm/array.h"
#include "escape_analysis.h"
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

// Creates an array type: { i32, T* }
LLVMTypeRef tmp_generic_codegen_array_type() {
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(), // size
          GENERIC_PTR,     // data pointer
      },
      2, 0); // 2 elements, not packed
}

LLVMTypeRef codegen_matrix_type(LLVMTypeRef element_type) {
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),                 // total_size
          LLVMInt32Type(),                 // rows
          LLVMInt32Type(),                 // cols
          LLVMPointerType(element_type, 0) // data pointer
      },
      4, 0); // 2 elements, not packed
}

LLVMValueRef get_array_struct(LLVMValueRef array, LLVMTypeRef array_type,
                              LLVMBuilderRef builder) {

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }
  return array_struct;
}

LLVMValueRef get_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMTypeRef element_type) {

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct = get_array_struct(array, array_type, builder);

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "get_array_data_ptr");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");
  return LLVMBuildLoad2(builder, element_type, element_ptr, "element");
}

LLVMValueRef set_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMValueRef value,
                               LLVMTypeRef element_type) {

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef array_struct = get_array_struct(array, array_type, builder);

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "get_array_data_ptr");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");

  return LLVMBuildStore(builder, value, element_ptr);
}

LLVMValueRef codegen_create_array(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  int array_size = ast->data.AST_LIST.len;

  LLVMValueRef first_element = NULL;

  if (array_size > 0) {
    first_element = codegen(ast->data.AST_LIST.items, ctx, module, builder);
  } else {
    LLVMTypeRef empty_type = type_to_llvm_type(ast->md, ctx, module);

    LLVMValueRef size_const = LLVMConstInt(LLVMInt32Type(), 0, 0);
    LLVMValueRef array_struct = LLVMGetUndef(empty_type);
    array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                        "insert_array_size");

    return array_struct;
  }

  LLVMTypeRef element_type = LLVMTypeOf(first_element);
  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const = LLVMConstInt(LLVMInt32Type(), array_size, 0);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr;

  // TODO: use proper allocation strategy
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    data_ptr = LLVMBuildArrayAlloca(builder, element_type, size_const,
                                    "array_data_alloc");
  } else {
    data_ptr = LLVMBuildArrayMalloc(builder, element_type, size_const,
                                    "array_data_alloc");
  }

  // data_ptr = LLVMBuildArrayMalloc(builder, element_type, size_const,
  //                                 "array_data_alloc");

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");

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
  LLVMValueRef array_struct = get_array_struct(array, array_type, builder);

  LLVMValueRef size =
      LLVMBuildExtractValue(builder, array_struct, 0, "get_array_size");
  return size;
}

LLVMValueRef ArrayFillHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr;
  // =
  //     LLVMBuildArrayMalloc(builder, element_type, size_const, "element_ptr");
  // TODO: use proper allocation strategy
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    data_ptr =
        LLVMBuildArrayAlloca(builder, element_type, size_const, "element_ptr");
  } else {
    data_ptr =
        LLVMBuildArrayMalloc(builder, element_type, size_const, "element_ptr");
  }

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");

  Type *ftype = ast->data.AST_APPLICATION.function->md;
  Type *fill_func_type = ftype->data.T_FN.to->data.T_FN.from;
  ast->data.AST_APPLICATION.args[1].md = fill_func_type;

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

LLVMValueRef ArrayFillConstHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  // printf("array fill const handler\n");
  // print_ast(ast);

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = el_type->kind == T_FN
                                 ? GENERIC_PTR
                                 : type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  // Get the constant fill value from arg 2 instead of a function
  LLVMValueRef const_fill_value =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr;

  // TODO: use proper allocation strategy
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    data_ptr =
        LLVMBuildArrayAlloca(builder, element_type, size_const, "element_ptr");
  } else {
    data_ptr =
        LLVMBuildArrayMalloc(builder, element_type, size_const, "element_ptr");
  }

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");

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

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, const_fill_value, element_ptr);

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

LLVMValueRef ArraySuccHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  LLVMValueRef new_array_struct = LLVMGetUndef(array_type);

  LLVMValueRef current_size =
      LLVMBuildExtractValue(builder, array_struct, 0, "current_size");

  LLVMValueRef is_size_gt_zero =
      LLVMBuildICmp(builder, LLVMIntSGT, current_size,
                    LLVMConstInt(LLVMInt32Type(), 0, 0), "is_size_gt_zero");
  LLVMValueRef size_mask =
      LLVMBuildZExt(builder, is_size_gt_zero, LLVMInt32Type(), "size_mask");

  LLVMValueRef size_decrement = size_mask; // Already 0 or 1

  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_decrement, "new_size");

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "data_ptr");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){size_mask}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 1, "insert_new_data_ptr");

  return new_array_struct;
}

LLVMValueRef ArrayRangeHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef offset_val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef size_val =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args + 2, ctx, module, builder);

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  LLVMValueRef new_array_struct = LLVMGetUndef(array_type);

  LLVMValueRef new_size = size_val;

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "data_ptr");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){offset_val}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 1, "insert_new_data_ptr");

  return new_array_struct;
}

LLVMValueRef ArrayOffsetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  // print_ast(ast);
  // printf("array offset\n");
  // print_type(ast->md);
  LLVMValueRef offset_val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  Type *_array_type = ast->md;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef array_struct;
  if (LLVMGetTypeKind(LLVMTypeOf(array)) == LLVMPointerTypeKind) {
    array_struct =
        LLVMBuildLoad2(builder, array_type, array, "load_array_struct");
  } else {
    array_struct = array;
  }

  LLVMValueRef new_array_struct = LLVMGetUndef(array_type);

  LLVMValueRef current_size =
      LLVMBuildExtractValue(builder, array_struct, 0, "current_size");

  LLVMValueRef is_size_gt_zero =
      LLVMBuildICmp(builder, LLVMIntSGT, current_size,
                    LLVMConstInt(LLVMInt32Type(), 0, 0), "is_size_gt_zero");
  LLVMValueRef size_mask =
      LLVMBuildZExt(builder, is_size_gt_zero, LLVMInt32Type(), "size_mask");

  LLVMValueRef size_decrement = LLVMBuildMul(
      builder, offset_val, size_mask,
      "0_or_1_times_offset"); // size is Already 0 or 1 * offset_val

  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_decrement, "new_size");

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 1, "data_ptr");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){offset_val}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 1, "insert_new_data_ptr");

  return new_array_struct;
}

LLVMValueRef ArrayStrideHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args + 2, ctx, module, builder);

  Type *_array_type = ast->md;
  return NULL;
}

LLVMValueRef CStrHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  Type *arr_type = ast->data.AST_APPLICATION.args->md;
  LLVMTypeRef llvm_arr_type = type_to_llvm_type(arr_type, ctx, module);
  LLVMValueRef arr =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  arr = get_array_struct(arr, llvm_arr_type, builder);

  return LLVMBuildExtractValue(builder, arr, 1, "get_array_data_ptr");
}

LLVMValueRef ArrayConstructor(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  print_ast(ast);
  print_type(ast->md);
  return NULL;
}
