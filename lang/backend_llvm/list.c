#include "backend_llvm/list.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "types/inference.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static LLVMContextRef type_context(LLVMTypeRef type) {
  return LLVMGetTypeContext(type);
}

static LLVMTypeRef generic_ptr_type_for_context(LLVMContextRef context) {
  return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
}

static LLVMTypeRef generic_ptr_type_for_module(LLVMModuleRef module) {
  return generic_ptr_type_for_context(LLVMGetModuleContext(module));
}

static LLVMBasicBlockRef append_block_in_module(LLVMModuleRef module,
                                                LLVMValueRef function,
                                                const char *name) {
  return LLVMAppendBasicBlockInContext(LLVMGetModuleContext(module), function,
                                       name);
}

static Type *list_operand_type(Ast *operand_ast, JITLangCtx *ctx) {
  if (!operand_ast) {
    return NULL;
  }

  JITSymbol *sym = lookup_id_ast(operand_ast, ctx);
  if (sym && sym->symbol_type) {
    return specialize_type_for_codegen(sym->symbol_type, ctx);
  }

  return specialize_type_for_codegen(operand_ast->type, ctx);
}

static Type *list_element_type_from_operand(Ast *operand_ast, JITLangCtx *ctx) {
  Type *list_type = list_operand_type(operand_ast, ctx);
  if (!list_type || !is_list_type(list_type)) {
    return NULL;
  }
  return type_of_list(list_type);
}

LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type) {
  LLVMContextRef llvm_ctx = type_context(llvm_el_type);
  LLVMTypeRef node_types[2];
  node_types[0] = llvm_el_type;
  node_types[1] = generic_ptr_type_for_context(llvm_ctx);

  LLVMTypeRef node_type = LLVMStructTypeInContext(llvm_ctx, node_types, 2, 0);
  return node_type;
}

// Function to create an LLVM list type
LLVMTypeRef create_llvm_list_type(Type *list_el_type, JITLangCtx *ctx,
                                  LLVMModuleRef module) {
  if (list_el_type->kind == T_VAR) {
    return generic_ptr_type_for_module(module);
  }
  if (list_el_type->kind == T_FN) {

    LLVMTypeRef llvm_el_type = generic_ptr_type_for_module(module);
    LLVMTypeRef node_type = llnode_type(llvm_el_type);

    return LLVMPointerType(node_type, 0);
  }

  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type, ctx, module);
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

  if (!list_el_type) {
    LLVMValueRef null_list = LLVMConstNull(LLVMTypeOf(list));
    return LLVMBuildICmp(builder, LLVMIntEQ, list, null_list, "is_null");
  }

  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef null_list = LLVMConstNull(LLVMPointerType(node_type, 0));
  return LLVMBuildICmp(builder, LLVMIntEQ, list, null_list, "is_null");
}

LLVMValueRef ll_is_not_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                            LLVMBuilderRef builder) {

  if (!list_el_type) {
    LLVMValueRef null_list = LLVMConstNull(LLVMTypeOf(list));
    return LLVMBuildICmp(builder, LLVMIntNE, list, null_list, "is_null");
  }

  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef null_list = LLVMConstNull(LLVMPointerType(node_type, 0));
  return LLVMBuildICmp(builder, LLVMIntNE, list, null_list, "is_not_null");
}
LLVMValueRef ll_get_head_val(LLVMValueRef list, LLVMTypeRef list_el_type,
                             LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef head = struct_ptr_get(0, list, node_type, builder);
  LLVMSetValueName(head, "list_head");
  return head;
}

LLVMValueRef ll_get_next(LLVMValueRef list, LLVMTypeRef list_el_type,
                         LLVMBuilderRef builder) {
  LLVMTypeRef node_type = llnode_type(list_el_type);
  LLVMValueRef rest = struct_ptr_get(1, list, node_type, builder);
  LLVMSetValueName(rest, "list_rest");
  return rest;
}

LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *list_el_type = type_of_list(ast->type);
  LLVMTypeRef llvm_el_type;
  if (list_el_type->kind == T_FN) {
    llvm_el_type = generic_ptr_type_for_module(module);
  } else {
    llvm_el_type = type_to_llvm_type(list_el_type, ctx, module);
  }

  LLVMTypeRef node_type = llnode_type(llvm_el_type);

  int len = ast->data.AST_LIST.len;

  if (len == 0) {
    return null_node(node_type);
  }

  // LLVMValueRef total_size = LLVMConstInt(LLVMInt64Type(), len, 0);
  // LLVMValueRef node_size = LLVMSizeOf(node_type);
  // LLVMValueRef alloc_size =
  //     LLVMBuildMul(builder, total_size, node_size, "alloc_size");

  // Allocate memory for all nodes at once???
  // LLVMValueRef memory_block;
  // TODO: use proper allocation strategy
  // if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
  //   memory_block = LLVMBuildAlloca(builder, LLVMArrayType(node_type, len),
  //                                  "list_memory_block");
  // } else {
  //   memory_block = LLVMBuildMalloc(builder, LLVMArrayType(node_type, len),
  //                                  "list_memory_block");
  // }
  //
  // memory_block = LLVMBuildMalloc(builder, LLVMArrayType(node_type, len),
  //                                "list_memory_block");
  LLVMValueRef mem[len];
  for (int i = 0; i < len; i++) {
    mem[i] =
        LLVMBuildMalloc(builder, node_type, "list_el_memory_non_contiguous");
  }

  // Create and link all nodes
  LLVMValueRef current_node = NULL;
  LLVMValueRef head = NULL;
  LLVMValueRef prev_node = NULL;

  for (int i = 0; i < len; i++) {
    Ast *item_ast = &ast->data.AST_LIST.items[i];
    LLVMValueRef item_value = codegen(item_ast, ctx, module, builder);

    Type *item_type = item_ast->type;

    // If the item is a function, we need to bitcast it to a pointer type
    // before storing it in the list
    if (item_type->kind == T_FN) {
      LLVMTypeRef func_ptr_type = generic_ptr_type_for_module(module);

      // Perform the bitcast
      item_value =
          LLVMBuildBitCast(builder, item_value, func_ptr_type, "func_ptr_cast");
    }

    // Calculate pointer to current node memory location
    // LLVMValueRef indices[2];
    // indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0); // Array base
    // indices[1] = LLVMConstInt(LLVMInt32Type(), i, 0); // Array index
    // LLVMValueRef node_ptr =
    //     LLVMBuildGEP2(builder, LLVMArrayType(node_type, len), memory_block,
    //                   indices, 2, "node_ptr");

    // current_node = node_ptr;
    LLVMValueRef node_ptr = mem[i];
    current_node = node_ptr;

    LLVMValueRef data_ptr =
        LLVMBuildStructGEP2(builder, node_type, node_ptr, 0, "data_ptr");
    LLVMBuildStore(builder, item_value, data_ptr);

    if (i == 0) {
      head = current_node;
    }

    if (prev_node != NULL) {
      LLVMValueRef next_ptr =
          LLVMBuildStructGEP2(builder, node_type, prev_node, 1, "next_ptr");
      LLVMBuildStore(builder, current_node, next_ptr);
    }

    prev_node = current_node;
  }

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

  Type *list_type = ast->type;
  LLVMTypeRef llvm_list_node_type = llnode_type(
      type_to_llvm_type(type_of_list(list_type), ctx, module));
  if (!llvm_list_node_type) {
    // print_ast(ast);
    return NULL;
  }

  LLVMValueRef append_list =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMBasicBlockRef entry = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry);
  LLVMBasicBlockRef loop_block =
      append_block_in_module(module, function, "loop");
  LLVMBasicBlockRef after_loop =
      append_block_in_module(module, function, "after_loop");

  LLVMValueRef current = list;

  LLVMBuildBr(builder, loop_block);
  LLVMPositionBuilderAtEnd(builder, loop_block);

  LLVMValueRef phi = LLVMBuildPhi(
      builder, LLVMPointerType(llvm_list_node_type, 0), "current_phi");
  LLVMValueRef incoming_values[] = {list};
  LLVMBasicBlockRef incoming_blocks[] = {entry};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 1);

  LLVMValueRef next_ptr_ptr =
      LLVMBuildStructGEP2(builder, llvm_list_node_type, phi, 1, "next_ptr_ptr");

  LLVMValueRef next_ptr =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0),
                     next_ptr_ptr, "next_ptr");

  LLVMValueRef is_null = LLVMBuildIsNull(builder, next_ptr, "is_null");

  LLVMBuildCondBr(builder, is_null, after_loop, loop_block);

  incoming_values[0] = next_ptr;
  incoming_blocks[0] = loop_block;
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 1);

  LLVMPositionBuilderAtEnd(builder, after_loop);

  next_ptr_ptr = LLVMBuildStructGEP2(builder, llvm_list_node_type, phi, 1,
                                     "final_next_ptr_ptr");

  LLVMBuildStore(builder, append_list, next_ptr_ptr);

  return list;
}

LLVMValueRef ListRefSetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *list_el_type =
      list_element_type_from_operand(ast->data.AST_APPLICATION.args, ctx);

  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef next =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMTypeRef llvm_list_el_type =
      (!list_el_type || list_el_type->kind == T_VAR ||
       list_el_type->kind == T_FN)
          ? generic_ptr_type_for_module(module)
          : type_to_llvm_type(list_el_type, ctx, module);

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

  Type *list_el_type =
      list_element_type_from_operand(ast->data.AST_APPLICATION.args, ctx);

  LLVMTypeRef llvm_el_type;

  if (!list_el_type || list_el_type->kind == T_VAR ||
      list_el_type->kind == T_FN) {
    llvm_el_type = generic_ptr_type_for_module(module);
  } else {
    llvm_el_type = type_to_llvm_type(list_el_type, ctx, module);
  }

  LLVMTypeRef llvm_list_node_type = llnode_type(llvm_el_type);

  LLVMValueRef is_null_list = LLVMBuildIsNull(builder, list, "is_null_list");
  LLVMBasicBlockRef entry = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(entry);
  LLVMBasicBlockRef loop_block =
      append_block_in_module(module, function, "loop");
  LLVMBasicBlockRef after_loop =
      append_block_in_module(module, function, "after_loop");
  LLVMBasicBlockRef null_case =
      append_block_in_module(module, function, "null_case");

  LLVMBuildCondBr(builder, is_null_list, null_case, loop_block);

  LLVMPositionBuilderAtEnd(builder, null_case);
  LLVMBuildBr(builder, after_loop);

  LLVMPositionBuilderAtEnd(builder, loop_block);

  LLVMValueRef phi = LLVMBuildPhi(
      builder, LLVMPointerType(llvm_list_node_type, 0), "current_phi");

  LLVMValueRef incoming_values[] = {list};
  LLVMBasicBlockRef incoming_blocks[] = {entry};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 1);

  LLVMValueRef next_ptr_ptr =
      LLVMBuildStructGEP2(builder, llvm_list_node_type, phi, 1, "next_ptr_ptr");

  LLVMValueRef next_ptr =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0),
                     next_ptr_ptr, "next_ptr");

  LLVMValueRef is_null = LLVMBuildIsNull(builder, next_ptr, "is_null");

  LLVMValueRef current_node = phi;

  LLVMBuildCondBr(builder, is_null, after_loop, loop_block);

  LLVMValueRef loop_values[] = {next_ptr};
  LLVMBasicBlockRef loop_blocks[] = {loop_block};
  LLVMAddIncoming(phi, loop_values, loop_blocks, 1);

  LLVMPositionBuilderAtEnd(builder, after_loop);

  LLVMValueRef result_phi =
      LLVMBuildPhi(builder, LLVMPointerType(llvm_list_node_type, 0), "result");

  LLVMValueRef null_values[] = {
      LLVMConstNull(LLVMPointerType(llvm_list_node_type, 0))};
  LLVMBasicBlockRef null_blocks[] = {null_case};
  LLVMAddIncoming(result_phi, null_values, null_blocks, 1);

  LLVMValueRef normal_values[] = {current_node};
  LLVMBasicBlockRef normal_blocks[] = {loop_block};
  LLVMAddIncoming(result_phi, normal_values, normal_blocks, 1);

  return result_phi;
}

LLVMValueRef ListPrependHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  Type *el_type =
      list_element_type_from_operand(ast->data.AST_APPLICATION.args + 1, ctx);
  LLVMTypeRef llvm_el_type =
      (!el_type || el_type->kind == T_VAR || el_type->kind == T_FN)
          ? GENERIC_PTR
          : type_to_llvm_type(el_type, ctx, module);
  LLVMTypeRef llvm_list_node_type = llnode_type(llvm_el_type);

  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  // Create new node with correct types
  LLVMValueRef node =
      ll_create_list_node(NULL, llvm_list_node_type, val, ctx, module, builder);

  // Set the next pointer to point to the existing list
  LLVMValueRef next_ptr =
      LLVMBuildStructGEP2(builder, llvm_list_node_type, node, 1, "next_ptr");
  LLVMBuildStore(builder, list, next_ptr);

  return node;

  // LLVMTypeRef llvm_list_node_type =
  //     llnode_type(type_to_llvm_type(el_type, ctx, module));
  //
  // LLVMValueRef val =
  //     codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  // return codegen_list_prepend(val, list, ctx, module, builder);
}

LLVMValueRef _codegen_string(const char *chars, int length, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_list_to_string(LLVMValueRef val, Type *val_type,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return _codegen_string("[]", 2, ctx, module, builder);
}

LLVMValueRef ListEmptyHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  Type *el_type =
      list_element_type_from_operand(ast->data.AST_APPLICATION.args, ctx);
  LLVMValueRef l =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMTypeRef llvm_el_type =
      (!el_type || el_type->kind == T_VAR || el_type->kind == T_FN)
          ? GENERIC_PTR
          : type_to_llvm_type(el_type, ctx, module);
  return ll_is_null(l, llvm_el_type, builder);
}
