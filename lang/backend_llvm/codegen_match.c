#include "backend_llvm/codegen_match.h"
#include "codegen_binop.h"
#include "serde.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_match_condition(LLVMValueRef expr_val, Ast *pattern,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (((Type *)pattern->md)->kind == T_INT) {
    return codegen_int_binop(builder, TOKEN_EQUALITY, expr_val,
                             codegen(pattern, ctx, module, builder));
  }

  if (((Type *)pattern->md)->kind == T_NUM) {
    return codegen_float_binop(builder, TOKEN_EQUALITY, expr_val,
                               codegen(pattern, ctx, module, builder));
  }
  return NULL;
}
#define AST_TRUE                                                               \
  &(Ast) {                                                                     \
    AST_BOOL, {                                                                \
      .AST_BOOL = { true }                                                     \
    }                                                                          \
  }

static bool is_default_case(Ast *test, int len, int i) {
  return ast_is_placeholder_id(test) && (i == (len - 1));
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef expr_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  // Create basic blocks
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef end_block =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "match.end");

  // Create a PHI node for the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMValueRef phi =
      LLVMBuildPhi(builder, LLVMTypeOf(expr_val), "match.result");

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

    JITLangCtx branch_ctx = {ctx->stack, ctx->stack_ptr + 1};
    // Check if this is the default case (represented by '_' in your example)
    if (is_default_case(test_expr, len, i)) {
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
