#include "backend_llvm/match.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/types.h"
#include "builtin_functions.h"
#include "function.h"
#include "list.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "variant.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  Type *test_val_type = ast->data.AST_MATCH.expr->md;

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef end_block =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "match.end");

  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMTypeRef res_type = type_to_llvm_type(ast->md, ctx->env, module);
  LLVMValueRef phi = LLVMBuildPhi(builder, res_type, "match.result");
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

    LLVMValueRef test_value = codegen_pattern_binding(
        test_expr, test_val, test_val_type, &branch_ctx, module, builder);

    if (i == len - 1) {
      // If it's the default case, just jump to the branch block
      // printf("If it's the default case, just jump to the branch block\n");
      // print_ast(test_expr);
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
    ht *branch_stack_frame = branch_ctx.stack + branch_ctx.stack_ptr;
    ht_reinit(branch_stack_frame);
  }

  // Position the builder at the end block and return the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  return phi;
}

LLVMValueRef codegen_pattern_binding(Ast *binding, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return _FALSE;
  }
  case AST_VOID: {
    return _TRUE;
  }
  default: {
    fprintf(stderr,
            "Error: pattern binding codegen - invalid binding pattern\n");
    print_ast(binding);
    return _FALSE;
  }
  }
}
