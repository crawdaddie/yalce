#include "adt.h"
#include "binding.h"
#include "builtin_functions.h"
#include "common.h"
#include "function.h"
#include "list.h"
#include "parse.h"
#include "symbols.h"
#include "types.h"
#include "types/type_ser.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef bind_value(Ast *id, LLVMValueRef val, Type *val_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
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

int get_constructor_index(Ast *pattern) {

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    pattern = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
  }

  if (pattern->tag == AST_IDENTIFIER) {

    int idx;
    extract_member_from_sum_type_idx(pattern->type, pattern, &idx);
    return idx;
  }

  if (pattern->tag == AST_APPLICATION &&
      pattern->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    int idx;
    extract_member_from_sum_type_idx(
        pattern->type, pattern->data.AST_APPLICATION.function, &idx);
    return idx;
  }

  if (pattern->tag == AST_APPLICATION &&
      pattern->data.AST_APPLICATION.function->tag == AST_RECORD_ACCESS) {

    Ast *id = pattern->data.AST_APPLICATION.function;
    while (id->tag == AST_RECORD_ACCESS) {
      id = id->data.AST_RECORD_ACCESS.member;
    }
    Ast p = *id;
    p.type = pattern->type;
    return get_constructor_index(&p);
  }

  if (pattern->tag == AST_RECORD_ACCESS) {

    Ast *id = pattern;
    while (id->tag == AST_RECORD_ACCESS) {
      id = id->data.AST_RECORD_ACCESS.member;
    }
    Ast p = *id;
    p.type = pattern->type;
    return get_constructor_index(&p);
  }
  return -1;
}

// if we have an expression like this
// match x with
// | EnumA 1 -> ...
// | EnumA 2 -> ...
// | EnumA 3 -> ...
// | EnumB _ -> ...
//
// and EnumA 1 doesn't match due to the tag A then it would be good to branch
// straight to EnumB rather than retesting EnumA 2, EnumA 3 since we already
// know the tag won't match
// this function rearranges the branches if they're out of order to make that a
// bit easier, so all the EnumAs etc are contiguous
//
// Rearranges match branches so patterns with the same constructor are
// contiguous. Wildcard patterns (catch-all '_') are moved to the end.
// patterns_and_bodies: array of 2*n nodes where patterns are at 2i,
// bodies at 2i+1 n: number of branches result: fixed-size array of 2*n nodes to
// hold rearranged pairs
void stable_partition_match_over_sum_type(Ast *patterns_and_bodies, size_t n,
                                          Ast *result) {
  if (n == 0)
    return;

#define MAX_CONSTRUCTORS 64
  typedef struct {
    int constructor_index;
    size_t count;
    size_t write_index;
  } ConstructorInfo;

  ConstructorInfo infos[MAX_CONSTRUCTORS] = {0};
  int constructor_indices[MAX_CONSTRUCTORS]; // Track which constructor indices
                                             // we've seen
  size_t num_constructors = 0;
  size_t num_wildcards = 0;

  // First pass: identify unique constructors and count them
  // Also count wildcards separately
  for (size_t i = 0; i < n; i++) {
    Ast pattern = patterns_and_bodies[2 * i];

    // Check if this is a wildcard pattern
    Ast *p = &pattern;
    if (p->tag == AST_MATCH_GUARD_CLAUSE) {
      p = p->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    bool is_wildcard = (p->tag == AST_IDENTIFIER && ast_is_placeholder_id(p));

    if (is_wildcard) {
      num_wildcards++;
      continue;
    }

    int tag = get_constructor_index(&pattern);

    if (tag < 0) {
      fprintf(stderr, "Error: match constructor rearranging failed %d\n", tag);
      print_ast_err(&pattern);
      return;
    }

    // Find or create constructor info
    size_t idx = 0;
    for (; idx < num_constructors; idx++) {
      if (constructor_indices[idx] == tag) {
        infos[idx].count++;
        break;
      }
    }

    if (idx == num_constructors) {
      // New constructor
      constructor_indices[num_constructors] = tag;
      infos[num_constructors].constructor_index = tag;
      infos[num_constructors].count = 1;
      num_constructors++;
    }
  }

  // Calculate starting positions for each constructor group
  // Each group starts at offset * 2 (since we store pairs)
  size_t offset = 0;
  for (size_t i = 0; i < num_constructors; i++) {
    infos[i].write_index = offset;
    offset += infos[i].count;
  }

  // Wildcards go at the end
  size_t wildcard_write_pos = offset;

  // Second pass: distribute pattern-body pairs to their groups
  // Wildcards are written to the end
  for (size_t i = 0; i < n; i++) {
    Ast *pattern = &patterns_and_bodies[2 * i];
    Ast *body = &patterns_and_bodies[2 * i + 1];

    // Check if this is a wildcard pattern
    Ast *p = pattern;
    if (p->tag == AST_MATCH_GUARD_CLAUSE) {
      p = p->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    bool is_wildcard = (p->tag == AST_IDENTIFIER && ast_is_placeholder_id(p));

    if (is_wildcard) {
      // Write wildcard to the end
      *(result + (2 * wildcard_write_pos)) = *pattern;
      *(result + (2 * wildcard_write_pos + 1)) = *body;
      wildcard_write_pos++;
      continue;
    }

    int tag = get_constructor_index(pattern);

    // Find constructor info and write both pattern and body
    for (size_t j = 0; j < num_constructors; j++) {
      if (infos[j].constructor_index == tag) {
        size_t write_pos = infos[j].write_index;
        *(result + (2 * write_pos)) = *pattern;
        *(result + (2 * write_pos + 1)) = *body;
        infos[j].write_index++;
        break;
      }
    }
  }
}

LLVMValueRef test_guard_clause(LLVMValueRef bool_acc, Ast *expr,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef v = codegen(expr, ctx, module, builder);
  return LLVMBuildAnd(builder, bool_acc, v, "and_with_guard");
}

LLVMValueRef test_pattern(Ast *pattern,

                          LLVMValueRef val, Type *val_type,

                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMValueRef test_list_cons_pattern(Ast *pattern, LLVMValueRef val,
                                    Type *val_type, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  Type *list_el_type = val_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_list_el_type = type_to_llvm_type(list_el_type, ctx, module);

  LLVMValueRef is_empty = ll_is_null(val, llvm_list_el_type, builder);

  LLVMValueRef parent_func =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  LLVMBasicBlockRef test_elements_block =
      LLVMAppendBasicBlock(parent_func, "list_cons_test_elements");

  LLVMBasicBlockRef merge_block =
      LLVMAppendBasicBlock(parent_func, "list_cons_merge");

  LLVMBasicBlockRef pre_branch_block = LLVMGetInsertBlock(builder);

  LLVMBuildCondBr(builder, is_empty, merge_block, test_elements_block);

  LLVMPositionBuilderAtEnd(builder, test_elements_block);

  LLVMValueRef elements_test_result = _TRUE;

  Ast *head_pattern = pattern->data.AST_APPLICATION.args;
  LLVMValueRef tv = test_pattern(
      head_pattern, ll_get_head_val(val, llvm_list_el_type, builder),
      list_el_type, ctx, module, builder);

  Ast *tail_pattern = pattern->data.AST_APPLICATION.args + 1;

  LLVMValueRef tpv =
      test_pattern(tail_pattern, ll_get_next(val, llvm_list_el_type, builder),
                   val_type, ctx, module, builder);

  LLVMBuildBr(builder, merge_block);

  LLVMPositionBuilderAtEnd(builder, merge_block);

  // Create phi to merge results
  // If we came from the empty branch: result is false
  // If we came from test_elements_block: result is elements_test_result
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "list_cons_result");

  LLVMBasicBlockRef incoming_blocks[2];
  LLVMValueRef incoming_values[2];

  incoming_blocks[0] = pre_branch_block;
  incoming_values[0] = LLVMConstInt(LLVMInt1Type(), 0, 0);

  incoming_blocks[1] = test_elements_block;
  incoming_values[1] = elements_test_result;

  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
  return LLVMBuildAnd(builder, tv, tpv, "");
}

LLVMValueRef test_pattern(Ast *pattern,

                          LLVMValueRef val, Type *val_type,

                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    Ast *p = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    LLVMValueRef tv = test_pattern(p, val, val_type, ctx, module, builder);
    LLVMValueRef gv = codegen(pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr,
                              ctx, module, builder);
    return LLVMBuildAnd(builder, tv, gv, "and_test_with_guard");
  }

  switch (pattern->tag) {
  case AST_BOOL:
  case AST_INT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_STRING: {
    return _codegen_equality(val_type, val,
                             codegen(pattern, ctx, module, builder), ctx,
                             module, builder);
    break;
  }
  case AST_IDENTIFIER: {

    if (ast_is_placeholder_id(pattern)) {
      return _TRUE; // Skip placeholder bindings like '_'
    }

    // const char *chars = pattern->data.AST_IDENTIFIER.value;
    // uint64_t id_hash = hash_string(chars,
    // pattern->data.AST_IDENTIFIER.length);
    //
    // // Local binding
    // JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val,
    //                             type_to_llvm_type(val_type, ctx, module));
    // ht_set_hash(ctx->frame->table, chars, id_hash, sym);
    bind_value(pattern, val, val_type, ctx, module, builder);

    return _TRUE;
  }

  case AST_TUPLE: {
    LLVMValueRef bool_acc = _TRUE;
    char field_name[16];
    for (int i = 0; i < pattern->data.AST_LIST.len; i++) {
      Ast *p = pattern->data.AST_LIST.items + i;
      sprintf(field_name, "struct_field_%d", i);
      LLVMValueRef tv = LLVMBuildExtractValue(builder, val, i, field_name);
      LLVMValueRef pv = test_pattern(p, tv, val_type->data.T_CONS.args[i], ctx,
                                     module, builder);
      bool_acc = LLVMBuildAnd(builder, bool_acc, pv, "match_succ_accumulator");
    }
    return bool_acc;
  }
  case AST_LIST: {
    if (pattern->data.AST_LIST.len == 0) {
      return ll_is_null(
          val, type_to_llvm_type(val_type->data.T_CONS.args[0], ctx, module),
          builder);
    }
    break;
  }
  case AST_APPLICATION: {
    if (is_list_cons_operator(pattern)) {
      return test_list_cons_pattern(pattern, val, val_type, ctx, module,
                                    builder);
    }

    break;
  }

  case AST_RANGE_EXPRESSION: {
    LLVMValueRef from_val =
        codegen(pattern->data.AST_RANGE_EXPRESSION.from, ctx, module, builder);

    LLVMValueRef to_val =
        codegen(pattern->data.AST_RANGE_EXPRESSION.to, ctx, module, builder);

    LLVMValueRef from = gte_val(val, from_val, val_type, ctx, module, builder);

    LLVMValueRef test_result =
        LLVMBuildAnd(builder, from,
                     lte_val(val, to_val, val_type, ctx, module, builder), "");

    return test_result;
  }

  default: {
    fprintf(stderr, "Error: pattern type not supported\n");
    print_ast_err(pattern);
    return NULL;
  }
  }

  Ast *guard = NULL;
  return _FALSE;
}

// given a partitioned list of Ast nodes for patterns that match a sum type
// if the current pattern fails the tag comparison, skip the same few tags in
// the current group and move on to the next tag match
int next_tag_group_idx(int pidx, int num_branches, Ast *cur_pattern,
                       Ast *sorted_patterns) {
  int cur_cons_idx = get_constructor_index(cur_pattern);

  for (int i = pidx + 1; i < num_branches; i++) {
    Ast *next = sorted_patterns + 2 * i;
    int ncons_idx = get_constructor_index(next);
    if (ncons_idx != cur_cons_idx) {
      return i;
    }
  }
  return pidx + 1;
}

void test_sum_type_pattern(int pidx, int num_branches,

                           LLVMBasicBlockRef tag_blocks[],
                           LLVMBasicBlockRef test_blocks[],
                           LLVMBasicBlockRef body_blocks[],

                           Ast *pattern, Ast *sorted_patterns,

                           LLVMValueRef val, Type *val_type, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *guard = NULL;

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    guard = pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
    pattern = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
  }

  Ast *p = pattern;

  LLVMPositionBuilderAtEnd(builder, tag_blocks[pidx]);

  // Check if this is a wildcard/catch-all pattern (identifier)
  // If it's the last branch with identifier pattern, just branch
  // unconditionally
  if (p->tag == AST_IDENTIFIER && pidx == num_branches - 1) {
    // Last branch with identifier pattern - unconditional match (wildcard)
    LLVMBuildBr(builder, body_blocks[pidx]);
    return;
  }

  // compute tag and branch on tag val

  LLVMValueRef tag;
  if (!is_simple_enum(val_type)) {
    tag = LLVMBuildExtractValue(builder, val, 0, "tag");
  } else {
    tag = val;
  }

  int ptag_idx;
  Type *subtype;
  if (p->tag == AST_IDENTIFIER) {
    subtype = extract_member_from_sum_type_idx(val_type, p, &ptag_idx);
  } else if (p->tag == AST_APPLICATION) {
    Ast *id = p->data.AST_APPLICATION.function;

    subtype = extract_member_from_sum_type_idx(
        val_type, p->data.AST_APPLICATION.function, &ptag_idx);
    // print_ast(pattern);
    // print_type(subtype);

  } else {
    fprintf(stderr, "Error could not handle tag match\n");
    return;
  }

  LLVMValueRef ptag_val = LLVMConstInt(LLVMInt8Type(), ptag_idx, 0);

  LLVMValueRef tags_match =
      LLVMBuildICmp(builder, LLVMIntEQ, tag, ptag_val, "eq_int");

  LLVMBasicBlockRef tag_fail;
  LLVMBasicBlockRef tag_succ;

  if (p->tag == AST_IDENTIFIER) {
    tag_fail =
        tag_blocks[next_tag_group_idx(pidx, num_branches, p, sorted_patterns)];
    tag_succ = body_blocks[pidx];

  } else if (p->tag == AST_APPLICATION) {
    if (pidx == num_branches - 1) {
      // Last branch - if tag doesn't match, unreachable
      LLVMValueRef parent_func =
          LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
      LLVMBasicBlockRef unreachable_block =
          LLVMAppendBasicBlock(parent_func, "match.exhausted");
      tag_fail = unreachable_block;
    } else {
      tag_fail = tag_blocks[next_tag_group_idx(pidx, num_branches, p,
                                               sorted_patterns)];
    }

    tag_succ = test_blocks[pidx];
  } else {
    fprintf(stderr, "Error could not handle tag match\n");
    return;
  }
  LLVMBuildCondBr(builder, tags_match, tag_succ, tag_fail);

  // If this is the last branch and tag test failed, terminate the unreachable
  // block
  if (pidx == num_branches - 1 && p->tag == AST_APPLICATION) {
    LLVMPositionBuilderAtEnd(builder, tag_fail);
    LLVMBuildUnreachable(builder);
  }

  LLVMBasicBlockRef test_fail;
  LLVMBasicBlockRef test_succ;
  if (p->tag == AST_IDENTIFIER) {
    test_fail = tag_blocks[pidx + 1];
    test_succ = body_blocks[pidx];
  } else if (p->tag == AST_APPLICATION) {
    if (pidx == num_branches - 1) {
      // Last branch - if payload doesn't match, unreachable
      LLVMValueRef parent_func = LLVMGetBasicBlockParent(test_blocks[pidx]);
      LLVMBasicBlockRef unreachable_block =
          LLVMAppendBasicBlock(parent_func, "match.exhausted");
      test_fail = unreachable_block;
    } else {
      test_fail = tag_blocks[pidx + 1];
    }
    test_succ = body_blocks[pidx];
  } else {
    fprintf(stderr, "Error could not handle tag match\n");
    return;
  }

  LLVMPositionBuilderAtEnd(builder, test_blocks[pidx]);
  if (p->tag == AST_APPLICATION) {
    LLVMValueRef payload =
        LLVMBuildExtractValue(builder, val, 1, "extract_sum_type_payload");
    Type *payload_type = subtype->data.T_CONS.args[0];

    LLVMValueRef cast_val =
        cast_union(payload, payload_type, ctx, module, builder);

    LLVMValueRef cond = test_pattern(p->data.AST_APPLICATION.args, cast_val,
                                     payload_type, ctx, module, builder);

    if (guard) {
      cond = test_guard_clause(cond, guard, ctx, module, builder);
      LLVMBuildCondBr(builder, cond, test_succ, test_fail);

      // If this is the last branch and we created an unreachable block,
      // terminate it
      if (pidx == num_branches - 1) {
        LLVMPositionBuilderAtEnd(builder, test_fail);
        LLVMBuildUnreachable(builder);
      }
      return;
    }

    LLVMBuildCondBr(builder, cond, test_succ, test_fail);

    // If this is the last branch and we created an unreachable block,
    // terminate it
    if (pidx == num_branches - 1) {
      LLVMPositionBuilderAtEnd(builder, test_fail);
      LLVMBuildUnreachable(builder);
    }
  }
}

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef test_val =
      codegen(ast->data.AST_MATCH.expr, ctx, module, builder);

  if (!test_val) {
    print_ast(ast);
    fprintf(stderr, "could not compile test expression\n");
    return NULL;
  }

  Type *val_type = ast->data.AST_MATCH.expr->type;
  Type *result_type = ast->type;
  LLVMTypeRef llvm_result_type = type_to_llvm_type(result_type, ctx, module);

  int num_branches = ast->data.AST_MATCH.len;

  // Rearrange branches if matching on sum type
  Ast *branches = ast->data.AST_MATCH.branches;
  Ast sorted_branches_storage[num_branches * 2];

  int is_match_over_sum = is_sum_type(branches->type);

  if (num_branches > 0 && is_match_over_sum) {
    stable_partition_match_over_sum_type(branches, num_branches,
                                         sorted_branches_storage);
    branches = sorted_branches_storage;
  }

  LLVMValueRef parent_func =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  bool is_tail_position = ast->is_body_tail;

  LLVMBasicBlockRef current_insert_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef merge_block = NULL;
  LLVMValueRef result_phi = NULL;
  bool is_void_type = (LLVMGetTypeKind(llvm_result_type) == LLVMVoidTypeKind);

  if (!is_tail_position) {
    merge_block = LLVMAppendBasicBlock(parent_func, "match.merge");
    LLVMPositionBuilderAtEnd(builder, merge_block);

    // Only create phi node if result type is not void
    if (!is_void_type) {
      result_phi = LLVMBuildPhi(builder, llvm_result_type, "match.result");
    }

    LLVMPositionBuilderAtEnd(builder, current_insert_block);
  }

  int num_branches_to_merge = 0;

  char block_name[32];

  LLVMBasicBlockRef tag_blocks[num_branches];
  LLVMBasicBlockRef test_blocks[num_branches];
  // LLVMBasicBlockRef guard_blocks[num_branches];
  LLVMBasicBlockRef body_blocks[num_branches];

  for (int i = 0; i < num_branches; i++) {

    if (is_match_over_sum) {
      sprintf(block_name, "match.test.tag.%d", i);
      tag_blocks[i] = LLVMAppendBasicBlock(parent_func, block_name);
    }

    sprintf(block_name, "match.test.%d", i);
    test_blocks[i] = LLVMAppendBasicBlock(parent_func, block_name);
    sprintf(block_name, "match.body.%d", i);

    // Ast *pattern = branches + (2 * i);
    // if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    //   sprintf(block_name, "match.test.guard.%d", i);
    //   guard_blocks[i] = LLVMAppendBasicBlock(parent_func, block_name);
    // }

    sprintf(block_name, "match.body.%d", i);
    body_blocks[i] = LLVMAppendBasicBlock(parent_func, block_name);
  }

  // Branch from entry block to first test block
  if (is_match_over_sum) {
    LLVMBuildBr(builder, tag_blocks[0]);
  } else {
    LLVMBuildBr(builder, test_blocks[0]);
  }

  for (int i = 0; i < num_branches; i++) {
    Ast *pattern = branches + (2 * i);
    Ast *branch_expr = branches + (2 * i + 1);

    STACK_ALLOC_CTX_PUSH(branch_ctx_mem, ctx)
    JITLangCtx branch_ctx = branch_ctx_mem;

    // print_type_env(ctx->env);
    if (val_type->alias &&
        type_contains_recursive_ref(val_type, val_type->alias) &&
        !lookup_type_ref(branch_ctx.env, val_type->alias)) {

      branch_ctx.env = env_extend(branch_ctx.env, val_type->alias, val_type);
    }

    if (is_match_over_sum) {
      test_sum_type_pattern(i, num_branches, tag_blocks, test_blocks,
                            // guard_blocks,
                            body_blocks, pattern, branches, test_val, val_type,
                            &branch_ctx, module, builder);
    } else {
      // Position builder at the test block for this pattern
      LLVMPositionBuilderAtEnd(builder, test_blocks[i]);

      LLVMValueRef pat_res = test_pattern(pattern, test_val, val_type,
                                          &branch_ctx, module, builder);

      LLVMBasicBlockRef fail_dest;
      if (i == num_branches - 1) {
        // Last branch - pattern must match or unreachable
        LLVMBasicBlockRef unreachable_block =
            LLVMAppendBasicBlock(parent_func, "match.exhausted");
        fail_dest = unreachable_block;
        LLVMBuildCondBr(builder, pat_res, body_blocks[i], fail_dest);
        LLVMPositionBuilderAtEnd(builder, unreachable_block);
        LLVMBuildUnreachable(builder);
      } else {
        fail_dest = test_blocks[i + 1];
        LLVMBuildCondBr(builder, pat_res, body_blocks[i], fail_dest);
      }
    }

    LLVMPositionBuilderAtEnd(builder, body_blocks[i]);
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

    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);

    if (is_tail_position) {
      // In tail position - return directly if not already terminated
      if (!LLVMGetBasicBlockTerminator(current_block)) {
        if (LLVMIsACallInst(branch_result)) {
          LLVMSetTailCall(branch_result, true);
        }
        build_ret(branch_result, branch_expr->type, builder);
      }
    } else {
      // Not in tail position - branch to merge block
      if (!LLVMGetBasicBlockTerminator(current_block)) {
        LLVMBuildBr(builder, merge_block);

        // Only add to phi if we have one (non-void type)
        if (result_phi && !is_void_type) {
          LLVMAddIncoming(result_phi, &branch_result, &current_block, 1);
        }
        num_branches_to_merge++;
      }
    }

    destroy_ctx(&branch_ctx);
  }

  // Delete any unreachable blocks (test blocks that were never used)
  for (int i = 0; i < num_branches; i++) {
    if (!LLVMGetFirstUse(LLVMBasicBlockAsValue(test_blocks[i]))) {
      LLVMDeleteBasicBlock(test_blocks[i]);
    }
  }

  // Handle merge block
  if (!is_tail_position) {
    if (num_branches_to_merge == 0) {
      LLVMDeleteBasicBlock(merge_block);
      LLVMBasicBlockRef last_block = LLVMGetInsertBlock(builder);
      if (!LLVMGetBasicBlockTerminator(last_block)) {
        LLVMBuildUnreachable(builder);
      }
      return LLVMGetUndef(llvm_result_type);

    } else {
      LLVMPositionBuilderAtEnd(builder, merge_block);
      // Return phi for non-void, or undef for void
      return is_void_type ? LLVMGetUndef(llvm_result_type) : result_phi;
    }
  } else {
    LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
    if (current && !LLVMGetBasicBlockTerminator(current)) {
      LLVMBuildUnreachable(builder);
    }

    return LLVMGetUndef(llvm_result_type);
  }
}
