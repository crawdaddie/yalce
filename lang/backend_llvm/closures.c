#include "./closures.h"
#include "application.h"
#include "binding.h"
#include "call_lowering.h"
#include "function.h"
#include "serde.h"
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

    if (is_generic(callable_sym->symbol_type)) {

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

static LLVMValueRef build_curried_env(Ast *expr, Type *clos_type,
                                      LLVMTypeRef closure_rec_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  Type *recordt = clos_type->closure_meta;
  if (!recordt) {
    fprintf(stderr, "Error: curried function has no closure metadata\n");
    return NULL;
  }

  LLVMValueRef env = LLVMBuildMalloc(builder, closure_rec_type, "curried_env");

  FlatApplication flat = flatten_application(expr);
  int num_curried = recordt->data.T_CONS.num_args;

  for (int idx = 0; idx < num_curried; idx++) {
    Type *field_type = recordt->data.T_CONS.args[idx];
    Ast *arg_ast = flat.args[idx];

    LLVMValueRef val = codegen(arg_ast, ctx, module, builder);
    if (!val) {
      fprintf(stderr, "Error: could not codegen curried arg %d\n", idx);
      free_flat_application(&flat);
      return NULL;
    }

    Type *arg_type = specialize_type_for_codegen(arg_ast->type, ctx);
    Type *expected_field_type = specialize_type_for_codegen(field_type, ctx);
    val = handle_type_conversions(val, arg_type, expected_field_type, ctx,
                                  module, builder);

    LLVMValueRef field_ptr = LLVMBuildStructGEP2(builder, closure_rec_type, env,
                                                 idx, "curried_env_field_ptr");
    LLVMBuildStore(builder, val, field_ptr);
  }

  free_flat_application(&flat);
  return env;
}

LLVMValueRef call_generic_closure_sym(Ast *app, Type *expected_fn_type,
                                      JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  LLVMValueRef closure = NULL;
  Type *original_type = deep_copy_type(sym->symbol_type);
  Type *expected_type = deep_copy_type(expected_fn_type);

  TypeEnv *env = sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env;

  JITLangCtx compilation_ctx = *ctx;
  compilation_ctx.type_subst =
      create_subst_for_generic_fn(original_type, expected_type);

  compilation_ctx.env = create_env_from_subst(env, compilation_ctx.type_subst);

  Type *clos_type = NULL;

  if (sym->symbol_data.STYPE_GENERIC_FUNCTION.ast->tag == AST_APPLICATION) {

    Ast *generic_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    clos_type = deep_copy_type(generic_ast->type);
    clos_type = resolve_type_in_env(clos_type, compilation_ctx.env);

    LLVMTypeRef llvm_rec_type =
        closure_record_type(clos_type, &compilation_ctx, module);
    LLVMTypeRef llvm_fn_type =
        closure_fn_type(clos_type, llvm_rec_type, &compilation_ctx, module);

    LLVMValueRef wrapper = specific_fns_lookup(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, clos_type);

    if (!wrapper) {
      wrapper =
          compile_curried_fn(generic_ast, expected_type, llvm_rec_type,
                             llvm_fn_type, &compilation_ctx, module, builder);
      if (!wrapper) {
        fprintf(stderr, "Error: could not compile curried wrapper\n");
        return NULL;
      }

      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
          specific_fns_extend(
              sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, clos_type,
              wrapper);
    }

    LLVMValueRef env_val =
        build_curried_env(generic_ast, clos_type, llvm_rec_type,
                          &compilation_ctx, module, builder);
    if (!env_val) {
      return NULL;
    }

    LLVMTypeRef closure_struct_type =
        type_to_llvm_type(clos_type, &compilation_ctx, module);
    LLVMValueRef fn_ptr =
        LLVMBuildBitCast(builder, wrapper, GENERIC_PTR, "curried_fn_ptr");

    closure = LLVMGetUndef(closure_struct_type);
    closure = LLVMBuildInsertValue(builder, closure, fn_ptr, 0, "closure_fn");
    closure = LLVMBuildInsertValue(builder, closure, env_val, 1, "closure_env");
  }

  if (!closure) {
    fprintf(stderr, "Error: could not compile specific instance of generic "
                    "closure\n");
    print_ast_err(app);
    return NULL;
  }

  return call_callable(app, clos_type ? clos_type : expected_fn_type, closure,
                       ctx, module, builder);
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
  if (!clos_type || clos_type->kind != T_FN) {
    fprintf(stderr, "Error: closure_fn_type expected function type\n");
    return NULL;
  }

  int num_params = fn_type_args_len(clos_type);

  LLVMTypeRef param_types[num_params + 1];
  param_types[0] = LLVMPointerType(rec_type, 0);

  Type *f = clos_type;
  for (int i = 0; i < num_params; i++, f = f->data.T_FN.to) {
    Type *param_type = f->data.T_FN.from;

    if (is_closure(param_type)) {
      param_types[i + 1] = type_to_llvm_type(param_type, ctx, module);
    } else if (param_type->kind == T_FN) {
      param_types[i + 1] = GENERIC_PTR;
    } else {
      param_types[i + 1] = type_to_llvm_type(param_type, ctx, module);
    }
  }

  Type *return_type = f;
  LLVMTypeRef ret_type = type_to_llvm_type(return_type, ctx, module);
  if (!ret_type) {
    return NULL;
  }

  return LLVMFunctionType(ret_type, param_types, num_params + 1, 0);
}

LLVMTypeRef closure_record_type(Type *clos_type, JITLangCtx *ctx,
                                LLVMModuleRef module) {
  if (!clos_type || !clos_type->closure_meta) {
    return LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);
  }

  Type *meta = clos_type->closure_meta;
  int num_fields = meta->data.T_CONS.num_args;
  LLVMTypeRef field_types[num_fields];

  for (int i = 0; i < num_fields; i++) {
    Type *field_type = meta->data.T_CONS.args[i];

    if (field_type->kind == T_FN && !is_closure(field_type)) {
      field_types[i] = GENERIC_PTR;
    } else if (is_closure(field_type)) {
      field_types[i] = type_to_llvm_type(field_type, ctx, module);
    } else {
      field_types[i] = type_to_llvm_type(field_type, ctx, module);
    }
  }

  return LLVMStructType(field_types, num_fields, 0);
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
