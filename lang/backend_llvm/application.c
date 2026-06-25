#include "backend_llvm/application.h"
#include "./coroutines/coroutines.h"
#include "adt.h"
#include "backend_llvm/call_lowering.h"
#include "closures.h"
#include "coroutines/coroutine_extensions.h"
#include "function.h"
#include "function_extern.h"
#include "modules.h"
#include "symbols.h"
#include "types.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static Ast *maybe_optimise_application(Ast *ast) {
  if (!is_coroutine_type(ast->type)) {
    return ast;
  }

  Ast *optimised = optimise_coro_combinators(ast);
  return optimised ? optimised : ast;
}

static bool is_non_module_record_member_call(Ast *app) {
  Ast *fn = app->data.AST_APPLICATION.function;
  return fn->tag == AST_RECORD_ACCESS &&
         !is_module_ast(fn->data.AST_RECORD_ACCESS.record);
}

static const char *application_symbol_name(Ast *app) {
  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn->tag == AST_IDENTIFIER) {
    return fn->data.AST_IDENTIFIER.value;
  }

  while (fn->tag == AST_RECORD_ACCESS) {
    fn = fn->data.AST_RECORD_ACCESS.member;
  }

  if (fn->tag == AST_IDENTIFIER) {
    return fn->data.AST_IDENTIFIER.value;
  }

  return "";
}

static void refine_callable_type_with_closure_args(Type *callable_type,
                                                   Ast *app, JITLangCtx *ctx) {
  Type *fn_cursor = callable_type;
  for (int i = 0; fn_cursor && fn_cursor->kind == T_FN &&
                  i < app->data.AST_APPLICATION.len;
       i++, fn_cursor = fn_cursor->data.T_FN.to) {
    Ast *arg = app->data.AST_APPLICATION.args + i;
    if (arg->tag != AST_IDENTIFIER) {
      continue;
    }

    JITSymbol *sym = lookup_id_ast(arg, ctx);
    if (sym && sym->type == STYPE_FUNCTION && is_closure(sym->symbol_type)) {
      fn_cursor->data.T_FN.from = sym->symbol_type;
    }
  }
}

static Type *resolve_expected_callable_type(Ast *app, JITLangCtx *ctx) {
  Type *callable_type = app->data.AST_APPLICATION.function->type;
  callable_type = specialize_type_for_codegen(callable_type, ctx);
  refine_callable_type_with_closure_args(callable_type, app, ctx);
  return callable_type;
}

static JITSymbol *lookup_application_symbol(Ast *app, JITLangCtx *ctx) {
  JITSymbol *sym = lookup_id_ast(app->data.AST_APPLICATION.function, ctx);
  if (sym) {
    return sym;
  }

  char buf[128];
  ast_to_sexpr(app->data.AST_APPLICATION.function, buf);
  fprintf(stderr, "Error callable symbol `%s` not found in scope %d\n", buf,
          ctx->stack_ptr);
  print_location(app->data.AST_APPLICATION.function);
  return NULL;
}

// static void attach_symbol_closure_meta(Type *callable_type, JITSymbol *sym) {
//   if (!callable_type || !sym || sym->type != STYPE_FUNCTION ||
//       is_closure(callable_type) || !is_closure(sym->symbol_type)) {
//     return;
//   }
//
//   callable_type->closure_meta = sym->symbol_type->closure_meta;
// }

static bool is_closure_symbol(JITSymbol *sym) {
  return sym->symbol_type && sym->symbol_type->closure_meta != NULL;
}

static Type *resolve_sym_type(Type *exp, Type *sym_type, JITLangCtx *ctx) {
  TICtx ti_ctx = {};
  Type *exp_copy = deep_copy_type(exp);
  Type *sym_copy = deep_copy_type(sym_type);
  unify(exp_copy, sym_copy, &ti_ctx);
  Subst *subst = solve_constraints(ti_ctx.constraints);
  JITLangCtx spec_ctx = *ctx;
  spec_ctx.type_subst = subst;
  spec_ctx.env = create_env_from_subst(ctx->env, spec_ctx.type_subst);
  return specialize_type_for_codegen(sym_copy, &spec_ctx);
}

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (is_pointer_type(to_type) && to_type->data.T_CONS.num_args == 1 &&
      types_equal(to_type->data.T_CONS.args[0], from_type)) {
    LLVMTypeRef lvft = type_to_llvm_type(from_type, ctx, module);
    LLVMValueRef alloca = LLVMBuildAlloca(builder, lvft, "tmp_alloca");
    LLVMBuildStore(builder, val, alloca);
    return alloca;
  }

  if (to_type->constructor) {
    ConsMethod constructor = to_type->constructor;
    return constructor(val, from_type, module, builder);
  }

  return val;
}

LLVMValueRef call_callable(Ast *ast, Type *callable_type, LLVMValueRef callable,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  if (!callable) {
    fprintf(stderr, "Error: callable not found for\n");
    print_ast_err(ast->data.AST_APPLICATION.function);
    print_location(ast);
    return NULL;
  }

  LoweredCallable lowered =
      lower_callable_value(callable, callable_type, ctx, module, builder);

  Ast app = *ast;
  if (ast->data.AST_APPLICATION.args->tag == AST_VOID) {
    app.data.AST_APPLICATION.len = 0;
  }

  return emit_lowered_call(&app, lowered, callable_type, ctx, module, builder);
}

static LLVMValueRef call_record_member(Ast *app, Type *callable_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  LLVMValueRef callable =
      codegen(app->data.AST_APPLICATION.function, ctx, module, builder);
  return call_callable(app, callable_type, callable, ctx, module, builder);
}

static LLVMValueRef call_variant_constructor(Ast *app, Type *callable_type,
                                             JITSymbol *sym, JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  return codegen_adt_member_with_args(callable_type, sym->llvm_type, app,
                                      application_symbol_name(app), ctx, module,
                                      builder);
}

static LLVMValueRef call_builtin_handler(Ast *app, JITSymbol *sym,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
      app, ctx, module, builder);
}

static LLVMValueRef call_generic_function(Ast *app, Type *expected_fn_type,
                                          JITSymbol *sym, JITLangCtx *ctx,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder) {
  Type *callable_type =
      resolve_sym_type(expected_fn_type, sym->symbol_type, ctx);
  LLVMValueRef callable =
      get_specific_callable(sym, callable_type, ctx, module, builder);
  return call_callable(app, callable_type, callable, ctx, module, builder);
}

static LLVMValueRef call_generic_constructor(Ast *app, Type *expected_fn_type,
                                             JITSymbol *sym, JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  Type *from_type = app->data.AST_APPLICATION.args->type;
  Type exp = (Type){
      T_FN, .data = {.T_FN = {.from = from_type, .to = expected_fn_type}}};
  LLVMValueRef callable =
      get_specific_callable(sym, &exp, ctx, module, builder);
  return call_callable(app, &exp, callable, ctx, module, builder);
}

static LLVMValueRef call_direct_symbol(Ast *app, JITSymbol *sym,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return call_callable(app, sym->symbol_type, sym->val, ctx, module, builder);
}

static LLVMValueRef call_lazy_extern_symbol(Ast *app, JITSymbol *sym,
                                            JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  LLVMValueRef callable = instantiate_extern_fn_sym(sym, ctx, module, builder);
  return call_callable(app, sym->symbol_type, callable, ctx, module, builder);
}

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  ast = maybe_optimise_application(ast);

  if (is_closure(ast->type) && application_is_partial(ast)) {
    return codegen_create_closure(ast, ctx, module, builder);
  }

  Type *callable_type = resolve_expected_callable_type(ast, ctx);

  if (is_non_module_record_member_call(ast)) {
    return call_record_member(ast, callable_type, ctx, module, builder);
  }

  JITSymbol *sym = lookup_application_symbol(ast, ctx);
  if (!sym) {
    return NULL;
  }

  if (is_closure_symbol(sym)) {
    return call_closure_sym(ast, callable_type, sym, ctx, module, builder);
  }

  if (sym->type >= STYPE_GENERIC_FUNCTION &&
      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
    return call_builtin_handler(ast, sym, ctx, module, builder);
  }

  if (sym->type == STYPE_VARIANT_TYPE) {
    return call_variant_constructor(ast, callable_type, sym, ctx, module,
                                    builder);
  }

  if (sym->symbol_type && is_coroutine_constructor_type(sym->symbol_type)) {
    return coro_create_with_reset_closure(sym, callable_type, ast, ctx, module,
                                          builder);
  }

  if (sym->symbol_type && is_coroutine_type(sym->symbol_type)) {
    return coro_symbol_resume(sym, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_FUNCTION &&
      !(sym->symbol_type && is_closure(sym->symbol_type))) {
    return call_generic_function(ast, callable_type, sym, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_CONSTRUCTOR) {
    return call_generic_constructor(ast, callable_type, sym, ctx, module,
                                    builder);
  }

  if (sym->type == STYPE_FUNCTION) {
    return call_direct_symbol(ast, sym, ctx, module, builder);
  }

  if (sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    return call_lazy_extern_symbol(ast, sym, ctx, module, builder);
  }

  return NULL;
}
