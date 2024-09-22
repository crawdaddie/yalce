#include "backend_llvm/list.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
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

LLVMTypeRef codegen_array_type(Type *type, TypeEnv *env, LLVMModuleRef module) {

  Type *el_type = type;
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, env, module);

  return LLVMPointerType(llvm_el_type, 0);
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

LLVMValueRef codegen_array_at(LLVMValueRef array_ptr, LLVMValueRef idx,
                              LLVMTypeRef el_type, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMTypeRef ty = LLVMArrayType(el_type, 0);
  LLVMValueRef element_ptr = array_el_ptr(array_ptr, ty, idx, builder);
  return LLVMBuildLoad2(builder, el_type, element_ptr, "");
}

LLVMValueRef codegen_array_set(LLVMValueRef array_ptr, LLVMValueRef idx,
                               LLVMValueRef val, LLVMTypeRef el_type,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMTypeRef ty = LLVMArrayType(el_type, 0);
  LLVMValueRef element_ptr = array_el_ptr(array_ptr, ty, idx, builder);
  return LLVMBuildStore(builder, val, element_ptr);
}

LLVMValueRef codegen_array(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  Type *array_type = ast->md;
  Type *el_type = array_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx->env, module);

  int len = ast->data.AST_LIST.len;

  LLVMTypeRef llvm_array_type = LLVMArrayType(llvm_el_type, len);

  LLVMValueRef len_val = LLVMConstInt(LLVMInt32Type(), len, 0);

  LLVMValueRef array_ptr;
  if (ctx->stack_ptr == 0) {
    array_ptr = LLVMBuildMalloc(builder, llvm_array_type, "heap_array");
  } else {
    array_ptr = LLVMBuildAlloca(builder, llvm_array_type, "stack_array");
  }

  for (int i = 0; i < len; i++) {

    LLVMValueRef value =
        codegen(ast->data.AST_LIST.items + i, ctx, module, builder);

    codegen_array_set(array_ptr, LLVMConstInt(LLVMInt32Type(), i, 0), value,
                      llvm_el_type, module, builder);
  }

  return array_ptr;
}

LLVMValueRef __codegen_array(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef array_ptr = codegen_array(ast, ctx, module, builder);

  int len = ast->data.AST_LIST.len;
  LLVMValueRef len_val = LLVMConstInt(LLVMInt32Type(), len, 0);
  LLVMTypeRef sized_type = LLVMStructType(
      (LLVMTypeRef[]){LLVMInt32Type(),
                      codegen_array_type(ast->md, ctx->env, module)},
      2, 0);

  LLVMValueRef sized = LLVMGetUndef(sized_type);
  LLVMBuildInsertValue(builder, sized, len_val, 0, "");
  LLVMBuildInsertValue(builder, sized, array_ptr, 1, "");
  LLVMDumpValue(sized);
  printf("\n");
  return sized;
}

// Function to get size of fixed-size array
LLVMValueRef get_array_size(LLVMBuilderRef builder, LLVMValueRef array_ptr) {
  // Get the type of the pointer
  LLVMTypeRef ptr_type = LLVMTypeOf(array_ptr);

  // Get the element type (which should be the array type)
  LLVMTypeRef array_type = LLVMGetElementType(ptr_type);

  // Make sure it's actually an array type
  if (LLVMGetTypeKind(array_type) != LLVMArrayTypeKind) {
    // Handle error: not an array type
    return NULL;
  }

  // Get the length of the array
  unsigned array_length = LLVMGetArrayLength(array_type);

  // Create an LLVM constant integer with this length
  return LLVMConstInt(LLVMInt32Type(), array_length, 0);
}
