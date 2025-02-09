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
LLVMTypeRef create_llvm_list_type(Type *list_el_type, TypeEnv *env,
                                  LLVMModuleRef module) {
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

LLVMValueRef ListConcatHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  Type *list_type = ast->md;
  LLVMTypeRef llvm_list_node_type = llnode_type(
      type_to_llvm_type(list_type->data.T_CONS.args[0], ctx->env, module));

  LLVMValueRef append_list =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  // Create basic blocks for the loop
  LLVMBasicBlockRef entry = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry);
  LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(function, "loop");
  LLVMBasicBlockRef after_loop = LLVMAppendBasicBlock(function, "after_loop");

  // Store the initial list pointer
  LLVMValueRef current = list;

  // Branch to loop
  LLVMBuildBr(builder, loop_block);
  LLVMPositionBuilderAtEnd(builder, loop_block);

  // Create PHI node for the current pointer
  LLVMValueRef phi = LLVMBuildPhi(
      builder, LLVMPointerType(llvm_list_node_type, 0), "current_phi");
  LLVMValueRef incoming_values[] = {list};
  LLVMBasicBlockRef incoming_blocks[] = {entry};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 1);

  // Load the next pointer from the current node
  LLVMValueRef next_ptr_ptr =
      LLVMBuildStructGEP2(builder, llvm_list_node_type, phi, 1, "next_ptr_ptr");

  LLVMValueRef next_ptr =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0),
                     next_ptr_ptr, "next_ptr");

  // Check if next pointer is null
  LLVMValueRef is_null = LLVMBuildIsNull(builder, next_ptr, "is_null");

  // Create the loop condition
  LLVMBuildCondBr(builder, is_null, after_loop, loop_block);

  // Update PHI node with the next pointer
  incoming_values[0] = next_ptr;
  incoming_blocks[0] = loop_block;
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 1);

  // Position builder at the end of loop for final node update
  LLVMPositionBuilderAtEnd(builder, after_loop);

  // Set the next pointer of the last node to append_list
  next_ptr_ptr = LLVMBuildStructGEP2(builder, llvm_list_node_type, phi, 1,
                                     "final_next_ptr_ptr");
  LLVMBuildStore(builder, append_list, next_ptr_ptr);

  // Return the original list
  return list;
}

LLVMValueRef ListPrependHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  Type *list_type = ast->md;
  LLVMTypeRef llvm_list_node_type = llnode_type(
      type_to_llvm_type(list_type->data.T_CONS.args[0], ctx->env, module));

  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  return codegen_list_prepend(val, list, ctx, module, builder);
}

LLVMValueRef _codegen_string(const char *chars, int length, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_list_to_string(LLVMValueRef val, Type *val_type,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return _codegen_string("[]", 2, ctx, module, builder);
}
