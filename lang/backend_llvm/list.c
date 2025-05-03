#include "backend_llvm/list.h"
#include "adt.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "serde.h"
#include "tuple.h"
#include "types/inference.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type) {
  LLVMTypeRef node_types[2];
  node_types[0] = llvm_el_type;
  node_types[1] = LLVMPointerType(LLVMVoidType(), 0); // Pointer to next node

  LLVMTypeRef node_type = LLVMStructType(node_types, 2, 0);
  return node_type;
}

// Function to create an LLVM list type
LLVMTypeRef create_llvm_list_type(Type *list_el_type, TypeEnv *env,
                                  LLVMModuleRef module) {
  if (list_el_type->kind == T_VAR) {
    return GENERIC_PTR;
  }
  if (list_el_type->kind == T_FN) {

    LLVMTypeRef llvm_el_type = GENERIC_PTR;
    LLVMTypeRef node_type = llnode_type(llvm_el_type);

    return LLVMPointerType(node_type, 0);
  }

  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, env, module);
  if (!llvm_el_type) {
    return NULL;
  }
  LLVMTypeRef node_type = llnode_type(llvm_el_type);

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

LLVMValueRef ll_is_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                        LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef null_list = LLVMConstNull(LLVMPointerType(node_type, 0));
  return LLVMBuildICmp(builder, LLVMIntEQ, list, null_list, "is_null");
}

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
  LLVMTypeRef llvm_el_type;
  if (list_el_type->kind == T_FN) {
    llvm_el_type = GENERIC_PTR;
  } else {
    llvm_el_type = type_to_llvm_type(list_el_type, ctx->env, module);
  }

  LLVMTypeRef node_type = llnode_type(llvm_el_type);

  int len = ast->data.AST_LIST.len;

  if (len == 0) {
    return null_node(node_type);
  }

  // Allocate memory for all nodes at once
  LLVMValueRef total_size = LLVMConstInt(LLVMInt32Type(), len, 0);
  LLVMValueRef node_size = LLVMSizeOf(node_type);
  LLVMValueRef alloc_size =
      LLVMBuildMul(builder, total_size, node_size, "alloc_size");
  LLVMValueRef memory_block = LLVMBuildMalloc(
      builder, LLVMArrayType(node_type, len), "list_memory_block");

  // Create and link all nodes
  LLVMValueRef current_node = NULL;
  LLVMValueRef head = NULL;
  LLVMValueRef prev_node = NULL;

  for (int i = 0; i < len; i++) {
    // Get the current item value
    Ast *item_ast = &ast->data.AST_LIST.items[i];
    LLVMValueRef item_value = codegen(item_ast, ctx, module, builder);

    Type *item_type = item_ast->md;

    // If the item is a function, we need to bitcast it to a pointer type
    // before storing it in the list
    if (item_type->kind == T_FN) {
      LLVMTypeRef func_ptr_type = GENERIC_PTR;

      // Perform the bitcast
      item_value =
          LLVMBuildBitCast(builder, item_value, func_ptr_type, "func_ptr_cast");
    }

    // Calculate pointer to current node memory location
    LLVMValueRef indices[2];
    indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0); // Array base
    indices[1] = LLVMConstInt(LLVMInt32Type(), i, 0); // Array index
    LLVMValueRef node_ptr =
        LLVMBuildGEP2(builder, LLVMArrayType(node_type, len), memory_block,
                      indices, 2, "node_ptr");

    current_node = node_ptr;

    LLVMValueRef data_ptr =
        LLVMBuildStructGEP2(builder, node_type, node_ptr, 0, "data_ptr");
    LLVMBuildStore(builder, item_value, data_ptr);

    // Save the head pointer if this is the first node
    if (i == 0) {
      head = current_node;
    }

    // Link the previous node to the current one
    if (prev_node != NULL) {
      LLVMValueRef next_ptr =
          LLVMBuildStructGEP2(builder, node_type, prev_node, 1, "next_ptr");
      LLVMBuildStore(builder, current_node, next_ptr);
    }

    prev_node = current_node;
  }

  // Ensure the last node's next pointer is NULL
  if (current_node != NULL) {
    LLVMValueRef next_ptr =
        LLVMBuildStructGEP2(builder, node_type, current_node, 1, "next_ptr");
    LLVMBuildStore(builder, null_node(node_type), next_ptr);
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
  if (!llvm_list_node_type) {
    print_ast(ast);
    return NULL;
  }

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

LLVMValueRef __ListTailHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *list_type = ast->md;
  LLVMTypeRef llvm_list_node_type = llnode_type(
      type_to_llvm_type(list_type->data.T_CONS.args[0], ctx->env, module));
  if (!llvm_list_node_type) {
    print_ast(ast);
    return NULL;
  }

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

  return phi;
}

LLVMValueRef ListRefSetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *list_type = ast->data.AST_APPLICATION.args->md;

  Type *list_el_type = *list_type->data.T_CONS.args;
  if (list_el_type->kind == T_VAR) {
    while (list_el_type->kind == T_VAR) {
      list_el_type = env_lookup(ctx->env, list_el_type->data.T_VAR);
    }
  }

  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef next =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);

  LLVMTypeRef node_type = llnode_type(llvm_list_el_type);

  LLVMValueRef next_str =
      LLVMBuildLoad2(builder, node_type, next, "node_struct");
  LLVMBuildStore(builder, next_str, list);

  return NULL;
}

LLVMValueRef ListTailHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *list_type = ast->md;

  Type *list_el_type = *list_type->data.T_CONS.args;
  if (list_el_type->kind == T_VAR) {
    while (list_el_type->kind == T_VAR) {
      list_el_type = env_lookup(ctx->env, list_el_type->data.T_VAR);
    }
  }

  LLVMTypeRef llvm_el_type;

  if (list_el_type->kind == T_FN) {
    llvm_el_type = GENERIC_PTR;
  } else {
    llvm_el_type = type_to_llvm_type(list_el_type, ctx->env, module);
  }

  LLVMTypeRef llvm_list_node_type = llnode_type(llvm_el_type);

  // Handle null list case
  LLVMValueRef is_null_list = LLVMBuildIsNull(builder, list, "is_null_list");
  LLVMBasicBlockRef entry = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry);
  LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(function, "loop");
  LLVMBasicBlockRef after_loop = LLVMAppendBasicBlock(function, "after_loop");
  LLVMBasicBlockRef null_case = LLVMAppendBasicBlock(function, "null_case");

  // Branch based on whether the list is null
  LLVMBuildCondBr(builder, is_null_list, null_case, loop_block);

  // Handle null list case
  LLVMPositionBuilderAtEnd(builder, null_case);
  LLVMBuildBr(builder, after_loop);

  // Position at loop block
  LLVMPositionBuilderAtEnd(builder, loop_block);

  // Create PHI node for the current pointer
  LLVMValueRef phi = LLVMBuildPhi(
      builder, LLVMPointerType(llvm_list_node_type, 0), "current_phi");

  // Initial value is the list head
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

  // Save the current node before potentially leaving the loop
  LLVMValueRef current_node = phi;

  // Create the loop condition
  LLVMBuildCondBr(builder, is_null, after_loop, loop_block);

  // IMPORTANT: This needs to be after the branch instruction
  // Add a self-reference to the PHI node with the next pointer
  LLVMValueRef loop_values[] = {next_ptr};
  LLVMBasicBlockRef loop_blocks[] = {loop_block};
  LLVMAddIncoming(phi, loop_values, loop_blocks, 1);

  // Position builder at the end of loop for final result
  LLVMPositionBuilderAtEnd(builder, after_loop);

  // Create a PHI node to merge results from both paths
  LLVMValueRef result_phi =
      LLVMBuildPhi(builder, LLVMPointerType(llvm_list_node_type, 0), "result");

  // From null case path: return null
  LLVMValueRef null_values[] = {
      LLVMConstNull(LLVMPointerType(llvm_list_node_type, 0))};
  LLVMBasicBlockRef null_blocks[] = {null_case};
  LLVMAddIncoming(result_phi, null_values, null_blocks, 1);

  // From loop termination: return the current node (which is the tail)
  LLVMValueRef normal_values[] = {current_node};
  LLVMBasicBlockRef normal_blocks[] = {loop_block};
  LLVMAddIncoming(result_phi, normal_values, normal_blocks, 1);

  return result_phi;
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
