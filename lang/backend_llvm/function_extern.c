#include "./function_extern.h"
#include "types.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <string.h>

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);

  if (fn == NULL) {
    fn = LLVMAddFunction(module, name, fn_type);

    // Set C calling convention for extern functions
    // This ensures LLVM uses the platform's C ABI, including proper
    // struct return handling (sret) for large return values
    LLVMSetFunctionCallConv(fn, LLVMCCallConv);
  }
  return fn;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);
  Type *fn_type = ast->type;

  if (fn_type->kind == T_SCHEME) {
    TICtx _c = {.env = ctx->env};
    fn_type = instantiate(fn_type, &_c);
  }
  int params_count = fn_type_args_len(fn_type);

  if (params_count == 1 && fn_type->data.T_FN.from->kind == T_VOID) {

    LLVMTypeRef ret_type =
        type_to_llvm_type(fn_type->data.T_FN.to, ctx, module);
    LLVMTypeRef llvm_fn_type = LLVMFunctionType(ret_type, NULL, 0, false);
    return get_extern_fn(name, llvm_fn_type, module);
  }
  LLVMTypeRef llvm_fn_type = type_to_llvm_type(fn_type, ctx, module);

  LLVMValueRef val = get_extern_fn(name, llvm_fn_type, module);
  return val;
}

LLVMValueRef instantiate_extern_fn_sym(JITSymbol *sym, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  if (sym->val == NULL) {
    LLVMValueRef val = codegen_extern_fn(
        sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast, ctx, module, builder);
    sym->val = val;
  }
  return sym->val;
}
