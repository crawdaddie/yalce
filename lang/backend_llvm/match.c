#include "backend_llvm/match.h"
#include "adt.h"
#include "backend_llvm/adt.h"
#include "backend_llvm/list.h"
#include "backend_llvm/tuple.h"
#include "backend_llvm/types.h"
#include "builtin_functions.h"
#include "parse.h"
#include "symbols.h"
#include "tuple.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

void set_as_tail(Ast *expr) {
  if (expr->tag == AST_BODY) {
    Ast *last = body_tail(expr);
    return set_as_tail(last);
  }
  expr->is_body_tail = true;
}
Ast *get_branch_tail(Ast *b) {
  if (b->tag == AST_BODY) {
    return body_tail(b);
  }
  return b;
}

void set_pattern_bindings(BindList *bl, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  // Iterate through the binding list and add each binding to the context
  for (BindList *b = bl; b != NULL; b = b->next) {
    if (ast_is_placeholder_id(b->binding)) {
      continue; // Skip placeholder bindings like '_'
    }

    const char *chars = b->binding->data.AST_IDENTIFIER.value;
    uint64_t id_hash =
        hash_string(chars, b->binding->data.AST_IDENTIFIER.length);

    // Local binding
    JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, b->type, b->val, b->val_type);
    ht_set_hash(ctx->frame->table, chars, id_hash, sym);
  }
}

void test_pattern_rec(Ast *pattern, BindList **bl, LLVMValueRef *test_result,
                      LLVMValueRef val, LLVMTypeRef val_type, Type *type,
                      JITLangCtx *ctx, LLVMModuleRef module,
                      LLVMBuilderRef builder) {

  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(pattern)) {
      return;
    }

    const char *chars = pattern->data.AST_IDENTIFIER.value;
    if (is_sum_type(type)) {
      for (int vidx = 0; vidx < type->data.T_CONS.num_args; vidx++) {
        Type *mem = type->data.T_CONS.args[vidx];

        if (strcmp(chars, mem->data.T_CONS.name) == 0) {
          if (mem->data.T_CONS.num_args == 0) {

            LLVMValueRef tag = extract_tag(val, builder);
            LLVMValueRef tag_test = LLVMBuildICmp(
                builder, LLVMIntEQ, tag, LLVMConstInt(LLVMInt8Type(), vidx, 0),
                "tag_match");

            *test_result = LLVMBuildAnd(builder, *test_result, tag_test, "");
            return;
          }
          break;
        }
      }
    }

    // Regular identifier - add to binding list
    BindList *new_binding = malloc(sizeof(BindList));
    *new_binding = (BindList){.val = val,
                              .val_type = val_type,
                              .type = type,
                              .binding = pattern,
                              .next = *bl};
    *bl = new_binding;
    return;
  }

  case AST_APPLICATION: {
    if (pattern->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
      return;
    }

    const char *cons_name =
        pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    // Handle list prepend (::)
    if (strcmp(cons_name, "::") == 0) {

      Type *list_el_type = type->data.T_CONS.args[0];
      LLVMTypeRef llvm_list_el_type =
          type_to_llvm_type(list_el_type, ctx, module);

      if (!llvm_list_el_type) {
        fprintf(stderr, "Error: list cons binding failed\n");
        return;
      }

      LLVMValueRef is_empty = ll_is_null(val, llvm_list_el_type, builder);
      LLVMValueRef is_not_empty =
          LLVMBuildNot(builder, is_empty, "list_not_empty");

      // AND with accumulated result
      *test_result = LLVMBuildAnd(builder, *test_result, is_not_empty, "");

      // Recursively test head and tail
      Ast *head_pattern = pattern->data.AST_APPLICATION.args;
      Ast *tail_pattern = pattern->data.AST_APPLICATION.args + 1;

      test_pattern_rec(head_pattern, bl, test_result,
                       ll_get_head_val(val, llvm_list_el_type, builder),
                       llvm_list_el_type, list_el_type, ctx, module, builder);

      test_pattern_rec(tail_pattern, bl, test_result,
                       ll_get_next(val, llvm_list_el_type, builder), val_type,
                       type, ctx, module, builder);
      return;
    }

    // Handle sum type variant with arguments
    Type *cons_type = pattern->type;
    if (cons_type->kind == T_CONS && is_sum_type(cons_type)) {

      // Find variant index
      int vidx;
      Type *variant_type;
      for (vidx = 0; vidx < cons_type->data.T_CONS.num_args; vidx++) {
        variant_type = cons_type->data.T_CONS.args[vidx];
        if (strcmp(cons_name, variant_type->data.T_CONS.name) == 0) {
          break;
        }
      }

      // Add tag test
      LLVMValueRef tag = extract_tag(val, builder);
      LLVMValueRef tag_test =
          LLVMBuildICmp(builder, LLVMIntEQ, tag,
                        LLVMConstInt(LLVMInt8Type(), vidx, 0), "tag_match");

      // AND with accumulated result
      *test_result = LLVMBuildAnd(builder, *test_result, tag_test, "");

      // Extract payload and test nested pattern
      LLVMValueRef payload = codegen_tuple_access(
          1, val, type_to_llvm_type(cons_type, ctx, module), builder);
      Type *payload_type = variant_type->data.T_CONS.args[0];
      LLVMTypeRef llvm_payload_type =
          type_to_llvm_type(payload_type, ctx, module);

      for (int i = 0; i < pattern->data.AST_APPLICATION.len; i++) {
        test_pattern_rec(pattern->data.AST_APPLICATION.args + i, bl,
                         test_result, payload, llvm_payload_type, payload_type,
                         ctx, module, builder);
      }
    }
    return;
  }

  case AST_TUPLE: {
    int len = pattern->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      LLVMValueRef elem_val = codegen_tuple_access(i, val, val_type, builder);
      Type *elem_type = type->data.T_CONS.args[i];
      LLVMTypeRef llvm_elem_type = type_to_llvm_type(elem_type, ctx, module);

      test_pattern_rec(pattern->data.AST_LIST.items + i, bl, test_result,
                       elem_val, llvm_elem_type, elem_type, ctx, module,
                       builder);
    }
    return;
  }

  case AST_LIST: {
    Type *list_el_type = type->data.T_CONS.args[0];
    LLVMTypeRef llvm_list_el_type =
        type_to_llvm_type(list_el_type, ctx, module);
    if (!llvm_list_el_type) {
      fprintf(stderr, "Error: list binding failed\n");
      return;
    }

    if (pattern->data.AST_LIST.len == 0) {
      // Empty list test
      LLVMValueRef is_empty = ll_is_null(val, llvm_list_el_type, builder);
      *test_result = LLVMBuildAnd(builder, *test_result, is_empty, "");
      return;
    }

    // Non-empty list - test each element
    LLVMValueRef current = val;
    for (int i = 0; i < pattern->data.AST_LIST.len; i++) {

      // Test that list has at least one more element
      LLVMValueRef is_empty = ll_is_null(current, llvm_list_el_type, builder);
      LLVMValueRef is_not_empty =
          LLVMBuildNot(builder, is_empty, "list_not_empty");

      *test_result = LLVMBuildAnd(builder, *test_result, is_not_empty, "");

      // Test head element
      LLVMValueRef head = ll_get_head_val(current, llvm_list_el_type, builder);
      test_pattern_rec(pattern->data.AST_LIST.items + i, bl, test_result, head,
                       llvm_list_el_type, list_el_type, ctx, module, builder);

      // Move to next element
      current = ll_get_next(current, llvm_list_el_type, builder);
    }
    return;
  }

  case AST_VOID: {
    // Void matches anything
    return;
  }

  default: {
    // Literal value - test for equality
    LLVMValueRef pattern_val = codegen(pattern, ctx, module, builder);
    LLVMValueRef equality_test =
        _codegen_equality(type, pattern_val, val, ctx, module, builder);

    *test_result = LLVMBuildAnd(builder, *test_result, equality_test, "");
    return;
  }
  }
}
// test val against a pattern collecting a series of bindings that should be
// made at the end
LLVMValueRef test_pattern(Ast *pattern, LLVMValueRef val, Type *val_type,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  BindList *bl = NULL;
  LLVMTypeRef llvm_val_type = type_to_llvm_type(val_type, ctx, module);

  // Start with true - AND all tests with this
  LLVMValueRef test_result = LLVMConstInt(LLVMInt1Type(), 1, 0);

  Ast *guard_expr = NULL;
  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    guard_expr = pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
    pattern = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
  }

  // Recursively collect tests and bindings, building up test_result
  test_pattern_rec(pattern, &bl, &test_result, val, llvm_val_type, val_type,
                   ctx, module, builder);

  // Apply bindings before evaluating guard (guard needs access to bound vars)
  set_pattern_bindings(bl, ctx, module, builder);

  if (guard_expr) {
    LLVMValueRef guard_result = codegen(guard_expr, ctx, module, builder);
    test_result = LLVMBuildAnd(builder, test_result, guard_result, "");
  }

  // Free allocated binding list
  while (bl != NULL) {
    BindList *next = bl->next;
    free(bl);
    bl = next;
  }

  return test_result;
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  if (!test_val) {
    fprintf(stderr, "could not compile test expression\n");
    return NULL;
  }

  Type *test_val_type = ast->data.AST_MATCH.expr->type;
  Type *result_type = ast->type;
  LLVMTypeRef llvm_result_type = type_to_llvm_type(result_type, ctx, module);

  int num_branches = ast->data.AST_MATCH.len;
  LLVMValueRef parent_func =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  bool is_tail_position = ast->is_body_tail;

  // Save current insertion point before creating merge block
  LLVMBasicBlockRef current_insert_block = LLVMGetInsertBlock(builder);

  // Create merge block only if not in tail position
  LLVMBasicBlockRef merge_block = NULL;
  LLVMValueRef result_phi = NULL;

  if (!is_tail_position) {
    merge_block = LLVMAppendBasicBlock(parent_func, "match.merge");
    LLVMPositionBuilderAtEnd(builder, merge_block);
    result_phi = LLVMBuildPhi(builder, llvm_result_type, "match.result");

    // Restore builder to where it was before creating merge block
    LLVMPositionBuilderAtEnd(builder, current_insert_block);
  }

  // Track whether we actually use the merge block
  int num_branches_to_merge = 0;

  // Generate code for each branch
  for (int i = 0; i < num_branches; i++) {
    Ast *pattern = ast->data.AST_MATCH.branches + (2 * i);
    Ast *branch_expr = ast->data.AST_MATCH.branches + (2 * i + 1);

    // Create basic blocks for this branch
    LLVMBasicBlockRef branch_body =
        LLVMAppendBasicBlock(parent_func, "match.case");
    LLVMBasicBlockRef next_test =
        (i < num_branches - 1) ? LLVMAppendBasicBlock(parent_func, "match.next")
                               : NULL;

    // Create new scope for pattern bindings
    STACK_ALLOC_CTX_PUSH(branch_ctx_mem, ctx)
    JITLangCtx branch_ctx = branch_ctx_mem;

    // Generate pattern matching test
    LLVMValueRef pattern_matches = test_pattern(
        pattern, test_val, test_val_type, &branch_ctx, module, builder);

    // Branch based on pattern match
    if (i == num_branches - 1) {
      // Last branch - always take it
      LLVMBuildBr(builder, branch_body);
    } else {
      // Conditional branch to next test or branch body
      LLVMBasicBlockRef false_dest = next_test ? next_test : merge_block;
      LLVMBuildCondBr(builder, pattern_matches, branch_body, false_dest);
    }

    // Generate branch body
    LLVMPositionBuilderAtEnd(builder, branch_body);

    if (is_tail_position) {
      set_as_tail(branch_expr);
    }

    LLVMValueRef branch_result =
        codegen(branch_expr, &branch_ctx, module, builder);

    if (!branch_result) {
      destroy_ctx(&branch_ctx);
      fprintf(stderr, "failed to compile match branch\n");
      return NULL;
    }

    // Handle branch termination
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);

    if (is_tail_position) {
      // In tail position - return directly if not already terminated
      if (!LLVMGetBasicBlockTerminator(current_block)) {
        if (LLVMIsACallInst(branch_result)) {
          LLVMSetTailCall(branch_result, true);
        }
        LLVMBuildRet(builder, branch_result);
      }
    } else {
      // Not in tail position - branch to merge block
      if (!LLVMGetBasicBlockTerminator(current_block)) {
        LLVMBuildBr(builder, merge_block);
        LLVMAddIncoming(result_phi, &branch_result, &current_block, 1);
        num_branches_to_merge++;
      }
    }

    destroy_ctx(&branch_ctx);

    // Position at next test block for next iteration
    if (next_test) {
      LLVMPositionBuilderAtEnd(builder, next_test);
    }
  }

  // Handle merge block
  if (!is_tail_position) {
    if (num_branches_to_merge == 0) {
      // No branches reached merge block - delete it
      LLVMDeleteBasicBlock(merge_block);
      // All branches must have returned - mark unreachable
      LLVMBasicBlockRef last_block = LLVMGetInsertBlock(builder);
      if (!LLVMGetBasicBlockTerminator(last_block)) {
        LLVMBuildUnreachable(builder);
      }
      return LLVMGetUndef(llvm_result_type);
    } else {
      // Some branches merged - return phi result
      LLVMPositionBuilderAtEnd(builder, merge_block);
      return result_phi;
    }
  } else {
    // In tail position - all branches returned
    // Make sure the current block is terminated
    LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
    if (current && !LLVMGetBasicBlockTerminator(current)) {
      LLVMBuildUnreachable(builder);
    }
    // Return undef - this value won't be used since all branches returned
    return LLVMGetUndef(llvm_result_type);
  }
}
