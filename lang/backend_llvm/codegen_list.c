#include "backend_llvm/codegen_list.h"
#include "backend_llvm/codegen_types.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type) {
  LLVMTypeRef node_types[2];
  node_types[0] = llvm_el_type;
  node_types[1] = LLVMPointerType(LLVMVoidType(), 0); // Pointer to next node

  // Create a struct type for the list node: { element, next_ptr }
  LLVMTypeRef node_type = LLVMStructType(node_types, 2, 0);
  return node_type;
}

// Function to create an LLVM tuple type
LLVMTypeRef list_type(Type *list_el_type) {
  // Convert the custom Type to LLVMTypeRef
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type);
  LLVMTypeRef node_type = llnode_type(llvm_el_type);

  // The list type is a pointer to the node type
  return LLVMPointerType(node_type, 0);
}

static LLVMValueRef null_node(LLVMTypeRef node_type) {
  return LLVMConstNull(LLVMPointerType(node_type, 0));
}

// Function to create a new node
LLVMValueRef ll_create_list_node_(LLVMTypeRef node_type, LLVMValueRef data,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  LLVMValueRef alloced_node =
      ctx->stack_ptr == 0 ? LLVMBuildMalloc(builder, node_type, "new_node")
                          : LLVMBuildAlloca(builder, node_type, "new_node");

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

LLVMValueRef ll_create_list_node(LLVMTypeRef node_type, LLVMValueRef data,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  LLVMValueRef alloced_node =
      ctx->stack_ptr == 0 ? LLVMBuildMalloc(builder, node_type, "new_node")
                          : LLVMBuildAlloca(builder, node_type, "new_node");

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

static LLVMValueRef ll_insert_at_head(LLVMValueRef old_head,
                                      LLVMValueRef new_head,
                                      LLVMTypeRef node_type,
                                      LLVMBuilderRef builder) {

  LLVMValueRef next_ptr =
      LLVMBuildStructGEP2(builder, node_type, new_head, 1, "next_ptr");

  LLVMBuildStore(builder, old_head, next_ptr);
  return new_head;
}

// LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
//                           LLVMBuilderRef builder) {
//   Type *list_el_type = *((Type *)ast->md)->data.T_CONS.args;
//   LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type);
//   LLVMTypeRef node_type = llnode_type(llvm_el_type);
//   int end = ast->data.AST_LIST.len;
//
//   if (end == 0) {
//     return LLVMConstNull(LLVMPointerType(node_type, 0));
//   }
//
//   Ast *item_ast = ast->data.AST_LIST.items;
//   LLVMValueRef head = ll_create_list_node(
//       node_type, codegen(item_ast, ctx, module, builder), ctx, module,
//       builder);
//   LLVMValueRef current = head;
//
//   for (int i = 1; i < end; i++) {
//     LLVMValueRef data = codegen(item_ast + i, ctx, module, builder);
//     LLVMValueRef new_node =
//         ll_create_list_node(node_type, data, ctx, module, builder);
//
//     // Set the next pointer of the current node to the new node
//     LLVMValueRef next_ptr =
//         LLVMBuildStructGEP2(builder, node_type, current, 1, "next_ptr");
//     LLVMBuildStore(builder, new_node, next_ptr);
//
//     current = new_node;
//   }
//
//   return head;
// }

LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  Type *list_el_type = *((Type *)ast->md)->data.T_CONS.args;
  LLVMTypeRef llvm_el_type = type_to_llvm_type(list_el_type);
  LLVMTypeRef node_type = llnode_type(llvm_el_type);
  int end = ast->data.AST_LIST.len;

  if (end == 0) {
    return null_node(node_type);
  }

  Ast *item_ast = ast->data.AST_LIST.items;

  LLVMValueRef head = null_node(node_type);

  while (end--) {
    LLVMValueRef new_head = ll_create_list_node(
        node_type, codegen(item_ast + end, ctx, module, builder), ctx, module,
        builder);

    // Set the next pointer of the current node to the new node
    LLVMValueRef next_ptr =
        LLVMBuildStructGEP2(builder, node_type, new_head, 1, "next_ptr");
    LLVMBuildStore(builder, head, next_ptr);
    head = new_head;
  }

  // LLVMValueRef current = head;
  //
  // for (int i = 1; i < end; i++) {
  //   LLVMValueRef data = codegen(item_ast + i, ctx, module, builder);
  //   LLVMValueRef new_node =
  //       ll_create_list_node(node_type, data, ctx, module, builder);
  //
  //   // Set the next pointer of the current node to the new node
  //   LLVMValueRef next_ptr =
  //       LLVMBuildStructGEP2(builder, node_type, current, 1, "next_ptr");
  //   LLVMBuildStore(builder, new_node, next_ptr);
  //
  //   current = new_node;
  // }
  //
  return head;
}
