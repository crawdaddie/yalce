
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
    Ast *last = body_tail(expr);
    return branch_is_match(last);
  }
  return false;
}

void set_as_tail(Ast *expr) {
  if (expr->tag == AST_BODY) {
    Ast *last = body_tail(expr);
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
  Type *_res_type = deep_copy_type(ast->md);
  _res_type = resolve_type_in_env_mut(_res_type, ctx->env);

  LLVMTypeRef res_type = type_to_llvm_type(_res_type, ctx, module);

  int len = ast->data.AST_MATCH.len;
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);

  bool branch_returns[len];
  bool any_branch_merges = false;

  for (int i = 0; i < len; i++) {
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

    if (is_return && (is_tail_call_expression(result_expr) ||
                      (branch_is_match(result_expr) &&
                       will_all_branches_return(result_expr)))) {
      branch_returns[i] = true;
    } else {
      any_branch_merges = true;
    }
  }

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

  for (int i = 0; i < len; i++) {
    Ast *test_expr = ast->data.AST_MATCH.branches + (2 * i);
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

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
    printf("match pattern %d\n", i);
    print_ast(test_expr);
    print_type(test_expr->md);

    LLVMValueRef test_value = codegen_pattern_binding(
        test_expr, test_val, test_val_type, &branch_ctx, module, builder);

    if (i == len - 1) {
      LLVMBuildBr(builder, branch_block);
    } else {
      LLVMBasicBlockRef false_target =
          next_block ? next_block : (any_branch_merges ? end_block : NULL);
      if (false_target) {
        LLVMBuildCondBr(builder, test_value, branch_block, false_target);
      } else {
        LLVMBuildCondBr(builder, test_value, branch_block, branch_block);
      }
    }

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

    if (branch_returns[i]) {
      if (is_tail_call_expression(result_expr)) {
        LLVMSetTailCall(branch_result, 1);
      }
      LLVMBuildRet(builder, branch_result);
    } else {
      if (((Type *)ast->md)->kind == T_VOID) {
        branch_result = LLVMGetUndef(LLVMVoidType());
      }

      if (end_block) {
        LLVMBuildBr(builder, end_block);
        LLVMBasicBlockRef current_branch_block = LLVMGetInsertBlock(builder);
        LLVMAddIncoming(phi, &branch_result, &current_branch_block, 1);
      }
    }

    if (next_block) {
      LLVMPositionBuilderAtEnd(builder, next_block);
    }

    destroy_ctx(&branch_ctx);
  }

  if (end_block) {
    LLVMPositionBuilderAtEnd(builder, end_block);
    return phi;
  } else {
    // Create a dummy return to satisfy LLVM
    LLVMBuildUnreachable(builder);
    return LLVMGetUndef(res_type);
  }
}

bool is_tail_call_expression(Ast *expr) {
  if (expr->tag == AST_BODY) {
    int n = expr->data.AST_BODY.len;
    Ast *last = expr->data.AST_BODY.tail->ast;
    return is_tail_call_expression(last);
  }

  return expr->tag == AST_APPLICATION;
}

bool will_all_branches_return(Ast *match_expr) {
  if (!branch_is_match(match_expr)) {
    return is_tail_call_expression(match_expr);
  }

  if (match_expr->tag == AST_BODY) {
    int n = match_expr->data.AST_BODY.len;
    // Ast *last = match_expr->data.AST_BODY.stmts[n - 1];
    Ast *last = match_expr->data.AST_BODY.tail->ast;
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
