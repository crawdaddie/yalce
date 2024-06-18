#include "backend_llvm/symbols.h"
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

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  ObjString name = ast->data.AST_LET.name;

  LLVMValueRef expr_val = codegen(ast->data.AST_LET.expr, ctx, module, builder);
  LLVMTypeRef type = LLVMTypeOf(expr_val);
  // fprintf(stderr, "let %s = fn: type %d\n", name.chars,
  // LLVMGetTypeKind(type)); LLVMDumpValue(expr_val);

  if (ast->data.AST_LET.expr->tag == AST_LAMBDA) {
    JITSymbol *v = malloc(sizeof(JITSymbol));

    *v = (JITSymbol){
        .llvm_type = type, .symbol_type = STYPE_FUNCTION, .val = expr_val};

    ht_set_hash(ctx->stack + ctx->stack_ptr, name.chars, name.hash, v);
    return expr_val;
  }

  if (ctx->stack_ptr == 0) {
    // top-level
    LLVMValueRef alloca_val =
        LLVMAddGlobalInAddressSpace(module, type, name.chars, 0);

    LLVMSetInitializer(alloca_val, expr_val);

    JITSymbol *v = malloc(sizeof(JITSymbol));

    *v = (JITSymbol){.llvm_type = type, .symbol_type = STYPE_TOP_LEVEL_VAR};

    ht_set_hash(ctx->stack + ctx->stack_ptr, name.chars, name.hash, v);

    return alloca_val;
  } else {
    // top-level
    LLVMValueRef alloca_val = LLVMBuildAlloca(builder, type, name.chars);
    LLVMBuildStore(builder, expr_val, alloca_val);

    JITSymbol *v = malloc(sizeof(JITSymbol));
    *v = (JITSymbol){
        .llvm_type = type, .val = alloca_val, .symbol_type = STYPE_LOCAL_VAR};

    ht_set_hash(ctx->stack + ctx->stack_ptr, name.chars, name.hash, v);

    return alloca_val;
  }

  return expr_val;
}
