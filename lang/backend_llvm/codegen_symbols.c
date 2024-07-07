#include "backend_llvm/codegen_symbols.h"
#include "codegen_tuple.h"
#include "codegen_types.h"
#include "serde.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

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
static LLVMValueRef current_func(LLVMBuilderRef builder) {
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

  if (res->symbol_type == STYPE_TOP_LEVEL_VAR) {
    LLVMValueRef glob = LLVMGetNamedGlobal(module, chars);
    LLVMValueRef val = LLVMGetInitializer(glob);
    //
    //
    // LLVMValueRef glob = res->val;
    // LLVMValueRef val = LLVMBuildLoad2(builder, res->llvm_type, glob, "");
    return val;
  } else if (res->symbol_type == STYPE_LOCAL_VAR) {
    LLVMValueRef val = LLVMBuildLoad2(builder, res->llvm_type, res->val, "");
    return val;
  } else if (res->symbol_type == STYPE_FN_PARAM) {
    int idx = res->idx;
    return LLVMGetParam(current_func(builder), idx);
  } else if (res->symbol_type == STYPE_FUNCTION) {
    printf("found res fn::::\n");
    return LLVMGetNamedFunction(module, chars);
  }

  return res->val;
}

void bind_symbol_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                          LLVMValueRef val, symbol_type sym_type,
                          JITLangCtx *ctx) {
  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = type, .symbol_type = sym_type, .val = val};
  ht *scope = ctx->stack + ctx->stack_ptr;
  ht_set_hash(scope, id, id_hash, v);
}

LLVMValueRef codegen_single_assignment(Ast *id, LLVMValueRef expr_val,
                                       Type *expr_type, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder, bool fn_param) {

  LLVMTypeRef llvm_val_type = LLVMTypeOf(expr_val);
  const char *id_chars = id->data.AST_IDENTIFIER.value;
  int id_length = id->data.AST_IDENTIFIER.length;
  uint64_t id_hash = hash_string(id_chars, id_length);

  if (fn_param) {
    bind_symbol_in_scope(id_chars, id_hash, llvm_val_type, expr_val,
                         STYPE_FN_PARAM, ctx);
  }

  if (expr_type->kind == T_FN) {
    bind_symbol_in_scope(id_chars, id_hash, llvm_val_type, expr_val,
                         STYPE_FUNCTION, ctx);
    return expr_val;
  }
  if (ctx->stack_ptr == 0) {

    // top-level
    LLVMValueRef alloca_val =
        LLVMAddGlobalInAddressSpace(module, llvm_val_type, id_chars, 0);
    LLVMSetInitializer(alloca_val, expr_val);

    // LLVMValueRef alloca_val =
    //     LLVMBuildMalloc(builder, llvm_val_type, "top_level_var");
    // LLVMBuildStore(builder, expr_val, alloca_val);

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
                                         bool fn_param) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return codegen_single_assignment(binding, expr_val, expr_type, ctx, module,
                                     builder, fn_param);
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
                                       module, builder, fn_param)) {
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
  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, ambient_ctx, module, builder);

  Type *expr_type = ast->data.AST_LET.expr->md;

  if (!expr_val) {
    return NULL;
  }

  LLVMTypeRef llvm_expr_type = LLVMTypeOf(expr_val);
  JITLangCtx ctx = {.stack = ambient_ctx->stack,
                    .stack_ptr = ast->data.AST_LET.in_expr != NULL
                                     ? ambient_ctx->stack_ptr + 1
                                     : ambient_ctx->stack_ptr

  };

  codegen_multiple_assignment(binding_identifier, expr_val, expr_type, &ctx,
                              module, builder, false);

  if (ast->data.AST_LET.in_expr != NULL) {
    LLVMValueRef res =
        codegen(ast->data.AST_LET.in_expr, &ctx, module, builder);
    return res;
  }

  return expr_val;
}
