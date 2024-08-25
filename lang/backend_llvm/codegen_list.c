#include "backend_llvm/codegen_list.h"
#include "backend_llvm/codegen_types.h"
#include "util.h"
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
LLVMTypeRef list_type(Type *list_el_type, TypeEnv *env) {
  // Convert the custom Type to LLVMTypeRef
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, env);
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

  Type *list_el_type = *((Type)ast->md).data.T_CONS.args;
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, ctx->env);
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
