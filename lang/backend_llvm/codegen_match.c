#include "backend_llvm/codegen_match.h"
#include "codegen_binop.h"
#include "codegen_list.h"
#include "codegen_symbols.h"
#include "codegen_tuple.h"
#include "codegen_types.h"
#include "serde.h"
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

LLVMValueRef codegen_equality(LLVMValueRef left, Type *left_type,
                              LLVMValueRef right, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  if (left_type->kind == T_INT) {
    return codegen_int_binop(builder, TOKEN_EQUALITY, left, right);
  }

  if (left_type->kind == T_NUM) {
    return codegen_float_binop(builder, TOKEN_EQUALITY, left, right);
  }
  return _FALSE;
}

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
    if (left->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
      Ast *pattern_left = left->data.AST_BINOP.left;
      Ast *pattern_right = left->data.AST_BINOP.right;
      LLVMTypeRef list_el_type = type_to_llvm_type(pattern_left->md, ctx->env);
      *res =
          and_vals(*res, ll_is_not_null(right, list_el_type, builder), builder);
      LLVMValueRef list_head_val =
          ll_get_head_val(right, list_el_type, builder);
      *res = and_vals(
          *res,
          match_values(pattern_left, list_head_val, res, ctx, module, builder),
          builder);
      LLVMValueRef list_next = ll_get_next(right, list_el_type, builder);
      *res = and_vals(
          *res,
          match_values(pattern_right, list_next, res, ctx, module, builder),
          builder);
    }
    return *res;
  }
    // case AST_LIST: {
    //   Type *t = ((Type *)left->md)->data.T_CONS.args[0];
    //   LLVMTypeRef list_el_type = type_to_llvm_type(t, ctx->env);
    //
    //   if (left->data.AST_LIST.len == 0) {
    //     return and_vals(*res, ll_is_null(right, list_el_type, builder),
    //     builder);
    //   } else {
    //
    //     int len = left->data.AST_LIST.len;
    //     Ast *items = left->data.AST_LIST.items;
    //
    //     Ast *head_item = items;
    //     LLVMValueRef list = right;
    //     while (len--) {
    //       *res = and_vals(*res, ll_is_not_null(list, list_el_type, builder),
    //                       builder);
    //
    //       LLVMValueRef list_head_val =
    //           ll_get_head_val(list, list_el_type, builder);
    //
    //       *res = and_vals(
    //           *res,
    //           match_values(head_item, list_head_val, res, ctx, module,
    //           builder), builder);
    //
    //       list = ll_get_next(list, list_el_type, builder);
    //       head_item++;
    //     }
    //
    //     *res = and_vals(*res, ll_is_null(list, list_el_type, builder),
    //     builder);
    //
    //     return *res;
    //   }
    // }
    //
    //
    // case AST_LIST: {
    //   Type *t = ((Type *)left->md)->data.T_CONS.args[0];
    //   LLVMTypeRef list_el_type = type_to_llvm_type(t, ctx->env);
    //   if (left->data.AST_LIST.len == 0) {
    //     *res = ll_is_null(right, list_el_type, builder);
    //   } else {
    //     LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
    //     LLVMBasicBlockRef match_block =
    //         LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
    //         "match");
    //     LLVMBasicBlockRef mismatch_block = LLVMAppendBasicBlock(
    //         LLVMGetBasicBlockParent(current_block), "mismatch");
    //     LLVMBasicBlockRef next_block =
    //         LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
    //         "next");
    //
    //     LLVMValueRef list = right;
    //
    //     LLVMValueRef _true = _TRUE;
    //     for (int i = 0; i < left->data.AST_LIST.len; i++) {
    //       // Check if the list is not null
    //       LLVMValueRef is_not_null = ll_is_not_null(list, list_el_type,
    //       builder); LLVMBuildCondBr(builder, is_not_null, match_block,
    //       mismatch_block);
    //
    //       // Match block
    //       LLVMPositionBuilderAtEnd(builder, match_block);
    //       LLVMValueRef list_head_val =
    //           ll_get_head_val(list, list_el_type, builder);
    //       LLVMValueRef match_result =
    //           match_values(&left->data.AST_LIST.items[i], list_head_val,
    //           &_true,
    //                        ctx, module, builder);
    //       LLVMBuildCondBr(builder, match_result, next_block, mismatch_block);
    //
    //       // Next block
    //       LLVMPositionBuilderAtEnd(builder, next_block);
    //       list = ll_get_next(list, list_el_type, builder);
    //
    //       // Prepare for next iteration
    //       match_block = LLVMAppendBasicBlock(
    //           LLVMGetBasicBlockParent(current_block), "match");
    //       next_block = LLVMAppendBasicBlock(
    //           LLVMGetBasicBlockParent(current_block), "next");
    //     }
    //
    //     // All elements matched
    //     LLVMBuildBr(builder, match_block);
    //     LLVMPositionBuilderAtEnd(builder, match_block);
    //     *res = LLVMConstInt(LLVMInt1Type(), 1, false);
    //     LLVMBuildBr(builder, mismatch_block);
    //
    //     // Mismatch block (also serves as the final block)
    //     LLVMPositionBuilderAtEnd(builder, mismatch_block);
    //     LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "result");
    //     LLVMValueRef false_const = LLVMConstInt(LLVMInt1Type(), 0, false);
    //     LLVMAddIncoming(phi, &false_const, &current_block, 1);
    //     LLVMAddIncoming(phi, res, &match_block, 1);
    //     *res = phi;
    //   }
    //   return *res;
    // }
  case AST_LIST: {
    Type *t = ((Type *)left->md)->data.T_CONS.args[0];
    LLVMTypeRef list_el_type = type_to_llvm_type(t, ctx->env);
    if (left->data.AST_LIST.len == 0) {
      *res = ll_is_null(right, list_el_type, builder);
    } else {
      LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
      LLVMBasicBlockRef end_block =
          LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "end");
      LLVMBasicBlockRef match_block = NULL;
      LLVMBasicBlockRef next_block = NULL;
      LLVMValueRef list = right;
      LLVMValueRef and_result =
          LLVMConstInt(LLVMInt1Type(), 1, false); // Start with true

      for (int i = 0; i < left->data.AST_LIST.len; i++) {
        match_block = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(current_block), "match");
        next_block = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(current_block), "next");

        // Check if the list is not null
        LLVMValueRef is_not_null = ll_is_not_null(list, list_el_type, builder);
        LLVMBuildCondBr(builder, is_not_null, match_block, end_block);

        // Match block
        LLVMPositionBuilderAtEnd(builder, match_block);
        LLVMValueRef list_head_val =
            ll_get_head_val(list, list_el_type, builder);
        LLVMValueRef match_result =
            match_values(&left->data.AST_LIST.items[i], list_head_val,
                         &and_result, ctx, module, builder);

        // Update and_result
        and_result =
            LLVMBuildAnd(builder, and_result, match_result, "and_result");

        LLVMBuildCondBr(builder, match_result, next_block, end_block);

        // Next block
        LLVMPositionBuilderAtEnd(builder, next_block);
        list = ll_get_next(list, list_el_type, builder);
      }

      // After the loop, branch to the end block
      LLVMBuildBr(builder, end_block);

      // End block (formerly mismatch block)
      LLVMPositionBuilderAtEnd(builder, end_block);
      LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "result");

      // Add incoming values to the phi node
      LLVMValueRef false_const = LLVMConstInt(LLVMInt1Type(), 0, false);
      LLVMAddIncoming(phi, &false_const, &current_block, 1);
      if (next_block) {
        LLVMAddIncoming(phi, &and_result, &next_block, 1);
      }

      *res = phi;
    }
    return *res;
  }

    // case AST_LIST: {
    //   Type *t = ((Type *)left->md)->data.T_CONS.args[0];
    //   LLVMTypeRef list_el_type = type_to_llvm_type(t, ctx->env);
    //   if (left->data.AST_LIST.len == 0) {
    //     *res = ll_is_null(right, list_el_type, builder);
    //   } else {
    //     LLVMValueRef _true = _TRUE;
    //     int len = left->data.AST_LIST.len;
    //     Ast *items = left->data.AST_LIST.items;
    //     LLVMValueRef list = right;
    //
    //     LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
    //
    //     LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
    //         LLVMGetBasicBlockParent(current_block), "continue");
    //
    //     LLVMBasicBlockRef fail_block =
    //         LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
    //         "fail");
    //
    //     LLVMBasicBlockRef end_block =
    //         LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
    //         "end");
    //
    //     for (int i = 0; i < len; i++) {
    //       LLVMValueRef is_not_null = ll_is_not_null(list, list_el_type,
    //       builder); LLVMBuildCondBr(builder, is_not_null, continue_block,
    //       fail_block);
    //
    //       LLVMPositionBuilderAtEnd(builder, continue_block);
    //       LLVMValueRef list_head_val =
    //           ll_get_head_val(list, list_el_type, builder);
    //
    //       LLVMValueRef match_result = match_values(&items[i], list_head_val,
    //                                                &_true, ctx, module,
    //                                                builder);
    //       LLVMBuildCondBr(builder, match_result,
    //                       LLVMAppendBasicBlock(
    //                           LLVMGetBasicBlockParent(current_block),
    //                           "continue"),
    //                       fail_block);
    //
    //       list = ll_get_next(list, list_el_type, builder);
    //       continue_block = LLVMGetInsertBlock(builder);
    //     }
    //
    //     LLVMValueRef is_null = ll_is_null(list, list_el_type, builder);
    //     LLVMBuildCondBr(builder, is_null, end_block, fail_block);
    //
    //     LLVMPositionBuilderAtEnd(builder, fail_block);
    //     LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), *res);
    //     LLVMBuildBr(builder, end_block);
    //
    //     LLVMPositionBuilderAtEnd(builder, end_block);
    //   }
    //   return *res;
    // }

  case AST_TUPLE: {
    return *res;
  }

  default: {
    LLVMValueRef left_val = codegen(left, ctx, module, builder);

    *res = and_vals(
        *res, codegen_equality(left_val, left->md, right, ctx, module, builder),
        builder);
    return *res;
  }
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
      return ll_is_null(list, list_el_type, builder);
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
