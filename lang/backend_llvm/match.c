#include "backend_llvm/match.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "binding.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <string.h>

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_simple_if_else(LLVMValueRef test_val, Ast *branches,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  LLVMValueRef phi = LLVM_IF_ELSE(builder, test_val,
                                  codegen(branches + 1, ctx, module, builder),
                                  codegen(branches + 3, ctx, module, builder));
  return phi;
}

static LLVMValueRef simple_option_match(LLVMValueRef test_val,
                                        Type *test_val_type, Type *res_val_type,
                                        Ast *branches, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  LLVMBasicBlockRef some_block = LLVMAppendBasicBlock(current_function, "some");
  LLVMBasicBlockRef none_block = LLVMAppendBasicBlock(current_function, "none");
  LLVMBasicBlockRef merge_block =
      LLVMAppendBasicBlock(current_function, "merge");

  LLVMValueRef is_some = codegen_option_is_some(test_val, builder);

  LLVMBuildCondBr(builder, is_some, some_block, none_block);

  LLVMPositionBuilderAtEnd(builder, some_block);
  LLVMValueRef some_result = ({
    LLVMValueRef some_val = LLVMBuildExtractValue(builder, test_val, 1, "");
    Ast *binding = branches->data.AST_APPLICATION.args;
    STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
    JITLangCtx branch_ctx = fn_ctx;
    codegen_pattern_binding(binding, some_val, type_of_option(test_val_type),
                            &branch_ctx, module, builder);

    LLVMValueRef branch_result =
        codegen(branches + 1, &branch_ctx, module, builder);

    if (res_val_type->kind == T_VOID) {
      branch_result = LLVMGetUndef(LLVMVoidType());
    }
    destroy_ctx(&branch_ctx);
    branch_result;
  });

  LLVMBuildBr(builder, merge_block);

  LLVMBasicBlockRef some_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, none_block);

  LLVMValueRef none_result = ({
    STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
    JITLangCtx branch_ctx = fn_ctx;

    LLVMValueRef branch_result =
        codegen(branches + 3, &branch_ctx, module, builder);

    if (res_val_type->kind == T_VOID) {
      branch_result = LLVMGetUndef(LLVMVoidType());
    }

    destroy_ctx(&branch_ctx);
    branch_result;
  });
  LLVMBuildBr(builder, merge_block);

  LLVMBasicBlockRef none_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);

  LLVMValueRef phi =
      LLVMBuildPhi(builder, LLVMTypeOf(some_result), "match.result");

  LLVMValueRef incoming_values[] = {some_result, none_result};
  LLVMBasicBlockRef incoming_blocks[] = {some_end_block, none_end_block};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

  return phi;
}

LLVMValueRef simple_binary_match(Ast *branches, LLVMValueRef val,
                                 Type *val_type, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  JITLangCtx then_ctx = *ctx;
  ht table;
  ht_init(&table);
  StackFrame sf = {.table = &table, .next = then_ctx.frame};
  then_ctx.frame = &sf;
  then_ctx.stack_ptr = ctx->stack_ptr + 1;
  JITLangCtx branch_ctx = then_ctx;

  LLVMValueRef condition = codegen_pattern_binding(
      branches, val, val_type, &branch_ctx, module, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

  LLVMBuildCondBr(builder, condition, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_result =
      codegen(branches + 1, &branch_ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, else_block);
  JITLangCtx else_ctx = *ctx;
  ht _table;
  ht_init(&_table);
  StackFrame _sf = {.table = &_table, .next = else_ctx.frame};
  else_ctx.frame = &_sf;
  else_ctx.stack_ptr = ctx->stack_ptr + 1;
  LLVMValueRef else_result = codegen(branches + 3, &else_ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "merge");
  LLVMValueRef incoming_values[] = {then_result, else_result};
  LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

  return phi;
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  Type *test_val_type = ast->data.AST_MATCH.expr->md;
  LLVMTypeRef res_type = type_to_llvm_type(ast->md, ctx->env, module);

  int len = ast->data.AST_MATCH.len;
  if (len == 2) {
    if (types_equal(test_val_type, &t_bool)) {
      (ast->data.AST_MATCH.branches + 1)->is_body_tail = ast->is_body_tail;
      (ast->data.AST_MATCH.branches + 3)->is_body_tail = ast->is_body_tail;
      return codegen_simple_if_else(test_val, ast->data.AST_MATCH.branches, ctx,
                                    module, builder);
    }

    if (test_val_type->alias && strcmp(test_val_type->alias, "Option") == 0) {
      return simple_option_match(test_val, test_val_type, ast->md,
                                 ast->data.AST_MATCH.branches, ctx, module,
                                 builder);
    }
    if ((ast->data.AST_MATCH.branches[0].tag != AST_MATCH_GUARD_CLAUSE) &&
        (ast->data.AST_MATCH.branches[2].tag != AST_MATCH_GUARD_CLAUSE)) {
      if (ast_is_placeholder_id(ast->data.AST_MATCH.branches + 2)) {
        return simple_binary_match(ast->data.AST_MATCH.branches, test_val,
                                   test_val_type, ctx, module, builder);
      }
    }
  }

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef end_block =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "match.end");

  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, res_type, "match.result");
  LLVMPositionBuilderAtEnd(builder, current_block);

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
      next_block = NULL; // Last iteration, no need for a next block
    }

    // JITLangCtx branch_ctx = ctx_push(*ctx);
    // {ctx->stack, ctx->stack_ptr + 1, .env = ctx->env};
    //
    STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
    JITLangCtx branch_ctx = fn_ctx;

    LLVMValueRef test_value = codegen_pattern_binding(
        test_expr, test_val, test_val_type, &branch_ctx, module, builder);

    if (i == len - 1) {
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

    if (((Type *)ast->md)->kind == T_VOID) {
      branch_result = LLVMGetUndef(LLVMVoidType());
    }

    LLVMBuildBr(builder, end_block);
    LLVMAddIncoming(phi, &branch_result, &branch_block, 1);

    // Continue with the next comparison if there is one
    if (next_block) {
      LLVMPositionBuilderAtEnd(builder, next_block);
    }

    destroy_ctx(&branch_ctx);
  }

  // Position the builder at the end block and return the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  return phi;
}
