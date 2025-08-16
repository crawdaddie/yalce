#include "./binding.h"
#include "./adt.h"
#include "./builtin_functions.h"
#include "./coroutines.h"
#include "./globals.h"
#include "./list.h"
#include "./symbols.h"
#include "./tuple.h"
#include "./types.h"
#include "./util.h"
#include "types/inference.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef match_list_prepend(Ast *binding, LLVMValueRef list,
                                Type *list_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *head_expr = binding->data.AST_APPLICATION.args;
  Ast *tail_expr = binding->data.AST_APPLICATION.args + 1;

  Type *list_el_type = list_type->data.T_CONS.args[0];
  if (is_generic(list_el_type)) {
    list_el_type = resolve_type_in_env(list_el_type, ctx->env);
  }

  LLVMTypeRef llvm_list_el_type =
      list_el_type->kind == T_FN ? GENERIC_PTR
                                 : type_to_llvm_type(list_el_type, ctx, module);

  LLVMValueRef list_empty = ll_is_null(list, llvm_list_el_type, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef match_block =
      LLVMAppendBasicBlock(function, "binding.list.match");
  LLVMBasicBlockRef merge_block =
      LLVMAppendBasicBlock(function, "binding.list.merge");

  LLVMBuildCondBr(builder, list_empty, merge_block, match_block);

  LLVMPositionBuilderAtEnd(builder, match_block);

  LLVMValueRef head_match = codegen_pattern_binding(
      head_expr, ll_get_head_val(list, llvm_list_el_type, builder),
      list_el_type, ctx, module, builder);

  LLVMValueRef tail_match = codegen_pattern_binding(
      tail_expr, ll_get_next(list, llvm_list_el_type, builder), list_type, ctx,
      module, builder);

  LLVMValueRef match_result =
      LLVMBuildAnd(builder, head_match, tail_match, "binding.list.result");

  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef match_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "result");

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

    JITSymbol *sym = lookup_id_ast(binding, ctx);

    if (sym && sym->storage) {

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
            1, val, type_to_llvm_type(cons_type, ctx, module), builder);

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
    LLVMTypeRef llvm_tuple_type = type_to_llvm_type(val_type, ctx, module);
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
        type_to_llvm_type(list_el_type, ctx, module);

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
