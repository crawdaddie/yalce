
#include "backend_llvm/match.h"
#include "backend_llvm/types.h"
#include "binding.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

bool is_tail_call_expression(Ast *expr);

bool will_all_branches_return(Ast *match_expr);

bool branch_is_match(Ast *expr) {
  if (expr->tag == AST_MATCH) {
    return true;
  }
  if (expr->tag == AST_BODY) {
    int n = expr->data.AST_BODY.len;
    Ast *last = expr->data.AST_BODY.stmts[n - 1];
    return branch_is_match(last);
  }
  return false;
}

void set_as_tail(Ast *expr) {
  if (expr->tag == AST_BODY) {
    int n = expr->data.AST_BODY.len;
    Ast *last = expr->data.AST_BODY.stmts[n - 1];
    return set_as_tail(last);
  }
  expr->is_body_tail = true;
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  bool is_return = ast->is_body_tail;

  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  if (!test_val) {

    fprintf(stderr, "could not compile test expression\n");
    return NULL;
  }

  Type *test_val_type = ast->data.AST_MATCH.expr->md;
  Type *_res_type = ast->md;
  // print_type_env(ctx->env);
  LLVMTypeRef res_type = type_to_llvm_type(_res_type, ctx, module);

  int len = ast->data.AST_MATCH.len;
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);

  // Track which branches actually need to merge
  bool branch_returns[len];
  bool any_branch_merges = false;

  // Pre-analyze branches to see if we need an end block at all
  for (int i = 0; i < len; i++) {
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

    // Check if this branch will definitely return (tail call or nested match
    // that returns)
    if (is_return && (is_tail_call_expression(result_expr) ||
                      (branch_is_match(result_expr) &&
                       will_all_branches_return(result_expr)))) {
      branch_returns[i] = true;
    } else {
      any_branch_merges = true;
    }
  }

  // Only create end block and phi if some branches actually merge
  LLVMBasicBlockRef end_block = NULL;
  LLVMValueRef phi = NULL;

  if (any_branch_merges) {
    end_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
                                     "match.end");
    LLVMPositionBuilderAtEnd(builder, end_block);
    phi = LLVMBuildPhi(builder, res_type, "match.result");
    LLVMPositionBuilderAtEnd(builder, current_block);
  }

  LLVMBasicBlockRef next_block = NULL;

  // Compile each branch
  for (int i = 0; i < len; i++) {
    Ast *test_expr = ast->data.AST_MATCH.branches + (2 * i);
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

    // Create a basic block for this branch
    LLVMBasicBlockRef branch_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(current_block), "match.branch");

    if (i < len - 1) {
      next_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block),
                                        "match.next");
    } else {
      next_block = NULL;
    }

    STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
    JITLangCtx branch_ctx = fn_ctx;

    LLVMValueRef test_value = codegen_pattern_binding(
        test_expr, test_val, test_val_type, &branch_ctx, module, builder);

    if (i == len - 1) {
      LLVMBuildBr(builder, branch_block);
    } else {
      // Conditional branch - if no end block exists, last branch goes nowhere
      LLVMBasicBlockRef false_target =
          next_block ? next_block : (any_branch_merges ? end_block : NULL);
      if (false_target) {
        LLVMBuildCondBr(builder, test_value, branch_block, false_target);
      } else {
        // All remaining branches return, so this is effectively an if-then
        LLVMBuildCondBr(builder, test_value, branch_block, branch_block);
      }
    }

    // Compile the result expression in the branch block
    LLVMPositionBuilderAtEnd(builder, branch_block);

    if (is_return) {
      set_as_tail(result_expr);
    }

    LLVMValueRef branch_result =
        codegen(result_expr, &branch_ctx, module, builder);

    if (!branch_result) {
      destroy_ctx(&branch_ctx);
      fprintf(stderr, "no branch result\n");
      return NULL;
    }

    // Handle the result based on whether this branch returns or merges
    if (branch_returns[i]) {
      // This branch should return directly
      if (is_tail_call_expression(result_expr)) {
        LLVMSetTailCall(branch_result, 1);
      }
      LLVMBuildRet(builder, branch_result);
      // Don't add to phi - this branch doesn't reach the end block
    } else {
      // This branch merges with others
      if (((Type *)ast->md)->kind == T_VOID) {
        branch_result = LLVMGetUndef(LLVMVoidType());
      }

      if (end_block) {
        LLVMBuildBr(builder, end_block);
        // Only add incoming if we actually branch to end_block
        LLVMBasicBlockRef current_branch_block = LLVMGetInsertBlock(builder);
        LLVMAddIncoming(phi, &branch_result, &current_branch_block, 1);
      }
    }

    // Continue with the next comparison if there is one
    if (next_block) {
      LLVMPositionBuilderAtEnd(builder, next_block);
    }

    destroy_ctx(&branch_ctx);
  }

  // Position the builder at the end block and return the result
  if (end_block) {
    LLVMPositionBuilderAtEnd(builder, end_block);
    return phi;
  } else {
    // All branches returned, this point is unreachable
    // Create a dummy return to satisfy LLVM
    LLVMBuildUnreachable(builder);
    return LLVMGetUndef(res_type);
  }
}

// Helper function to check if an expression is a direct tail call
bool is_tail_call_expression(Ast *expr) {
  if (expr->tag == AST_BODY) {
    int n = expr->data.AST_BODY.len;
    Ast *last = expr->data.AST_BODY.stmts[n - 1];
    return is_tail_call_expression(last);
  }

  return expr->tag == AST_APPLICATION;
}

// Helper function to check if all branches of a nested match will return
bool will_all_branches_return(Ast *match_expr) {
  if (!branch_is_match(match_expr)) {
    return is_tail_call_expression(match_expr);
  }

  if (match_expr->tag == AST_BODY) {
    int n = match_expr->data.AST_BODY.len;
    Ast *last = match_expr->data.AST_BODY.stmts[n - 1];
    return will_all_branches_return(last);
  }

  if (match_expr->tag == AST_MATCH) {
    int len = match_expr->data.AST_MATCH.len;
    for (int i = 0; i < len; i++) {
      Ast *branch_result = match_expr->data.AST_MATCH.branches + (2 * i + 1);
      if (!will_all_branches_return(branch_result)) {
        return false;
      }
    }
    return true;
  }

  return false;
}
