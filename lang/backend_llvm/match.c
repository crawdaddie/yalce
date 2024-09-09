#include "backend_llvm/match.h"
#include "backend_llvm/binop.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/types.h"
#include "serde.h"
#include "symbols.h"
#include "variant.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef codegen_equality(LLVMValueRef left, Type *left_type,
                              LLVMValueRef right, Type *right_type,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  if (left_type->kind == T_INT) {
    Method method = left_type->implements[0]->methods[0];

    LLVMBinopMethod binop_method = method.method;
    return binop_method(left, right, module, builder);
  }

  int vidx;
  if (is_variant_type(right_type) &&
      variant_contains_type(right_type, left_type, &vidx)) {
    return match_variant_member(left, right, vidx, left_type, ctx, module,
                                builder);
  }

  // if (left_type->kind == T_CHAR) {
  //   return codegen_int_binop(builder, TOKEN_EQUALITY, left, right);
  // }
  //
  // if (left_type->kind == T_NUM) {
  //   return codegen_float_binop(builder, TOKEN_EQUALITY, left, right);
  // }
  return _FALSE;
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  Type *test_val_type = ast->data.AST_MATCH.expr->md;

  // Create basic blocks
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef end_block =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(current_block), "match.end");

  // Create a PHI node for the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMValueRef phi = LLVMBuildPhi(
      builder, type_to_llvm_type(ast->md, ctx->env, module), "match.result");
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
    LLVMValueRef test_value = match_values(test_expr, test_val, test_val_type,
                                           &branch_ctx, module, builder);

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

LLVMValueRef match_values(Ast *binding, LLVMValueRef val, Type *val_type,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {

    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    if (*(binding->data.AST_IDENTIFIER.value) == '_') {
      return _TRUE;
    }

    if (val_type->kind == T_FN && !(is_generic(val_type))) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym = new_symbol(STYPE_FUNCTION, val_type, val, llvm_type);

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

      return _TRUE;
    }

    if (ctx->stack_ptr == 0) {

      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym =
          new_symbol(STYPE_TOP_LEVEL_VAR, val_type, val, llvm_type);

      codegen_set_global(sym, val, val_type, llvm_type, ctx, module, builder);
      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
      return _TRUE;
    }
    LLVMValueRef simple_enum_member =
        codegen_simple_enum_member(binding, ctx, module);

    if (simple_enum_member) {
      return codegen_eq_int(simple_enum_member, val, module, builder);
    }

    JITSymbol *sym = lookup_id_ast(binding, ctx);

    if (!sym) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val, llvm_type);
      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
      return _TRUE;
    }
    // else {
    // Type *variant_member =
    //     env_lookup(ctx->env, binding->data.AST_IDENTIFIER.value);
    // int idx;
    // if (variant_member && is_variant_type(val_type) &&
    //     variant_contains_type(val_type, variant_member, &idx)) {
    //   return match_simple_variant_member(binding, idx, val_type, val, ctx,
    //                                      module, builder);
    // }
    // }
    break;
  }

  default: {

    LLVMValueRef test_val = codegen(binding, ctx, module, builder);

    Type *test_type = binding->md;
    LLVMValueRef match_test = codegen_equality(test_val, test_type, val,
                                               val_type, ctx, module, builder);

    return match_test;
  }
  }
}
