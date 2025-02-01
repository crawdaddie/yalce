#include "backend_llvm/match.h"
#include "adt.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "builtin_functions.h"
#include "list.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <stdlib.h>
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
  if (len == 2) {
    if (types_equal(test_val_type, &t_bool)) {
      return codegen_simple_if_else(test_val, ast->data.AST_MATCH.branches, ctx,
                                    module, builder);
    }
  }

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

    destroy_ctx(&branch_ctx);

    // ht_destroy(branch_ctx.frame->table);
    // free(branch_ctx.frame);
  }

  // Position the builder at the end block and return the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  return phi;
}

LLVMValueRef chained_ands() {}

LLVMValueRef codegen_pattern_binding(Ast *binding, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(binding)) {
      return _TRUE;
    }

    const char *chars = binding->data.AST_IDENTIFIER.value;
    uint64_t id_hash = hash_string(chars, binding->data.AST_IDENTIFIER.length);

    Type *enum_type = env_lookup(ctx->env, chars);

    if (enum_type != NULL && is_simple_enum(enum_type)) {
      int vidx;
      Type *mem;
      for (vidx = 0; vidx < enum_type->data.T_CONS.num_args; vidx++) {
        mem = enum_type->data.T_CONS.args[vidx];
        if (strcmp(chars, mem->data.T_CONS.name) == 0) {
          break;
        }
      }

      return LLVMBuildICmp(builder, LLVMIntEQ, val,
                           LLVMConstInt(LLVMInt8Type(), vidx, 0), "enum val");
    }

    if (ctx->stack_ptr == 0) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym =
          new_symbol(STYPE_TOP_LEVEL_VAR, val_type, val, llvm_type);
      codegen_set_global(sym, val, val_type, llvm_type, ctx, module, builder);
      ht_set_hash(ctx->frame->table, chars, id_hash, sym);
      return _TRUE;
    } else {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val, llvm_type);
      ht_set_hash(ctx->frame->table, chars, id_hash, sym);
      return _TRUE;
    }

    return _FALSE;
  }
  case AST_APPLICATION: {
    if (binding->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
      return NULL;
    }

    if (strcmp(
            binding->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
            "::") == 0) {

      Ast *head_expr = binding->data.AST_APPLICATION.args;
      Ast *tail_expr = binding->data.AST_APPLICATION.args + 1;

      Type *list_el_type = val_type->data.T_CONS.args[0];

      LLVMTypeRef llvm_list_el_type =
          type_to_llvm_type(list_el_type, ctx->env, module);

      LLVMValueRef res = LLVM_IF_ELSE(
          builder, ll_is_not_null(val, llvm_list_el_type, builder),

          LLVM_IF_ELSE(
              builder,
              (codegen_pattern_binding(
                  head_expr, ll_get_head_val(val, llvm_list_el_type, builder),
                  list_el_type, ctx, module, builder)),
              codegen_pattern_binding(
                  tail_expr, ll_get_next(val, llvm_list_el_type, builder),
                  val_type, ctx, module, builder),
              _FALSE),
          _FALSE);

      return res;
    }

    const char *chars =
        binding->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    Type *cons_type = binding->md;
    if (cons_type->kind == T_CONS) {
      if (is_variant_type(cons_type)) {
        int vidx;
        Type *mem;
        for (vidx = 0; vidx < cons_type->data.T_CONS.num_args; vidx++) {
          mem = cons_type->data.T_CONS.args[vidx];
          if (strcmp(chars, mem->data.T_CONS.name) == 0) {
            break;
          }
        }

        // Extract the tag (first field - Int8)
        LLVMValueRef tag = extract_tag(val, builder);

        // Compare tag with variant index
        LLVMValueRef tag_match =
            LLVMBuildICmp(builder, LLVMIntEQ, tag,
                          LLVMConstInt(LLVMInt8Type(), vidx, 0), "tag_match");

        LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
        LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
        LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
        LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
        LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

        LLVMBuildCondBr(builder, tag_match, then_block, else_block);

        LLVMPositionBuilderAtEnd(builder, then_block);
        LLVMValueRef next_val = codegen_tuple_access(
            1, val, type_to_llvm_type(cons_type, ctx->env, module), builder);
        LLVMValueRef next_result = codegen_pattern_binding(
            binding->data.AST_APPLICATION.args, next_val,
            cons_type->data.T_CONS.args[vidx]->data.T_CONS.args[0], ctx, module,
            builder);
        LLVMBuildBr(builder, merge_block);
        LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

        LLVMPositionBuilderAtEnd(builder, else_block);
        LLVMBuildBr(builder, merge_block);
        LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

        LLVMPositionBuilderAtEnd(builder, merge_block);
        LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "result");
        LLVMValueRef false_const = LLVMConstInt(LLVMInt1Type(), 0, 0);

        LLVMValueRef incoming_values[] = {next_result, false_const};
        LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
        LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

        return phi;
      } else {
      }
    }

    return _FALSE;
  }

  case AST_VOID: {
    return _TRUE;
  }

  case AST_LIST: {
    if (binding->data.AST_LIST.len == 0) {
      Type *list_el_type = val_type->data.T_CONS.args[0];
      LLVMTypeRef llvm_list_el_type =
          type_to_llvm_type(list_el_type, ctx->env, module);

      return ll_is_null(val, llvm_list_el_type, builder);
    }
  }
  case AST_MATCH_GUARD_CLAUSE: {
    LLVMValueRef test_val =
        codegen_pattern_binding(binding->data.AST_MATCH_GUARD_CLAUSE.test_expr,
                                val, val_type, ctx, module, builder);

    LLVMValueRef guard_val = codegen(
        binding->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx, module, builder);

    return LLVMBuildAnd(builder, test_val, guard_val, "guard_and_test");
  }
  default: {
    // test equality of real values
    return _codegen_equality(binding->md,
                             codegen(binding, ctx, module, builder), val, ctx,
                             module, builder);
  }
  }
}
