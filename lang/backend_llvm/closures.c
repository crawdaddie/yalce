#include "./closures.h"
#include "application.h"
#include "serde.h"
#include "symbols.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <stdlib.h>

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef compile_closure_fn(Ast *lambda, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {}
#include "./closures.h"
#include "application.h"
#include "binding.h"
#include "function.h"
#include "symbols.h"
#include "types.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef find_callable_from_generic(Ast *expr, Type *callable_type,
                                        Type *ftype, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  TICtx tctx = {};
  unify(callable_type, ftype, &tctx);
  Subst *subst = solve_constraints(tctx.constraints);
  TypeEnv *new_env = create_env_from_subst(ctx->env, subst);
  JITLangCtx _ctx = *ctx;
  _ctx.env = new_env;
  Ast ast_func = *expr->data.AST_APPLICATION.function;
  ast_func.type = ftype;
  return codegen(&ast_func, &_ctx, module, builder);
}

static bool closure_env_should_stack_alloc(Ast *expr, JITLangCtx *ctx) {
  return find_allocation_strategy(expr, ctx) == EA_STACK_ALLOC;
}

LLVMValueRef call_closure_obj(LLVMValueRef rec, Type *closure_type, Ast *app,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  // printf("CALL closure obj\n");
  // print_ast(app);

  int num_args = fn_type_args_len(closure_type);
  // printf("num args %d\n", num_args);

  LLVMTypeRef env_type = closure_record_type(closure_type, ctx, module);
  LLVMTypeRef clos_fn_type =
      closure_fn_type(closure_type, env_type, ctx, module);

  LLVMValueRef fn = LLVMBuildExtractValue(builder, rec, 0, "fn");
  LLVMTypeRef fn_type = clos_fn_type;
  LLVMValueRef args[num_args + 1];
  LLVMValueRef rec_env = LLVMBuildExtractValue(builder, rec, 1, "env");

  Type *ff = closure_type;

  if (ff->data.T_FN.from->kind == T_VOID &&
      app->data.AST_APPLICATION.args->tag == AST_VOID) {

    LLVMValueRef call = LLVMBuildCall2(
        builder, fn_type, fn, (LLVMValueRef[]){rec_env}, 1, "call_closure");
    return call;
  }

  // return call_callable(app, closure_type, );

  args[0] = rec_env;

  for (int i = 0; i < num_args; i++, ff = ff->data.T_FN.to) {
    Type *arg_type = deep_copy_type(app->data.AST_APPLICATION.args[i].type);
    arg_type = resolve_type_in_env(arg_type, ctx->env);
    Type *expected_type = ff->data.T_FN.from;

    args[i + 1] =
        codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);

    args[i + 1] = handle_type_conversions(args[i + 1], arg_type, expected_type,
                                          ctx, module, builder);
  }
  LLVMValueRef call =
      LLVMBuildCall2(builder, fn_type, fn, args, num_args + 1, "call_closure");

  return call;
}

LLVMValueRef compile_curried_fn(Ast *expr, Type *expected_clos_type,
                                LLVMTypeRef closure_rec_type,
                                LLVMTypeRef clos_fn_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *clos_type = expr->type;
  clos_type = resolve_type_in_env(clos_type, ctx->env);

  Type *callable_type;
  LLVMTypeRef llvm_callable_type;
  LLVMValueRef callable_val;
  const char *fname;

  if (expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    fname = expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    JITSymbol *callable_sym =
        lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    // if (callable_sym->type == STYPE_GENERIC_FUNCTION &&
    //     callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
    //
    //   return
    //   callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
    //       expr, ctx, module, builder);
    // }

    if (!callable_sym) {
      fprintf(stderr, "Symbol to curry not found\n");
      return NULL;
    }

    Type *ftype = deep_copy_type(expr->data.AST_APPLICATION.function->type);
    ftype = resolve_type_in_env(ftype, ctx->env);
    callable_type = ftype;
    llvm_callable_type = type_to_llvm_type(callable_type, ctx, module);

    if (is_generic(callable_sym->symbol_type) ||
        callable_sym->symbol_type->kind == T_SCHEME) {
      callable_val = find_callable_from_generic(expr, callable_type, ftype, ctx,
                                                module, builder);
    } else {
      callable_val = callable_sym->val;
    }

  } else if (expr->data.AST_APPLICATION.function->tag == AST_LAMBDA) {

    fname = expr->data.AST_APPLICATION.function->data.AST_LAMBDA.fn_name.chars;
    callable_type = expr->data.AST_APPLICATION.function->type;
    llvm_callable_type = type_to_llvm_type(callable_type, ctx, module);
    callable_val = codegen(expr, ctx, module, builder);
  } else {
    fprintf(stderr, "Could not find callable val\n");
    return NULL;
  }

  char name[32];
  snprintf(name, 32, "curried.%s", fname);
  START_FUNC(module, name, clos_fn_type);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx);
  int len = fn_type_args_len(callable_type);
  LLVMValueRef args[len];
  Type *recordt = clos_type->closure_meta;
  LLVMValueRef record = LLVMGetParam(func, 0);

  int i;
  for (i = 0; i < recordt->data.T_CONS.num_args; i++) {
    Type *rt = recordt->data.T_CONS.args[i];

    // printf("record arg %d: ", i);
    LLVMTypeRef lt = type_to_llvm_type(rt, &fn_ctx, module);

    // print_type(recordt->data.T_CONS.args[i]);
    // LLVMDumpType(lt);
    // printf("\n");

    args[i] = LLVMBuildLoad2(
        builder, rt->kind == T_FN && (!is_closure(rt)) ? GENERIC_PTR : lt,
        LLVMBuildStructGEP2(builder, closure_rec_type, record, i,
                            "closure_record_val_ptr"),
        "closure_record_val");
  }

  Type *ef = expected_clos_type;
  Type *f = clos_type;

  for (int j = 0; i < len;
       i++, j++, ef = ef->data.T_FN.to, f = f->data.T_FN.to) {

    args[i] = LLVMGetParam(func, 1 + j);
    args[i] = handle_type_conversions(args[i], ef->data.T_FN.from,
                                      f->data.T_FN.from, ctx, module, builder);
  }

  LLVMValueRef body = LLVMBuildCall2(builder, llvm_callable_type, callable_val,
                                     args, len, "curried_fn_call");

  if (fn_return_type(callable_type)->kind == T_VOID) {
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  END_FUNC;
  destroy_ctx(&fn_ctx);

  return func;
}
LLVMValueRef call_generic_closure_sym(Ast *app, Type *expected_fn_type,
                                      JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  LLVMValueRef closure;
  printf("call closure sym\n");
  print_ast(app);
  printf("current callable: ");
  print_type(expected_fn_type);
  printf("original callable: ");
  print_type(sym->symbol_type);
  print_ast(sym->symbol_data.STYPE_GENERIC_FUNCTION.ast);

  // if (sym->symbol_data.STYPE_GENERIC_FUNCTION.ast->tag == AST_APPLICATION) {
  //   closure = compile_curried_fn(sym->symbol_data.STYPE_GENERIC_FUNCTION.ast,
  //   sym->symbol_type->closure_meta, );
  // }

  if (!closure) {
    fprintf(stderr, "Error: could not compile specific instance of generic "
                    "closure\n");
    print_ast_err(app);
    return NULL;
  }

  return call_callable(app, expected_fn_type, closure, ctx, module, builder);
}

LLVMValueRef call_closure_sym(Ast *app, Type *expected_fn_type, JITSymbol *sym,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  Type *call_type = expected_fn_type;

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return call_generic_closure_sym(app, call_type, sym, ctx, module, builder);
  }

  return call_callable(app, call_type, sym->val, ctx, module, builder);
}

LLVMTypeRef closure_fn_type(Type *clos_type, LLVMTypeRef rec_type,
                            JITLangCtx *ctx, LLVMModuleRef module) {
  return NULL;
}

LLVMTypeRef closure_record_type(Type *clos_type, JITLangCtx *ctx,
                                LLVMModuleRef module) {
  return NULL;
}

LLVMValueRef codegen_const_curried_fn(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return NULL;
}
LLVMValueRef codegen_create_closure(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  if (ast->tag == AST_APPLICATION &&
      ast->data.AST_APPLICATION.is_curried_with_constants) {
    return codegen_const_curried_fn(ast, ctx, module, builder);
  }
  return NULL;
}
