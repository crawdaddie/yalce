#include "backend_llvm/codegen_match.h"
#include "codegen_match_values.h"
#include "codegen_types.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
static bool is_default_case(int len, int i) { return i == (len - 1); }

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef expr_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  Type *expr_val_type = ast->data.AST_MATCH.expr->md;

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
    // Compile the test expression
    // LLVMValueRef test_value = codegen_match_condition(
    //     expr_val, test_expr, &branch_ctx, module, builder);
    LLVMValueRef _and = _TRUE;
    LLVMValueRef test_value = match_values(test_expr, expr_val, expr_val_type,
                                           &_and, &branch_ctx, module, builder);

    if (is_default_case(len, i)) {
      // If it's the default case, just jump to the branch block
      LLVMBuildBr(builder, branch_block);
    } else {
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
