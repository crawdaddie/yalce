#include "./loop.h"
#include "array.h"
#include "common.h"
#include "list.h"
#include "parse.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "types/inference.h"
#include "types/type.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_loop_range(Ast *binding, Ast *range, Ast *body,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  const char *loop_var_name = binding->data.AST_IDENTIFIER.value;

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef cond_block =
      LLVMAppendBasicBlock(current_function, "loop.cond");
  LLVMBasicBlockRef body_block =
      LLVMAppendBasicBlock(current_function, "loop.body");
  LLVMBasicBlockRef inc_block =
      LLVMAppendBasicBlock(current_function, "loop.inc");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlock(current_function, "loop.after");

  LLVMValueRef start_val =
      codegen(range->data.AST_RANGE_EXPRESSION.from, ctx, module, builder);

  if (!start_val)
    return NULL;

  LLVMValueRef end_val =
      codegen(range->data.AST_RANGE_EXPRESSION.to, ctx, module, builder);
  if (!end_val)
    return NULL;

  LLVMValueRef loop_var_alloca =
      LLVMBuildAlloca(builder, LLVMInt32Type(), loop_var_name);
  LLVMBuildStore(builder, start_val, loop_var_alloca);

  LLVMTypeRef llvm_type = LLVMInt32Type();

  JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, &t_int, start_val, llvm_type);
  sym->storage = loop_var_alloca;

  const char *chars = binding->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, binding->data.AST_IDENTIFIER.length);

  ht_set_hash(ctx->frame->table, chars, id_hash, sym);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);

  LLVMValueRef current_val =
      LLVMBuildLoad2(builder, LLVMInt32Type(), loop_var_alloca, "current");

  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntULT, current_val, end_val, "loopcond");

  LLVMBuildCondBr(builder, cond, body_block, after_block);

  LLVMPositionBuilderAtEnd(builder, body_block);
  codegen(body, ctx, module, builder);

  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);

  LLVMValueRef inc_val = LLVMBuildAdd(
      builder, current_val, LLVMConstInt(LLVMInt32Type(), 1, 0), "inc");
  LLVMBuildStore(builder, inc_val, loop_var_alloca);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef codegen_loop_iter_array(Ast *binding, Ast *iter_array_expr,
                                     Ast *body, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  const char *loop_var_name = binding->data.AST_IDENTIFIER.value;

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef cond_block =
      LLVMAppendBasicBlock(current_function, "loop.cond");

  LLVMBasicBlockRef body_block =
      LLVMAppendBasicBlock(current_function, "loop.body");
  LLVMBasicBlockRef inc_block =
      LLVMAppendBasicBlock(current_function, "loop.inc");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlock(current_function, "loop.after");

  Type *arr_type = iter_array_expr->data.AST_APPLICATION.args->md;
  Type *_type = arr_type->data.T_CONS.args[0];

  LLVMTypeRef loop_var_type = type_to_llvm_type(_type, ctx, module);

  LLVMValueRef array_val =
      codegen(iter_array_expr->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef array_size =
      codegen_get_array_size(builder, array_val, loop_var_type);

  codegen(iter_array_expr->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef loop_var_alloca =
      LLVMBuildAlloca(builder, loop_var_type, loop_var_name);

  LLVMValueRef start_idx = LLVMConstInt(LLVMInt32Type(), 0, 0);

  LLVMValueRef loop_iter_alloca =
      LLVMBuildAlloca(builder, LLVMInt32Type(), "iterator");

  LLVMBuildStore(builder, start_idx, loop_iter_alloca);

  LLVMValueRef start_val =
      get_array_element(builder, array_val, start_idx, loop_var_type);
  LLVMBuildStore(builder, start_val, loop_var_alloca);

  LLVMValueRef end_val =
      array_size; // loop is equivalent to for (int i = 0; i < size; i++) {...

  // TODO: need to use codegen_pattern_binding to properly set up complex
  // bindings like (i, x)
  JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, _type, start_val, loop_var_type);
  sym->storage = loop_var_alloca;

  const char *chars = binding->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, binding->data.AST_IDENTIFIER.length);
  ht_set_hash(ctx->frame->table, chars, id_hash, sym);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);

  LLVMValueRef current_val =
      LLVMBuildLoad2(builder, LLVMInt32Type(), loop_var_alloca, "current");

  LLVMValueRef current_idx_val =
      LLVMBuildLoad2(builder, LLVMInt32Type(), loop_iter_alloca, "current");

  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntULT, current_idx_val, end_val, "loopcond");

  LLVMBuildCondBr(builder, cond, body_block, after_block);
  LLVMPositionBuilderAtEnd(builder, body_block);

  codegen(body, ctx, module, builder);

  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);

  LLVMValueRef inc_val = LLVMBuildAdd(
      builder, current_idx_val, LLVMConstInt(LLVMInt32Type(), 1, 0), "inc");

  LLVMBuildStore(builder, inc_val, loop_iter_alloca);
  LLVMValueRef next_arr_el =
      get_array_element(builder, array_val, inc_val, loop_var_type);
  LLVMBuildStore(builder, next_arr_el, loop_var_alloca);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef codegen_loop_iter_list(Ast *binding, Ast *iter_expr, Ast *body,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  const char *loop_var_name = binding->data.AST_IDENTIFIER.value;

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef cond_block =
      LLVMAppendBasicBlock(current_function, "loop.cond");
  LLVMBasicBlockRef body_block =
      LLVMAppendBasicBlock(current_function, "loop.body");
  LLVMBasicBlockRef inc_block =
      LLVMAppendBasicBlock(current_function, "loop.inc");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlock(current_function, "loop.after");

  Type *ltype = iter_expr->data.AST_APPLICATION.args->md;

  LLVMValueRef list_val =
      codegen(iter_expr->data.AST_APPLICATION.args, ctx, module, builder);

  Type *list_el_type = ltype->data.T_CONS.args[0];
  LLVMTypeRef llvm_list_el_type = type_to_llvm_type(list_el_type, ctx, module);
  LLVMTypeRef llvm_list_node_type = llnode_type(llvm_list_el_type);

  LLVMValueRef loop_var_alloca =
      LLVMBuildAlloca(builder, llvm_list_el_type, loop_var_name);

  LLVMValueRef iterator = LLVMBuildAlloca(
      builder, LLVMPointerType(llvm_list_node_type, 0), "iterator");

  LLVMBuildStore(builder, list_val, iterator);

  LLVMValueRef is_null = LLVMBuildIsNull(builder, list_val, "is_null_check");

  LLVMBasicBlockRef init_var_block =
      LLVMAppendBasicBlock(current_function, "init_var");

  LLVMBuildCondBr(builder, is_null, after_block, init_var_block);

  LLVMPositionBuilderAtEnd(builder, init_var_block);

  LLVMValueRef first_node = LLVMBuildLoad2(
      builder, LLVMPointerType(llvm_list_node_type, 0), iterator, "first_node");

  LLVMValueRef first_data_ptr = LLVMBuildStructGEP2(
      builder, llvm_list_node_type, first_node, 0, "first_data_ptr");
  LLVMValueRef first_data =
      LLVMBuildLoad2(builder, llvm_list_el_type, first_data_ptr, "first_data");

  // TODO: need to use codegen_pattern_binding to properly set up bindings like
  // (i, x)
  JITSymbol *sym =
      new_symbol(STYPE_LOCAL_VAR, list_el_type, first_data, llvm_list_el_type);
  sym->storage = loop_var_alloca;

  LLVMBuildStore(builder, first_data, loop_var_alloca);
  const char *chars = binding->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, binding->data.AST_IDENTIFIER.length);
  ht_set_hash(ctx->frame->table, chars, id_hash, sym);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);

  LLVMValueRef current_node =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0), iterator,
                     "current_node");
  LLVMValueRef cond = LLVMBuildIsNotNull(builder, current_node, "loop_cond");

  LLVMBuildCondBr(builder, cond, body_block, after_block);

  LLVMPositionBuilderAtEnd(builder, body_block);

  codegen(body, ctx, module, builder);

  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);

  LLVMValueRef next_ptr = LLVMBuildStructGEP2(builder, llvm_list_node_type,
                                              current_node, 1, "next_ptr");
  LLVMValueRef next_node = LLVMBuildLoad2(
      builder, LLVMPointerType(llvm_list_node_type, 0), next_ptr, "next_node");

  LLVMBuildStore(builder, next_node, iterator);

  LLVMValueRef is_next_null =
      LLVMBuildIsNull(builder, next_node, "is_next_null");

  LLVMBasicBlockRef update_var_block =
      LLVMAppendBasicBlock(current_function, "update_var");
  LLVMBasicBlockRef continue_block =
      LLVMAppendBasicBlock(current_function, "continue");

  LLVMBuildCondBr(builder, is_next_null, continue_block, update_var_block);

  LLVMPositionBuilderAtEnd(builder, update_var_block);

  LLVMValueRef next_data_ptr = LLVMBuildStructGEP2(
      builder, llvm_list_node_type, next_node, 0, "next_data_ptr");
  LLVMValueRef next_data =
      LLVMBuildLoad2(builder, llvm_list_el_type, next_data_ptr, "next_data");
  LLVMBuildStore(builder, next_data, loop_var_alloca);

  LLVMBuildBr(builder, continue_block);

  LLVMPositionBuilderAtEnd(builder, continue_block);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  return NULL; // Return void for loops
}

LLVMValueRef codegen_loop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Ast *body = ast->data.AST_LET.in_expr; // Loop body
  Ast *binding = ast->data.AST_LET.binding;
  Ast *iter_expr = ast->data.AST_LET.expr;

  STACK_ALLOC_CTX_PUSH(loop_ctx, ctx);
  if (iter_expr->tag == AST_RANGE_EXPRESSION) {
    return codegen_loop_range(binding, iter_expr, body, &loop_ctx, module,
                              builder);
  }

  if (is_loop_of_iterable(ast)) {
    Type *iterable_type =
        ast->data.AST_LET.expr->data.AST_APPLICATION.function->md;

    if (is_coroutine_constructor_type(iterable_type)) {
      iterable_type = iterable_type->data.T_FN.from;

      if (is_array_type(iterable_type)) {
        return codegen_loop_iter_array(binding, iter_expr, body, &loop_ctx,
                                       module, builder);
      }

      if (is_list_type(iterable_type)) {
        return codegen_loop_iter_list(binding, iter_expr, body, &loop_ctx,
                                      module, builder);
      }
    }
  }

  fprintf(stderr, "Error: loop type not implemented\n");
  print_ast_err(ast);
  return NULL;
}
