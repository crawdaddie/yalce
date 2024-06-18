#include "backend_llvm/symbols.h"
#include "serde.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  char *chars = ast->data.AST_IDENTIFIER.value;
  int length = ast->data.AST_IDENTIFIER.length;

  JITSymbol res;

  if (codegen_lookup_id(chars, length, ctx, &res)) {
    return NULL;
  }

  if (res.symbol_type == STYPE_TOP_LEVEL_VAR) {
    LLVMValueRef glob = LLVMGetNamedGlobal(module, chars);
    LLVMValueRef val = LLVMGetInitializer(glob);
    return val;
  } else if (res.symbol_type == STYPE_LOCAL_VAR) {
    LLVMValueRef val = LLVMBuildLoad2(builder, res.llvm_type, res.val, "");
    return val;
  } else if (res.symbol_type == STYPE_FN_PARAM) {
    // printf("found param %s\n", chars);

    // LLVMValueRef param_val = LLVMGetParam(func, i);
    // LLVMDumpValue(res.val);
    // LLVMDumpType(res.llvm_type);
    return res.val;
  } else if (res.symbol_type == STYPE_FUNCTION) {
    return LLVMGetNamedFunction(module, chars);
  }

  return res.val;
}

int codegen_lookup_id(const char *id, int length, JITLangCtx *ctx,
                      JITSymbol *result) {

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
  *result = *res;
  return 0;
}

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *_ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  ObjString name = ast->data.AST_LET.name;

  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, _ctx, module, builder);
  LLVMTypeRef type = LLVMTypeOf(expr_val);
  print_ast(ast);

  ht *next_scope;

  JITLangCtx ctx = {.stack = _ctx->stack,
                    .stack_ptr = ast->data.AST_LET.in_expr != NULL
                                     ? _ctx->stack_ptr + 1
                                     : _ctx->stack_ptr

  };

  ht *scope = ctx.stack + ctx.stack_ptr;

  if (ast->data.AST_LET.expr->tag == AST_LAMBDA) {
    JITSymbol *v = malloc(sizeof(JITSymbol));

    *v = (JITSymbol){
        .llvm_type = type, .symbol_type = STYPE_FUNCTION, .val = expr_val};

    ht_set_hash(scope, name.chars, name.hash, v);
  }

  if (ctx.stack_ptr == 0) {
    // top-level
    LLVMValueRef alloca_val =
        LLVMAddGlobalInAddressSpace(module, type, name.chars, 0);

    LLVMSetInitializer(alloca_val, expr_val);

    JITSymbol *v = malloc(sizeof(JITSymbol));

    *v = (JITSymbol){.llvm_type = type, .symbol_type = STYPE_TOP_LEVEL_VAR};

    ht_set_hash(scope, name.chars, name.hash, v);

  } else {
    // top-level
    LLVMValueRef alloca_val = LLVMBuildAlloca(builder, type, name.chars);
    LLVMBuildStore(builder, expr_val, alloca_val);

    JITSymbol *v = malloc(sizeof(JITSymbol));
    *v = (JITSymbol){
        .llvm_type = type, .val = alloca_val, .symbol_type = STYPE_LOCAL_VAR};

    ht_set_hash(scope, name.chars, name.hash, v);
  }

  if (ast->data.AST_LET.in_expr != NULL) {

    fprintf(stderr, "in expr\n");

    LLVMValueRef res =
        codegen(ast->data.AST_LET.in_expr, &ctx, module, builder);
    LLVMDumpValue(res);
    return res;
  }

  return expr_val;
}
