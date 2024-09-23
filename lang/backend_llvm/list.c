#include "backend_llvm/list.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "tuple.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type) {
  LLVMTypeRef node_types[2];
  node_types[0] = llvm_el_type;
  node_types[1] = LLVMPointerType(LLVMVoidType(), 0); // Pointer to next node

  // Create a struct type for the list node: { element, next_ptr }
  LLVMTypeRef node_type = LLVMStructType(node_types, 2, 0);
  return node_type;
}

// Function to create an LLVM list type
LLVMTypeRef list_type(Type *list_el_type, TypeEnv *env, LLVMModuleRef module) {
  // Convert the custom Type to LLVMTypeRef
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, env, module);
  LLVMTypeRef node_type = llnode_type(llvm_el_type);

  // The list type is a pointer to the node type
  return LLVMPointerType(node_type, 0);
}

LLVMValueRef null_node(LLVMTypeRef node_type) {
  return LLVMConstNull(LLVMPointerType(node_type, 0));
}

LLVMValueRef ll_create_list_node(LLVMValueRef mem, LLVMTypeRef node_type,
                                 LLVMValueRef data, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef alloced_node =
      mem == NULL ? LLVMBuildMalloc(builder, node_type, "new_node") : mem;

  // Set the data
  LLVMValueRef data_ptr =
      LLVMBuildStructGEP2(builder, node_type, alloced_node, 0, "data_ptr");
  LLVMBuildStore(builder, data, data_ptr);

  // Set the next pointer to null
  LLVMValueRef next_ptr =
      LLVMBuildStructGEP2(builder, node_type, alloced_node, 1, "next_ptr");
  LLVMBuildStore(builder, null_node(node_type), next_ptr);

  return alloced_node;
}

// Helper function to check if a list is null
LLVMValueRef ll_is_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                        LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef null_list = LLVMConstNull(LLVMPointerType(node_type, 0));
  return LLVMBuildICmp(builder, LLVMIntEQ, list, null_list, "is_null");
}

// Helper function to check if a list is null
LLVMValueRef ll_is_not_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                            LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef null_list = LLVMConstNull(LLVMPointerType(node_type, 0));
  return LLVMBuildICmp(builder, LLVMIntNE, list, null_list, "is_not_null");
}
LLVMValueRef ll_get_head_val(LLVMValueRef list, LLVMTypeRef list_el_type,
                             LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  return struct_ptr_get(0, list, node_type, builder);
}

LLVMValueRef ll_get_next(LLVMValueRef list, LLVMTypeRef list_el_type,
                         LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  return struct_ptr_get(1, list, node_type, builder);
}

LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *list_el_type = *((Type *)ast->md)->data.T_CONS.args;
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, ctx->env, module);
  LLVMTypeRef node_type = llnode_type(llvm_el_type);
  int len = ast->data.AST_LIST.len;

  if (len == 0) {
    return null_node(node_type);
  }

  Ast *item_ast = ast->data.AST_LIST.items;

  LLVMValueRef end_node = null_node(node_type);

  LLVMValueRef head = LLVMBuildArrayMalloc(
      builder, node_type, LLVMConstInt(LLVMInt32Type(), len, 0),
      "list_array_malloc"); // malloc an array all at once since we know we'll
                            // need len nodes off the bat
  LLVMValueRef tail = head;

  LLVMValueRef element_size = LLVMSizeOf(node_type);
  for (int i = 0; i < len; i++) {

    // Set the data
    struct_ptr_set(0, tail, node_type,
                   codegen(ast->data.AST_LIST.items + i, ctx, module, builder),
                   builder);

    if (i < len - 1) {
      LLVMValueRef next_tail =
          increment_ptr(tail, node_type, element_size, builder);
      struct_ptr_set(1, tail, node_type, next_tail, builder);

      tail = next_tail;
    } else {
      // Set the final next pointer to null
      struct_ptr_set(1, tail, node_type, end_node, builder);
    }
  }

  return head;
}

LLVMValueRef codegen_list_prepend(LLVMValueRef l, LLVMValueRef list,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  LLVMTypeRef el_type = LLVMTypeOf(l);
  LLVMTypeRef node_type = llnode_type(el_type);
  LLVMValueRef node =
      ll_create_list_node(NULL, node_type, l, ctx, module, builder);
  struct_ptr_set(1, node, node_type, list, builder);
  return node;
}

LLVMTypeRef array_struct_type(LLVMTypeRef data_ptr_type) {

  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),
          data_ptr_type,
      },
      2, 0);
}
LLVMTypeRef codegen_array_type(Type *type, TypeEnv *env, LLVMModuleRef module) {

  Type *el_type = type;
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, env, module);

  return array_struct_type(LLVMPointerType(llvm_el_type, 0));
}

LLVMValueRef array_el_ptr(LLVMValueRef array, LLVMTypeRef llvm_array_type,
                          LLVMValueRef idx, LLVMBuilderRef builder) {

  return LLVMBuildGEP2(builder, llvm_array_type, array,
                       (LLVMValueRef[]){
                           LLVMConstInt(LLVMInt32Type(), 0, 0),
                           idx // Array index
                       },
                       2, "element_ptr");
}

LLVMValueRef codegen_array_at(LLVMValueRef array, LLVMValueRef idx,
                              LLVMTypeRef el_type, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  LLVMTypeRef struct_type = array_struct_type(LLVMPointerType(el_type, 0));

  LLVMValueRef array_ptr = codegen_tuple_access(1, array, struct_type, builder);

  LLVMTypeRef ty = LLVMArrayType(el_type, 0);
  LLVMValueRef element_ptr = array_el_ptr(array_ptr, ty, idx, builder);
  return LLVMBuildLoad2(builder, el_type, element_ptr, "");
}

// Function to get size of fixed-size array
LLVMValueRef codegen_get_array_size(LLVMBuilderRef builder,
                                    LLVMValueRef array) {

  // printf("codegen get array size\n");
  // LLVMDumpType(LLVMTypeOf(array));
  // printf("\n");
  // LLVMValueRef element_ptr = LLVMBuildStructGEP2(builder, LLVMTypeOf(array),
  //                                                array, 0,
  //                                                "get_size_element");
  // LLVMTypeRef element_type = LLVMInt32Type();
  // return LLVMBuildLoad2(builder, element_type, element_ptr, "size_load");
  //
  // Check if the tuple is a pointer type
  if (LLVMGetTypeKind(LLVMTypeOf(array)) != LLVMPointerTypeKind) {
    // If it's not a pointer, use LLVMBuildExtractValue - extracts value from a
    // direct value rather than a ptr which needs a GEP2 instruction
    return LLVMBuildExtractValue(builder, array, 0, "struct_element");
  }

  LLVMValueRef element_ptr = LLVMBuildStructGEP2(builder, LLVMTypeOf(array),
                                                 array, 0, "get_tuple_element");

  LLVMTypeRef element_type = LLVMInt32Type();

  return LLVMBuildLoad2(builder, element_type, element_ptr,
                        "tuple_element_load");
}

LLVMValueRef codegen_array_set(LLVMValueRef array, LLVMValueRef idx,
                               LLVMValueRef val, LLVMTypeRef el_type,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef struct_type = array_struct_type(LLVMPointerType(el_type, 0));
  LLVMValueRef array_ptr = codegen_tuple_access(1, array, struct_type, builder);
  LLVMTypeRef ty = LLVMArrayType(el_type, 0);
  LLVMValueRef element_ptr = array_el_ptr(array_ptr, ty, idx, builder);
  return LLVMBuildStore(builder, val, element_ptr);
}

LLVMValueRef _codegen_array(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  Type *array_type = ast->md;
  Type *el_type = array_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx->env, module);

  int len = ast->data.AST_LIST.len;

  LLVMTypeRef llvm_array_type = LLVMArrayType(llvm_el_type, len);

  LLVMValueRef len_val = LLVMConstInt(LLVMInt32Type(), len, 0);

  LLVMValueRef data_ptr;
  if (ctx->stack_ptr == 0) {
    data_ptr = LLVMBuildMalloc(builder, llvm_array_type, "heap_array");
  } else {
    data_ptr = LLVMBuildAlloca(builder, llvm_array_type, "stack_array");
  }

  for (int i = 0; i < len; i++) {

    // Calculate pointer to array element
    LLVMValueRef element_ptr =
        LLVMBuildGEP2(builder, llvm_array_type, data_ptr,
                      (LLVMValueRef[]){
                          LLVMConstInt(LLVMInt32Type(), 0, 0),
                          LLVMConstInt(LLVMInt32Type(), i, 0) // Array index
                      },
                      2, "element_ptr");

    // Create value to store (let's store the index itself for this example)
    LLVMValueRef value =
        codegen(ast->data.AST_LIST.items + i, ctx, module, builder);

    // Store the value
    LLVMBuildStore(builder, value, element_ptr);
  }

  return data_ptr;
}

LLVMValueRef codegen_array(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  int len = ast->data.AST_LIST.len;
  LLVMValueRef data_ptr = _codegen_array(ast, ctx, module, builder);
  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);
  // Create struct type
  //

  LLVMTypeRef struct_type = array_struct_type(data_ptr_type);

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str =
      LLVMBuildInsertValue(builder, str, LLVMConstInt(LLVMInt32Type(), len, 0),
                           0, "insert_array_size");
  return str;
}

LLVMValueRef codegen_array_init(LLVMValueRef size, LLVMValueRef item,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  LLVMTypeRef llvm_el_type = LLVMTypeOf(item);

  LLVMValueRef len_val = size;

  LLVMValueRef array_ptr;
  if (ctx->stack_ptr == 0) {
    array_ptr =
        LLVMBuildArrayMalloc(builder, llvm_el_type, len_val, "heap_array");
  } else {
    array_ptr =
        LLVMBuildArrayAlloca(builder, llvm_el_type, len_val, "stack_array");
  }

  return array_ptr;
}
