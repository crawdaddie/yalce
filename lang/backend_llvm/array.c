#include "backend_llvm/array.h"
#include "coroutines/coroutines.h"
#include "escape_analysis.h"
#include "types.h"
#include "types/type_ser.h"
#include <llvm-c/Core.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static LLVMContextRef type_context(LLVMTypeRef type) {
  return LLVMGetTypeContext(type);
}

static LLVMTypeRef i32_type_for(LLVMTypeRef type) {
  return LLVMInt32TypeInContext(type_context(type));
}

static LLVMTypeRef generic_ptr_type_for_module(LLVMModuleRef module) {
  return LLVMPointerType(LLVMInt8TypeInContext(LLVMGetModuleContext(module)),
                         0);
}

static LLVMBasicBlockRef append_block_in_module(LLVMModuleRef module,
                                                LLVMValueRef function,
                                                const char *name) {
  return LLVMAppendBasicBlockInContext(LLVMGetModuleContext(module), function,
                                       name);
}

// Creates an array value type: { i32 size, i32 offset, T* data }
LLVMTypeRef codegen_array_type(LLVMTypeRef element_type) {
  LLVMContextRef llvm_ctx = type_context(element_type);
  return LLVMStructTypeInContext(
      llvm_ctx,
      (LLVMTypeRef[]){
          LLVMInt32TypeInContext(llvm_ctx), // size
          LLVMInt32TypeInContext(llvm_ctx), // offset from allocation base
          LLVMPointerType(element_type, 0)  // data pointer
      },
      3, 0); // 3 elements, not packed
}

// Strings use the same view layout as arrays: { i32 size, i32 offset, i8* data }.
LLVMTypeRef codegen_string_type(LLVMTypeRef char_type) {
  LLVMContextRef llvm_ctx = type_context(char_type);
  return LLVMStructTypeInContext(
      llvm_ctx,
      (LLVMTypeRef[]){
          LLVMInt32TypeInContext(llvm_ctx), // size
          LLVMInt32TypeInContext(llvm_ctx), // offset from allocation base
          LLVMPointerType(char_type, 0)     // chars pointer
      },
      3, 0);
}

// Creates a generic array value type: { i32, i32, i8* }
LLVMTypeRef tmp_generic_codegen_array_type() {
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(), // size
          LLVMInt32Type(), // offset
          GENERIC_PTR,     // data pointer
      },
      3, 0); // 3 elements, not packed
}

LLVMTypeRef codegen_matrix_type(LLVMTypeRef element_type) {
  LLVMContextRef llvm_ctx = type_context(element_type);
  return LLVMStructTypeInContext(
      llvm_ctx,
      (LLVMTypeRef[]){
          LLVMInt32TypeInContext(llvm_ctx), // total_size
          LLVMInt32TypeInContext(llvm_ctx), // rows
          LLVMInt32TypeInContext(llvm_ctx), // cols
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
      LLVMBuildExtractValue(builder, array_struct, 2, "get_array_data_ptr");

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
      LLVMBuildExtractValue(builder, array_struct, 2, "get_array_data_ptr");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr, (LLVMValueRef[]){index}, 1,
                    "element_ptr");

  LLVMBuildStore(builder, value, element_ptr);
  return array;
}

// forward-decl
LLVMValueRef __allocate_coroutine_array(JITLangCtx *ctx, LLVMBuilderRef builder,
                                        LLVMTypeRef element_type,
                                        LLVMValueRef size_const);

LLVMValueRef codegen_create_array(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  int array_size = ast->data.AST_LIST.len;

  if (array_size > 0) {
  } else {
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMTypeRef empty_type = type_to_llvm_type(ast->type, ctx, module);
    LLVMValueRef size_const = LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0,
                                           0);
    LLVMValueRef array_struct = LLVMGetUndef(empty_type);
    array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                        "insert_array_size");
    if (is_string_type(ast->type)) {
      LLVMValueRef null_data =
          LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0));
      array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 1,
                                          "insert_array_offset");
      array_struct = LLVMBuildInsertValue(builder, array_struct, null_data, 2,
                                          "insert_array_data");
    } else {
      Type *element_type_ref = ast->type->data.T_CONS.args[0];
      LLVMTypeRef element_type =
          element_type_ref->kind == T_FN
              ? generic_ptr_type_for_module(module)
              : type_to_llvm_type(element_type_ref, ctx, module);
      LLVMValueRef null_data =
          LLVMConstNull(LLVMPointerType(element_type, 0));
      array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 1,
                                          "insert_array_offset");
      array_struct = LLVMBuildInsertValue(builder, array_struct, null_data, 2,
                                          "insert_array_data");
    }

    return array_struct;
  }

  Type *array_type_ref = ast->type;
  Type *element_type_ref = array_type_ref->data.T_CONS.args[0];
  if (is_generic(element_type_ref)) {
    element_type_ref = specialize_type_for_codegen(element_type_ref, ctx);
  }
  LLVMTypeRef element_type =
      element_type_ref->kind == T_FN
          ? generic_ptr_type_for_module(module)
          : type_to_llvm_type(element_type_ref, ctx, module);
  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const =
      LLVMConstInt(i32_type_for(element_type), array_size, 0);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr;
  // if (ctx->allocator) {
  //   printf("use custom allocator in this code\n");
  //   print_ast(ast);
  // } else
  // printf("create array %d \n", find_allocation_strategy(ast, ctx));
  // print_ast(ast);
  if (ctx->coro_ctx) {
    data_ptr =
        __allocate_coroutine_array(ctx, builder, element_type, size_const);
  } else if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {

    data_ptr = LLVMBuildAlloca(builder, LLVMArrayType(element_type, array_size),
                               "array_data_alloc");

  } else {
    // data_ptr = LLVMBuildArrayMalloc(builder, element_type, size_const,
    //                                 "array_data_alloc");

    data_ptr = LLVMBuildMalloc(builder, LLVMArrayType(element_type, array_size),
                               "array_data_alloc");
  }

  array_struct = LLVMBuildInsertValue(builder, array_struct, size_const, 0,
                                      "insert_array_size");
  array_struct = LLVMBuildInsertValue(
      builder, array_struct,
      LLVMConstInt(LLVMInt32TypeInContext(LLVMGetModuleContext(module)), 0, 0),
      1, "insert_array_offset");

  TICtx ti_ctx = {.env = ctx->env};
  if (is_constant_expr(ast, &ti_ctx) && ctx->coro_ctx) {
    LLVMBuilderRef init_builder = coro_create_frame_builder(ctx, module);

    if (!init_builder) {
      return NULL;
    }

    for (int i = 0; i < array_size; i++) {
      LLVMValueRef element =
          codegen(ast->data.AST_LIST.items + i, ctx, module, init_builder);

      LLVMValueRef element_ptr =
          LLVMBuildGEP2(init_builder, element_type, data_ptr,
                        (LLVMValueRef[]){LLVMConstInt(i32_type_for(element_type),
                                                      i, 0)},
                        1, "element_ptr");
      LLVMBuildStore(init_builder, element, element_ptr);
    }

    LLVMDisposeBuilder(init_builder);
  } else {
    for (int i = 0; i < array_size; i++) {
      LLVMValueRef element =
          codegen(ast->data.AST_LIST.items + i, ctx, module, builder);

      LLVMValueRef element_ptr =
          LLVMBuildGEP2(builder, element_type, data_ptr,
                        (LLVMValueRef[]){LLVMConstInt(i32_type_for(element_type),
                                                      i, 0)},
                        1, "element_ptr");
      LLVMBuildStore(builder, element, element_ptr);
    }
  }

  array_struct = LLVMBuildInsertValue(builder, array_struct, data_ptr, 2,
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

  Type *_array_type = ast->type;
  Type *el_type = _array_type->data.T_CONS.args[0];
  // print_ast(ast);
  // print_type(_array_type);
  // print_type((ast->data.AST_APPLICATION.args + 1)->type);

  LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);

  LLVMValueRef size =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  bool is_const_size;
  long long size_const;
  if (LLVMIsConstant(size)) {
    is_const_size = true;
    size_const = LLVMConstIntGetZExtValue(size);
  }

  LLVMValueRef array_struct = LLVMGetUndef(array_type);

  LLVMValueRef data_ptr;
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC &&
      ctx->coro_ctx == NULL) {
    if (is_const_size) {

      data_ptr = LLVMBuildAlloca(
          builder, LLVMArrayType(element_type, size_const), "element_ptr");
    } else {
      data_ptr =
          LLVMBuildArrayAlloca(builder, element_type, size, "element_ptr");
    }

  } else {

    if (is_const_size) {

      data_ptr = LLVMBuildMalloc(
          builder, LLVMArrayType(element_type, size_const), "element_ptr");
    } else {
      data_ptr =
          LLVMBuildArrayMalloc(builder, element_type, size, "element_ptr");
    }
  }

  array_struct =
      LLVMBuildInsertValue(builder, array_struct, size, 0, "insert_array_size");
  array_struct = LLVMBuildInsertValue(
      builder, array_struct,
      LLVMConstInt(LLVMInt32TypeInContext(LLVMGetModuleContext(module)), 0, 0),
      1, "insert_array_offset");

  Type *ftype = ast->data.AST_APPLICATION.function->type;
  Type *fill_func_type = ftype->data.T_FN.to->data.T_FN.from;

  ast->data.AST_APPLICATION.args[1].type = fill_func_type;

  LLVMValueRef fill_func =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry_block);
  LLVMBasicBlockRef loop_block =
      append_block_in_module(module, function, "loop");
  LLVMBasicBlockRef after_block =
      append_block_in_module(module, function, "after_loop");

  LLVMTypeRef i32_type = LLVMInt32TypeInContext(LLVMGetModuleContext(module));
  LLVMValueRef counter = LLVMBuildAlloca(builder, i32_type, "counter");
  LLVMBuildStore(builder, LLVMConstInt(i32_type, 0, 0), counter);

  LLVMBuildBr(builder, loop_block);

  LLVMPositionBuilderAtEnd(builder, loop_block);

  LLVMValueRef current_idx =
      LLVMBuildLoad2(builder, i32_type, counter, "current_idx");

  LLVMValueRef idx_args[] = {current_idx};

  LLVMValueRef element = LLVMBuildCall2(
      builder,
      LLVMFunctionType(element_type, (LLVMTypeRef[]){i32_type}, 1, 0),
      fill_func, idx_args, 1, "fill_element");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, element, element_ptr);

  LLVMValueRef next_idx = LLVMBuildAdd(
      builder, current_idx, LLVMConstInt(i32_type, 1, 0), "next_idx");
  LLVMBuildStore(builder, next_idx, counter);

  LLVMValueRef end_cond =
      LLVMBuildICmp(builder, LLVMIntSLT, next_idx, size, "end_cond");

  LLVMBuildCondBr(builder, end_cond, loop_block, after_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  array_struct = LLVMBuildInsertValue(builder, array_struct, data_ptr, 2,
                                      "insert_array_data");
  return array_struct;
}

LLVMValueRef ArrayFillConstHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  // printf("array fill const handler\n");
  // print_ast(ast);

  Type *_array_type = ast->type;
  Type *el_type = _array_type->data.T_CONS.args[0];

  LLVMTypeRef element_type =
      el_type->kind == T_FN ? generic_ptr_type_for_module(module)
                            : type_to_llvm_type(el_type, ctx, module);

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef size_const =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

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
  array_struct = LLVMBuildInsertValue(
      builder, array_struct,
      LLVMConstInt(LLVMInt32TypeInContext(LLVMGetModuleContext(module)), 0, 0),
      1, "insert_array_offset");

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry_block);
  LLVMBasicBlockRef loop_block =
      append_block_in_module(module, function, "loop");
  LLVMBasicBlockRef after_block =
      append_block_in_module(module, function, "after_loop");

  LLVMTypeRef i32_type = LLVMInt32TypeInContext(LLVMGetModuleContext(module));
  LLVMValueRef counter = LLVMBuildAlloca(builder, i32_type, "counter");
  LLVMBuildStore(builder, LLVMConstInt(i32_type, 0, 0), counter);

  LLVMBuildBr(builder, loop_block);

  LLVMPositionBuilderAtEnd(builder, loop_block);

  LLVMValueRef current_idx =
      LLVMBuildLoad2(builder, i32_type, counter, "current_idx");

  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){current_idx}, 1, "element_ptr");
  LLVMBuildStore(builder, const_fill_value, element_ptr);

  LLVMValueRef next_idx = LLVMBuildAdd(
      builder, current_idx, LLVMConstInt(i32_type, 1, 0), "next_idx");
  LLVMBuildStore(builder, next_idx, counter);

  LLVMValueRef end_cond =
      LLVMBuildICmp(builder, LLVMIntSLT, next_idx, size_const, "end_cond");

  LLVMBuildCondBr(builder, end_cond, loop_block, after_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  array_struct = LLVMBuildInsertValue(builder, array_struct, data_ptr, 2,
                                      "insert_array_data");
  return array_struct;
}

LLVMValueRef ArraySuccHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *_array_type = ast->type;
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
                    LLVMConstInt(LLVMTypeOf(current_size), 0, 0),
                    "is_size_gt_zero");
  LLVMValueRef size_mask =
      LLVMBuildZExt(builder, is_size_gt_zero, LLVMTypeOf(current_size),
                    "size_mask");

  LLVMValueRef size_decrement = size_mask; // Already 0 or 1

  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_decrement, "new_size");

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 2, "data_ptr");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array_struct, 1, "array_offset");
  LLVMValueRef new_offset =
      LLVMBuildAdd(builder, current_offset, size_mask, "new_offset");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){size_mask}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_offset,
                                          1, "insert_new_offset");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 2, "insert_new_data_ptr");

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

  Type *_array_type = ast->type;
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
      LLVMBuildExtractValue(builder, array_struct, 2, "data_ptr");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array_struct, 1, "array_offset");
  LLVMValueRef new_offset =
      LLVMBuildAdd(builder, current_offset, offset_val, "new_offset");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){offset_val}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_offset,
                                          1, "insert_new_offset");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 2, "insert_new_data_ptr");

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

  Type *_array_type = ast->type;
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
                    LLVMConstInt(LLVMTypeOf(current_size), 0, 0),
                    "is_size_gt_zero");
  LLVMValueRef size_mask =
      LLVMBuildZExt(builder, is_size_gt_zero, LLVMTypeOf(current_size),
                    "size_mask");

  LLVMValueRef size_decrement = LLVMBuildMul(
      builder, offset_val, size_mask,
      "0_or_1_times_offset"); // size is Already 0 or 1 * offset_val

  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_decrement, "new_size");

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array_struct, 2, "data_ptr");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array_struct, 1, "array_offset");
  LLVMValueRef new_offset =
      LLVMBuildAdd(builder, current_offset, offset_val, "new_offset");

  // Calculate the pointer offset in the same way (0 or 1 based on original
  // size) This ensures we don't move the pointer if the size was 0
  LLVMValueRef new_data_ptr =
      LLVMBuildGEP2(builder, element_type, data_ptr,
                    (LLVMValueRef[]){offset_val}, 1, "new_data_ptr");

  // Build the new array struct
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                          0, "insert_new_size");
  new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_offset,
                                          1, "insert_new_offset");
  new_array_struct = LLVMBuildInsertValue(
      builder, new_array_struct, new_data_ptr, 2, "insert_new_data_ptr");

  return new_array_struct;
}

LLVMValueRef ArrayStrideHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef array =
      codegen(ast->data.AST_APPLICATION.args + 2, ctx, module, builder);

  Type *_array_type = ast->type;
  return NULL;
}

LLVMValueRef CStrHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  Type *arr_type = ast->data.AST_APPLICATION.args->type;
  LLVMTypeRef llvm_arr_type = type_to_llvm_type(arr_type, ctx, module);
  LLVMValueRef arr =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  arr = get_array_struct(arr, llvm_arr_type, builder);

  return LLVMBuildExtractValue(builder, arr, 2, "get_array_data_ptr");
}

LLVMValueRef ArrayConstructor(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  print_ast(ast);
  return NULL;
}

LLVMValueRef ArrayConstructorHandler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  print_ast(ast);
  Type *ptr_type = (ast->data.AST_APPLICATION.args + 1)->type;
  LLVMValueRef data_ptr =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMValueRef size =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMValueRef v = LLVMGetUndef(tmp_generic_codegen_array_type());
  v = LLVMBuildInsertValue(builder, v, size, 0, "insert_arr_size");
  v = LLVMBuildInsertValue(builder, v, LLVMConstInt(LLVMInt32Type(), 0, 0), 1,
                           "insert_arr_offset");
  v = LLVMBuildInsertValue(builder, v, data_ptr, 2, "insert_arr_data");
  return v;
}
LLVMValueRef ArrayOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {}
