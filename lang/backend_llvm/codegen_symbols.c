#include "backend_llvm/codegen_symbols.h"
#include "codegen_list.h"
#include "codegen_tuple.h"
#include "codegen_types.h"
#include "types/util.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
// #define _DBG_GENERIC_SYMBOLS

// forward decl
JITSymbol *create_generic_fn_symbol(Ast *binding_identifier, Ast *fn_ast,
                                    JITLangCtx *ctx);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

int codegen_lookup_id(const char *id, int length, JITLangCtx *ctx,
                      JITSymbol **result) {

  ObjString key = {.chars = id, length, hash_string(id, length)};
  JITSymbol *res = NULL;

  int ptr = ctx->stack_ptr;

  while (ptr >= 0 && !((res = (JITSymbol *)ht_get_hash(ctx->stack + ptr,
                                                       key.chars, key.hash)))) {
    ptr--;
  }

  if (!res) {
    return 1;
  }
  *result = res;
  return 0;
}

JITSymbol *lookup_id_mutable(const char *id, int length, JITLangCtx *ctx) {

  ObjString key = {.chars = id, length, hash_string(id, length)};
  int ptr = ctx->stack_ptr;

  while (ptr >= 0) {
    JITSymbol *res = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
    if (res != NULL) {
      return res;
    }
    ptr--;
  }
  return NULL;
}

LLVMValueRef current_func(LLVMBuilderRef builder) {
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  return LLVMGetBasicBlockParent(current_block);
}

LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  const char *chars = ast->data.AST_IDENTIFIER.value;
  int length = ast->data.AST_IDENTIFIER.length;

  JITSymbol *res = NULL;

  if (codegen_lookup_id(chars, length, ctx, &res)) {

    printf("codegen identifier failed symbol %s not found in scope %d\n", chars,
           ctx->stack_ptr);
    return NULL;
  }

  if (res->type == STYPE_TOP_LEVEL_VAR) {
    LLVMValueRef glob = LLVMGetNamedGlobal(module, chars);
    LLVMValueRef val = LLVMGetInitializer(glob);
    return val;
  } else if (res->type == STYPE_LOCAL_VAR) {
    LLVMValueRef val = LLVMBuildLoad2(builder, res->llvm_type, res->val, "");
    return val;
  } else if (res->type == STYPE_FN_PARAM) {
    int idx = res->symbol_data.STYPE_FN_PARAM;
    return LLVMGetParam(current_func(builder), idx);
  } else if (res->type == STYPE_FUNCTION) {
    return res->val;
  } else if (res->type == STYPE_GENERIC_FUNCTION) {
    return NULL;
  }

  return res->val;
}

void bind_symbol_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                          LLVMValueRef val, symbol_type sym_type,
                          JITLangCtx *ctx) {

  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = type, .type = sym_type, .val = val};

  ht *scope = ctx->stack + ctx->stack_ptr;
  ht_set_hash(scope, id, id_hash, v);
}

void bind_fn_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                      LLVMValueRef val, symbol_type sym_type, JITLangCtx *ctx) {

  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = type, .type = sym_type, .val = val};

  ht *scope = ctx->stack + ctx->stack_ptr;
  ht_set_hash(scope, id, id_hash, v);
}

void bind_fn_param_in_scope(const char *id, uint64_t id_hash, int param_idx,
                            LLVMTypeRef type, LLVMValueRef val,
                            symbol_type sym_type, JITLangCtx *ctx) {
  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = type,
                   .type = sym_type,
                   .val = val,
                   .symbol_data = {.STYPE_FN_PARAM = param_idx}};
  ht *scope = ctx->stack + ctx->stack_ptr;
  ht_set_hash(scope, id, id_hash, v);
}

LLVMValueRef codegen_single_assignment(Ast *id, LLVMValueRef expr_val,
                                       Type *expr_type, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder, bool is_fn_param,
                                       int fn_param_idx) {
  if (ast_is_placeholder_id(id)) {
    return expr_val;
  }

  const char *id_chars = id->data.AST_IDENTIFIER.value;
  // printf("binding symbol '%s'\n", id_chars);
  int id_length = id->data.AST_IDENTIFIER.length;
  uint64_t id_hash = hash_string(id_chars, id_length);

  if (is_fn_param) {
    bind_fn_param_in_scope(id_chars, id_hash, fn_param_idx, NULL, expr_val,
                           STYPE_FN_PARAM, ctx);
    return expr_val;
  }
  LLVMTypeRef llvm_val_type = LLVMTypeOf(expr_val);

  if (expr_type->kind == T_FN) {

    JITSymbol *v = malloc(sizeof(JITSymbol));
    *v = (JITSymbol){.llvm_type = LLVMTypeOf(expr_val),
                     .type = STYPE_FUNCTION,
                     .val = expr_val,
                     .symbol_data = {.STYPE_FUNCTION = {.fn_type = expr_type}}};

    ht *scope = ctx->stack + ctx->stack_ptr;
    ht_set_hash(scope, id_chars, id_hash, v);
    return expr_val;
  }

  if (ctx->stack_ptr == 0) {

    // top-level
    LLVMValueRef alloca_val =
        LLVMAddGlobalInAddressSpace(module, llvm_val_type, id_chars, 0);
    LLVMSetInitializer(alloca_val, expr_val);

    bind_symbol_in_scope(id_chars, id_hash, llvm_val_type, alloca_val,
                         STYPE_TOP_LEVEL_VAR, ctx);

    return expr_val;
  }

  // not-top-level or symbol is used in subsequent in expr
  LLVMValueRef alloca_val = LLVMBuildAlloca(builder, llvm_val_type, id_chars);
  LLVMBuildStore(builder, expr_val, alloca_val);

  bind_symbol_in_scope(id_chars, id_hash, llvm_val_type, alloca_val,
                       STYPE_LOCAL_VAR, ctx);

  return expr_val;
}

LLVMValueRef codegen_multiple_assignment(Ast *binding, LLVMValueRef expr_val,
                                         Type *expr_type, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         bool is_fn_param, int fn_param_idx) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {

    return codegen_single_assignment(binding, expr_val, expr_type, ctx, module,
                                     builder, is_fn_param, fn_param_idx);
  }
  case AST_BINOP: {
    if (binding->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
      Ast *left = binding->data.AST_BINOP.left;

      LLVMValueRef res_left = codegen_multiple_assignment(
          left, ll_get_head_val(expr_val, type_to_llvm_type(left->md), builder),
          left->md, ctx, module, builder, is_fn_param, fn_param_idx);

      // assign left to first member

      Ast *right = binding->data.AST_BINOP.right;
      LLVMValueRef res_right = codegen_multiple_assignment(
          right, ll_get_next(expr_val, type_to_llvm_type(left->md), builder),
          left->md, ctx, module, builder, is_fn_param, fn_param_idx);

      if (res_left || res_right) {
        return expr_val;
      } else {
        return NULL;
      }

    } else {
      fprintf(stderr, "Error - codegen assignment: invalid binding syntax");
      return NULL;
    }
  }
  case AST_TUPLE: {
    if (!is_tuple_type(expr_type)) {
      fprintf(stderr, "Error - cannot destructure a non-tuple");
      return NULL;
    }

    int binding_len = binding->data.AST_LIST.len;
    if (binding_len != expr_type->data.T_CONS.num_args) {
      fprintf(
          stderr,
          "Error - cannot destructure a tuple with %d members to %d bindings",
          expr_type->data.T_CONS.num_args, binding_len);
      return NULL;
    }

    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      if (ast_is_placeholder_id(b)) {
        continue;
      }
      LLVMValueRef tuple_member =
          codegen_tuple_access(i, expr_val, LLVMTypeOf(expr_val), builder);
      Type *tuple_member_type = expr_type->data.T_CONS.args[i];

      if (!codegen_multiple_assignment(b, tuple_member, tuple_member_type, ctx,
                                       module, builder, is_fn_param,
                                       fn_param_idx)) {
        return NULL;
      }
    }

    return expr_val;
  }
  default: {
    fprintf(stderr,
            "Error - codegen assignment: binding syntax not yet supported");
    return NULL;
  }
  }
}

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *ambient_ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *binding_identifier = ast->data.AST_LET.binding;

  JITLangCtx ctx = {.stack = ambient_ctx->stack,
                    .stack_ptr = ast->data.AST_LET.in_expr != NULL
                                     ? ambient_ctx->stack_ptr + 1
                                     : ambient_ctx->stack_ptr

  };

  if (is_generic(ast->md) && ast->data.AST_LET.expr->tag == AST_LAMBDA) {
    JITSymbol *generic_sym = create_generic_fn_symbol(
        binding_identifier, ast->data.AST_LET.expr, ambient_ctx);

    ht *scope = ambient_ctx->stack + ambient_ctx->stack_ptr;
    const char *id = binding_identifier->data.AST_IDENTIFIER.value;
    int id_len = binding_identifier->data.AST_IDENTIFIER.length;
    ht_set_hash(scope, id, hash_string(id, id_len), generic_sym);

#ifdef _DBG_GENERIC_SYMBOLS
    printf("generic symbol '%s':\n", id);
    print_type(generic_sym->symbol_data.STYPE_GENERIC_FUNCTION.ast->md);
    printf("\n");
#endif
    return NULL;
  }

  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, ambient_ctx, module, builder);

  LLVMTypeRef llvm_expr_type = LLVMTypeOf(expr_val);
  Type *expr_type = ast->data.AST_LET.expr->md;

  if (!expr_val) {
    return NULL;
  }
  codegen_multiple_assignment(binding_identifier, expr_val, expr_type, &ctx,
                              module, builder, false, 0);

  if (ast->data.AST_LET.in_expr != NULL) {
    LLVMValueRef res =
        codegen(ast->data.AST_LET.in_expr, &ctx, module, builder);
    return res;
  }

  return expr_val;
}
