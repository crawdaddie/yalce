#include "backend_llvm/application.h"
#include "function.h"
#include "serde.h"
#include "symbols.h"
#include "llvm-c/Core.h"

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  // printf("handle type conversions\n");
  // print_type(from_type);
  // print_type(to_type);

  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (!to_type->constructor) {
    return val;
  }

  ConsMethod constructor = to_type->constructor;
  return constructor(val, from_type, module, builder);
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (!sym) {
    return NULL;
  }

  if (sym->type == STYPE_GENERIC_FUNCTION &&
      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
    return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
        ast, ctx, module, builder);
  }

  if (sym->type == STYPE_FUNCTION) {
    int args_len = ast->data.AST_APPLICATION.len;

    Type *callable_type = sym->symbol_type;
    int expected_args_len = fn_type_args_len(callable_type);
    LLVMValueRef callable = sym->val;
    LLVMTypeRef llvm_callable_type = LLVMGlobalGetValueType(callable);

    if (callable_type->kind == T_FN &&
        callable_type->data.T_FN.from->kind == T_VOID) {

      return LLVMBuildCall2(builder, llvm_callable_type, callable, NULL, 0,
                            "call_func");
    }
    if (args_len < expected_args_len) {
      // TODO: currying
      return NULL;
    }

    LLVMValueRef app_vals[args_len];
    for (int i = 0; i < args_len; i++) {
      LLVMValueRef app_val =
          codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);

      Type *expected_type = callable_type->data.T_FN.from;

      callable_type = callable_type->data.T_FN.to;

      app_vals[i] = app_val;
    }

    return LLVMBuildCall2(builder, llvm_callable_type, callable, app_vals,
                          args_len, "call_func");
  }

  return NULL;
}
