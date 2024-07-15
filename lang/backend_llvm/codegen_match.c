#include "backend_llvm/codegen_match.h"
#include "codegen_binop.h"
#include "codegen_list.h"
#include "codegen_symbols.h"
#include "codegen_tuple.h"
#include "codegen_types.h"
#include "types/type.h"
#include "types/util.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_match_condition(LLVMValueRef expr_val, Ast *pattern,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef match_values(Ast *left, LLVMValueRef right, LLVMValueRef *res,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  switch (left->tag) {
  case AST_IDENTIFIER: {
    if (*(left->data.AST_IDENTIFIER.value) == '_') {
      return *res;
    }

    LLVMTypeRef llvm_type = LLVMTypeOf(right);
    const char *id_chars = left->data.AST_IDENTIFIER.value;
    int id_len = left->data.AST_IDENTIFIER.length;

    JITSymbol *sym = malloc(sizeof(JITSymbol));

    if (ctx->stack_ptr == 0) {

      // top-level
      LLVMValueRef alloca_val =
          LLVMAddGlobalInAddressSpace(module, llvm_type, id_chars, 0);
      LLVMSetInitializer(alloca_val, right);

      *sym = (JITSymbol){STYPE_TOP_LEVEL_VAR, llvm_type, alloca_val};

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
      return *res;
    }

    *sym = (JITSymbol){STYPE_LOCAL_VAR, llvm_type, right};
    ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                hash_string(id_chars, id_len), sym);
    return *res;
  }
  case AST_BINOP: {
    return *res;
  }

  case AST_TUPLE: {
    return *res;
  }

  default:
    return *res;
  }
  return *res;
}

static LLVMValueRef codegen_match_tuple(LLVMValueRef expr_val, Ast *pattern,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  LLVMValueRef res = _TRUE;
  LLVMTypeRef tuple_type = LLVMTypeOf(expr_val);
  size_t len = pattern->data.AST_LIST.len;
  Ast *items = pattern->data.AST_LIST.items;
  for (size_t i = 0; i < len; i++) {
    Ast *tuple_item_ast = items + i;
    if (ast_is_placeholder_id(tuple_item_ast)) {
      continue;
    }

    LLVMValueRef tuple_member_val =
        codegen_tuple_access(i, expr_val, tuple_type, builder);

    if (!codegen_multiple_assignment(tuple_item_ast, tuple_member_val,
                                     tuple_item_ast->md, ctx, module, builder,
                                     false, 0)) {

      // assignment returns null - no assignment made so compare literal val
      LLVMValueRef match_cond = codegen_match_condition(
          tuple_member_val, tuple_item_ast, ctx, module, builder);

      res = and_vals(res, match_cond, builder);
    }
  }

  return res;
}

static LLVMValueRef codegen_match_list(LLVMValueRef list, Ast *pattern,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  LLVMValueRef res = _TRUE;
  LLVMTypeRef list_type = LLVMTypeOf(list);

  LLVMTypeRef list_el_type =
      type_to_llvm_type(((Type *)pattern->md)->data.T_CONS.args[0], ctx->env);

  if (pattern->tag == AST_BINOP &&
      pattern->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {

    res = LLVMBuildAnd(builder, res,
                       ll_is_not_null(list, list_el_type, builder), "");

    Ast *left = pattern->data.AST_BINOP.left;

    LLVMValueRef list_head_val = ll_get_head_val(list, list_el_type, builder);

    if (!codegen_multiple_assignment(left, list_head_val, left->md, ctx, module,
                                     builder, false, 0)) {

      // assignment returns null - no assignment made so compare literal val
      LLVMValueRef match_cond =
          codegen_match_condition(list_head_val, left, ctx, module, builder);
      res = and_vals(res, match_cond, builder);
    }
    Ast *right = pattern->data.AST_BINOP.right;
    LLVMValueRef list_next = ll_get_next(list, list_el_type, builder);

    if (!codegen_multiple_assignment(right, list_next, right->md, ctx, module,
                                     builder, false, 0)) {

      // assignment returns null - no assignment made so compare literal val
      LLVMValueRef match_cond =
          codegen_match_condition(list_next, right, ctx, module, builder);
      res = and_vals(res, match_cond, builder);
    }

    return res;
  }

  if (pattern->tag == AST_LIST) {
    size_t len = pattern->data.AST_LIST.len;

    if (len == 0) {
      return is_null_node(list, list_el_type, builder);
    }

    Ast *items = pattern->data.AST_LIST.items;
    LLVMValueRef list_head = list;
    for (size_t i = 0; i < len; i++) {
      Ast *list_item_ast = items + i;

      if (ast_is_placeholder_id(list_item_ast)) {
        continue;
      }

      LLVMValueRef list_member_val =
          ll_get_head_val(list_head, list_el_type, builder);

      if (!codegen_multiple_assignment(list_item_ast, list_member_val,
                                       list_item_ast->md, ctx, module, builder,
                                       false, 0)) {

        // assignment returns null - no assignment made so compare literal val
        LLVMValueRef match_cond = codegen_match_condition(
            list_member_val, list_item_ast, ctx, module, builder);
        res = and_vals(res, match_cond, builder);
      }

      list_head = ll_get_next(list_head, list_el_type, builder);
    }

    return res;
  }

  return _TRUE;
}

LLVMValueRef codegen_match_condition(LLVMValueRef expr_val, Ast *pattern,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  if (ast_is_placeholder_id(pattern)) {
    return _TRUE;
  }

  if (pattern->tag == AST_IDENTIFIER) {
    codegen_single_assignment(pattern, expr_val, pattern->md, ctx, module,
                              builder, false, 0);
    return _TRUE;
  }

  if (is_tuple_type(pattern->md)) {
    return codegen_match_tuple(expr_val, pattern, ctx, module, builder);
  }

  if (is_list_type(pattern->md)) {
    return codegen_match_list(expr_val, pattern, ctx, module, builder);
  }

  if (((Type *)pattern->md)->kind == T_INT) {
    return codegen_int_binop(builder, TOKEN_EQUALITY, expr_val,
                             codegen(pattern, ctx, module, builder));
  }

  if (((Type *)pattern->md)->kind == T_NUM) {
    return codegen_float_binop(builder, TOKEN_EQUALITY, expr_val,
                               codegen(pattern, ctx, module, builder));
  }
  return _FALSE;
}

static bool is_default_case(int len, int i) { return i == (len - 1); }

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  // printf("codegen match final type: ");
  // print_type(ast->md);
  // printf("\n");

  LLVMValueRef expr_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  // Create basic blocks
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef end_block =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "match.end");

  // Create a PHI node for the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, type_to_llvm_type(ast->md, ctx->env),
                                  "match.result");
  //
  // Return to the current block to start generating comparisons
  LLVMPositionBuilderAtEnd(builder, current_block);

  LLVMBasicBlockRef next_block = NULL;
  int len = ast->data.AST_MATCH.len;

  // Compile each branch
  for (int i = 0; i < len; i++) {
    Ast *test_expr = ast->data.AST_MATCH.branches + (2 * i);
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

    // Create a basic block for this branch
    char block_name[20];
    snprintf(block_name, sizeof(block_name), "match.branch%d", i);
    LLVMBasicBlockRef branch_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(current_block), block_name);

    if (i < len - 1) {
      next_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
                                        "match.next");
    } else {
      next_block = NULL; // Last iteration, no need for a next block
    }

    JITLangCtx branch_ctx = {ctx->stack, ctx->stack_ptr + 1, .env = ctx->env};

    // Check if this is the default case
    if (is_default_case(len, i)) {

      // Compile the test expression
      LLVMValueRef test_value = codegen_match_condition(
          expr_val, test_expr, &branch_ctx, module, builder);

      // If it's the default case, just jump to the branch block
      LLVMBuildBr(builder, branch_block);
    } else {

      // Compile the test expression
      LLVMValueRef test_value = codegen_match_condition(
          expr_val, test_expr, &branch_ctx, module, builder);

      // Create the conditional branch
      LLVMBuildCondBr(builder, test_value, branch_block,
                      next_block ? next_block : end_block);
    }

    // Compile the result expression in the branch block
    LLVMPositionBuilderAtEnd(builder, branch_block);

    LLVMValueRef branch_result =
        codegen(result_expr, &branch_ctx, module, builder);

    LLVMBuildBr(builder, end_block);
    LLVMAddIncoming(phi, &branch_result, &branch_block, 1);

    // Continue with the next comparison if there is one
    if (next_block) {
      LLVMPositionBuilderAtEnd(builder, next_block);
    }
  }

  // Position the builder at the end block and return the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  return phi;
}
