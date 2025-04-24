#include "backend_llvm/match.h"
#include "adt.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "builtin_functions.h"
#include "coroutines.h"
#include "list.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types/inference.h"
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

    // ht_destroy(branch_ctx.frame->table);
    // free(branch_ctx.frame);
  }

  // Position the builder at the end block and return the result
  LLVMPositionBuilderAtEnd(builder, end_block);
  return phi;
}

LLVMValueRef chained_ands() {}
LLVMValueRef match_list_prepend(Ast *binding, LLVMValueRef list,
                                Type *list_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *head_expr = binding->data.AST_APPLICATION.args;
  Ast *tail_expr = binding->data.AST_APPLICATION.args + 1;

  Type *list_el_type = list_type->data.T_CONS.args[0];

  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);
  LLVMValueRef list_empty = ll_is_null(list, llvm_list_el_type, builder);

  // Create blocks for the branch
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef match_block = LLVMAppendBasicBlock(function, "match.list");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge.list");

  // Branch based on list_empty
  LLVMBuildCondBr(builder, list_empty, merge_block, match_block);

  // Build the match block for non-empty list
  LLVMPositionBuilderAtEnd(builder, match_block);

  // Match head and tail
  LLVMValueRef head_match = codegen_pattern_binding(
      head_expr, ll_get_head_val(list, llvm_list_el_type, builder),
      list_el_type, ctx, module, builder);

  LLVMValueRef tail_match = codegen_pattern_binding(
      tail_expr, ll_get_next(list, llvm_list_el_type, builder), list_type, ctx,
      module, builder);

  // Combine the results
  LLVMValueRef match_result =
      LLVMBuildAnd(builder, head_match, tail_match, "match.result");
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef match_end_block = LLVMGetInsertBlock(builder);

  // Build merge block with phi node
  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "result");

  // Set up phi node inputs
  LLVMValueRef false_const = LLVMConstInt(LLVMInt1Type(), 0, 0);
  LLVMValueRef incoming_values[] = {match_result, false_const};
  LLVMBasicBlockRef incoming_blocks[] = {match_end_block, current_block};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

  return phi;
}

LLVMValueRef codegen_pattern_binding(Ast *binding, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(binding)) {
      return _TRUE;
    }

    int inner_state_slot = get_inner_state_slot(binding);
    if (inner_state_slot >= 0) {

      JITSymbol *sym = lookup_id_ast(binding, ctx);
      LLVMValueRef storage = sym->storage;
      LLVMBuildStore(builder, val, storage);
      sym->val = val;
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

      JITSymbol *ex_sym = ht_get_hash(ctx->frame->table, chars, id_hash);

      JITSymbol *sym;
      if (ex_sym != NULL) {
        // printf("restore existing symbol\n");
        // print_ast(binding);
        ex_sym->val = val;
        ex_sym->llvm_type = llvm_type;
        ex_sym->symbol_type = val_type;
        LLVMBuildStore(builder, val, ex_sym->storage);
        sym = ex_sym;
      } else {
        sym = new_symbol(STYPE_TOP_LEVEL_VAR, val_type, val, llvm_type);
        codegen_set_global(chars, sym, val, val_type, llvm_type, ctx, module,
                           builder);
      }

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
      return match_list_prepend(binding, val, val_type, ctx, module, builder);
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

  case AST_TUPLE: {
    int len = binding->data.AST_LIST.len;
    LLVMValueRef success = _TRUE;
    LLVMTypeRef llvm_tuple_type = type_to_llvm_type(val_type, ctx->env, module);
    for (int i = 0; i < len; i++) {
      success = LLVMBuildAnd(
          builder,
          codegen_pattern_binding(
              binding->data.AST_LIST.items + i,
              codegen_tuple_access(i, val, llvm_tuple_type, builder),
              val_type->data.T_CONS.args[i], ctx, module, builder),
          success, "");
    }
    return success;
  }

  case AST_LIST: {
    Type *list_el_type = val_type->data.T_CONS.args[0];
    LLVMTypeRef llvm_list_el_type =
        type_to_llvm_type(list_el_type, ctx->env, module);

    if (binding->data.AST_LIST.len == 0) {
      return ll_is_null(val, llvm_list_el_type, builder);
    }

    LLVMValueRef l = val;
    LLVMValueRef res = _TRUE;
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      LLVMValueRef head = ll_get_head_val(l, llvm_list_el_type, builder);
      res = LLVMBuildAnd(
          builder, res,
          codegen_pattern_binding(b, head, list_el_type, ctx, module, builder),
          "");
      l = ll_get_next(l, llvm_list_el_type, builder);
    }
    return res;
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
