#include "backend_llvm/match.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/types.h"
#include "coroutines.h"
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

LLVMValueRef codegen_equality(LLVMValueRef left, Type *left_type,
                              LLVMValueRef right, Type *right_type,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  if (left_type->kind == T_INT) {
    Method method = left_type->implements[0]->methods[0];

    LLVMBinopMethod binop_method = method.method;
    return binop_method(left, right, module, builder);
  }

  if (left_type->kind == T_NUM) {
    Method method = left_type->implements[0]->methods[0];

    LLVMBinopMethod binop_method = method.method;
    return binop_method(left, right, module, builder);
  }

  if (left_type->kind == T_CHAR) {

    return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "char_cmp");
  }

  if (left_type->kind == T_BOOL) {
    return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "bool_cmp");
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

    LLVMValueRef test_value = match_values(test_expr, test_val, test_val_type,
                                           &branch_ctx, module, builder);

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

LLVMValueRef match_values(Ast *binding, LLVMValueRef val, Type *val_type,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  switch (binding->tag) {
  case AST_MATCH_GUARD_CLAUSE: {
    LLVMValueRef test_val =
        match_values(binding->data.AST_MATCH_GUARD_CLAUSE.test_expr, val,
                     val_type, ctx, module, builder);

    LLVMValueRef guard_val = codegen(
        binding->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx, module, builder);
    return LLVMBuildAnd(builder, test_val, guard_val, "guard_and_test");
  }
  case AST_IDENTIFIER: {

    const char *id_chars = binding->data.AST_IDENTIFIER.value;

    int id_len = binding->data.AST_IDENTIFIER.length;

    if (*(binding->data.AST_IDENTIFIER.value) == '_' &&
        binding->data.AST_IDENTIFIER.length == 1) {
      return _TRUE;
    }

    if (is_coroutine_generator_fn(val_type)) {

      Type *def_type = val_type;
      Type *instance_type = fn_return_type(def_type);
      LLVMTypeRef llvm_def_type;
      LLVMValueRef coroutine_func = val;

      JITSymbol *def_sym = new_symbol(STYPE_COROUTINE_GENERATOR, def_type,
                                      coroutine_func, llvm_def_type);

      def_sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type =
          type_to_llvm_type(
              instance_type->data.T_COROUTINE_INSTANCE.params_type, ctx->env,
              module);

      const char *id_chars = binding->data.AST_IDENTIFIER.value;
      int id_len = binding->data.AST_IDENTIFIER.length;

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), def_sym);

      return _TRUE;
    }
    if (val_type->kind == T_FN && !(is_generic(val_type))) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym = new_symbol(STYPE_FUNCTION, val_type, val, llvm_type);

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

      return _TRUE;
    }
    if (val_type->kind == T_COROUTINE_INSTANCE) {
      LLVMTypeRef llvm_instance_type = coroutine_instance_type(
          type_to_llvm_type(val_type->data.T_COROUTINE_INSTANCE.params_type,
                            ctx->env, module));

      JITSymbol *sym = new_symbol(STYPE_COROUTINE_INSTANCE, val_type, val,
                                  llvm_instance_type);

      Type *inst_ret = val_type->data.T_COROUTINE_INSTANCE.yield_interface;
      inst_ret = fn_return_type(inst_ret);
      Type *inst_params = val_type->data.T_COROUTINE_INSTANCE.params_type;

      sym->symbol_data.STYPE_COROUTINE_INSTANCE.def_fn_type = LLVMFunctionType(
          type_to_llvm_type(inst_ret, ctx->env, module),
          (LLVMTypeRef[]){LLVMPointerType(llvm_instance_type, 0)}, 1, 0);

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

    JITSymbol *sym = lookup_id_in_current_scope(binding, ctx);

    int vidx;
    Type *v = variant_lookup_name(ctx->env, binding->data.AST_IDENTIFIER.value,
                                  &vidx);

    if (!sym && !v) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);

      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val, llvm_type);
      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
      return _TRUE;
    }

    if (v) {
      // MATCH SIMPLE ENUM MEMBER
      LLVMValueRef simple_enum_member =
          codegen_simple_enum_member(binding, ctx, module, builder);

      if (simple_enum_member) {
        return codegen_eq_int(simple_enum_member, val, module, builder);
      }
    }

    return _FALSE;
  }
  case AST_APPLICATION: {
    if (strcmp(
            binding->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
            "::") == 0) {

      if (!is_list_type(val_type)) {
        fprintf(stderr, "Error: list operations on non-list type\n");
        print_ast_err(binding);
        return NULL;
      }
      Type *el_type = val_type->data.T_CONS.args[0];

      // test x::rest against val - if val is empty list then fail
      // if val has at least one member
      // pop the head and bind 'x' to head
      // bind 'rest' to the tail of val - tail can be empty
      //
      LLVMTypeRef el_type_llvm = type_to_llvm_type(el_type, ctx->env, module);
      LLVMValueRef res = _TRUE;
      LLVMValueRef is_not_null = ll_is_not_null(val, el_type_llvm, builder);
      res = LLVMBuildAnd(builder, res, is_not_null, "");

      Ast *left = binding->data.AST_APPLICATION.args;
      LLVMValueRef head = ll_get_head_val(val, el_type_llvm, builder);

      res = LLVMBuildAnd(
          builder, res, match_values(left, head, el_type, ctx, module, builder),
          "");

      LLVMValueRef rest = ll_get_next(val, el_type_llvm, builder);

      Ast *right = binding->data.AST_APPLICATION.args + 1;
      res = LLVMBuildAnd(
          builder, res,
          match_values(right, rest, val_type, ctx, module, builder), "");
      return res;
    }

    int vidx;
    Type *variant_parent = variant_lookup(ctx->env, binding->md, &vidx);

    if (variant_parent) {
      LLVMValueRef tags_match =
          codegen_eq_int(variant_extract_tag(val, builder),
                         LLVMConstInt(TAG_TYPE, vidx, 0), module, builder);

      Type *contained_type = ((Type *)binding->md)->data.T_CONS.args[0];

      LLVMValueRef vals_match = match_values(
          binding->data.AST_APPLICATION.args,
          variant_extract_value(
              val, type_to_llvm_type(contained_type, ctx->env, module),
              builder),
          contained_type, ctx, module, builder);

      return LLVMBuildAnd(builder, tags_match, vals_match, "");
    }

    // TODO: build more cons cases
    return _FALSE;
  }

  // case AST_BINOP: {
  //   token_type op = binding->data.AST_BINOP.op;
  //   if (op != TOKEN_DOUBLE_COLON) {
  //     break;
  //   }
  //
  //   if (!is_list_type(val_type)) {
  //     break;
  //   }
  //
  // }
  case AST_LIST: {
    if (binding->data.AST_LIST.len == 0) {

      Type *el_type = val_type->data.T_CONS.args[0];
      LLVMTypeRef el_type_llvm = type_to_llvm_type(el_type, ctx->env, module);

      LLVMValueRef is_null = ll_is_null(val, el_type_llvm, builder);
      return is_null;
    }
  }

  case AST_TUPLE: {
    LLVMValueRef and = _TRUE;

    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      Type *el_type = val_type->data.T_CONS.args[i];
      LLVMTypeRef tuple_type = type_to_llvm_type(val_type, ctx->env, module);
      LLVMValueRef match_res =
          match_values(binding->data.AST_LIST.items + i,
                       codegen_tuple_access(i, val, tuple_type, builder),
                       el_type, ctx, module, builder);
      and = LLVMBuildAnd(builder, and, match_res, "");
    }

    return and;
  }
  case AST_VOID: {
    return _TRUE;
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
