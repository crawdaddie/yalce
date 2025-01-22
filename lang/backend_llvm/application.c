#include "backend_llvm/application.h"
#include "function.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
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

LLVMValueRef curry_callable_symbol(JITSymbol *sym, Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return NULL;
}
static LLVMValueRef call_callable(Ast *ast, Type *callable_type,
                                  LLVMValueRef callable, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  if (!callable) {
    return NULL;
  }

  int args_len = ast->data.AST_APPLICATION.len;

  LLVMTypeRef llvm_callable_type =
      type_to_llvm_type(callable_type, ctx->env, module);

  if (callable_type->kind == T_FN &&
      callable_type->data.T_FN.from->kind == T_VOID) {

    return LLVMBuildCall2(builder, llvm_callable_type, callable, NULL, 0,
                          "call_func");
  }

  LLVMValueRef app_vals[args_len];

  for (int i = 0; i < args_len; i++) {

    Type *expected_type = callable_type->data.T_FN.from;

    Ast *app_arg = ast->data.AST_APPLICATION.args + i;

    LLVMValueRef app_val;
    if (is_generic(app_arg->md) && expected_type->kind == T_FN) {
      Ast _app_arg = *app_arg;
      _app_arg.md = expected_type;
      app_val = codegen(&_app_arg, ctx, module, builder);
    } else {
      app_val = codegen(app_arg, ctx, module, builder);
    }

    callable_type = callable_type->data.T_FN.to;

    app_vals[i] = app_val;
  }

  LLVMValueRef res = LLVMBuildCall2(builder, llvm_callable_type, callable,
                                    app_vals, args_len, "call_func");
  return res;
}

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  if (ast->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
    LLVMValueRef callable =
        codegen(ast->data.AST_APPLICATION.function, ctx, module, builder);

    if (!callable) {
      fprintf(stderr, "Error: could not access record\n");
      return NULL;
    }

    return call_callable(ast, expected_fn_type, callable, ctx, module, builder);
  }

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (!sym) {
    fprintf(stderr, "Error callable symbol %s not found in scope %d\n",
            sym_name, ctx->stack_ptr);
    return NULL;
  }

  int args_len = ast->data.AST_APPLICATION.len;
  int expected_args_len = fn_type_args_len(sym->symbol_type);

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    if (sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {

      return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
          ast, ctx, module, builder);
    }

    LLVMValueRef callable =
        get_specific_callable(sym, expected_fn_type, ctx, module, builder);

    return call_callable(ast, expected_fn_type, callable, ctx, module, builder);
  }

  if (sym->type == STYPE_FUNCTION) {
    Type *callable_type = sym->symbol_type;

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);
    return res;
  }

  if (sym->type == STYPE_PARTIAL_EVAL_CLOSURE) {
    if (args_len +
            sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len ==
        sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len) {

      LLVMValueRef *provided_args =
          sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args;
      int provided_args_len =
          sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len;
      int total_len =
          sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len;

      Type *original_callable_type =
          sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type;

      JITSymbol *original_callable_sym =
          (JITSymbol *)sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym;

      LLVMValueRef full_args[total_len];

      for (int i = 0;
           i < sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len;
           i++) {
        full_args[i] = sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args[i];
      }

      for (int i = 0; i < args_len; i++) {
        full_args[sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE
                      .provided_args_len +
                  i] =
            codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);
      }

      printf("gathered args for callable curried\n");
      print_type(original_callable_type);
      print_type(expected_fn_type);
    }
  }

  return NULL;
}
