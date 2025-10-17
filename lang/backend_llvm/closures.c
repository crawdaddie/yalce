#include "./closures.h"
#include "../types/closures.h"
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

LLVMValueRef compile_closure_fn(Ast *lambda, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {}

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
  ast_func.md = ftype;
  return codegen(&ast_func, &_ctx, module, builder);
}

LLVMValueRef compile_curried_fn(Ast *expr, Type *expected_clos_type,
                                LLVMTypeRef closure_rec_type,
                                LLVMTypeRef clos_fn_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *clos_type = expr->md;
  clos_type = resolve_type_in_env(clos_type, ctx->env);

  Type *callable_type;
  LLVMTypeRef llvm_callable_type;
  LLVMValueRef callable_val;
  const char *fname;

  if (expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    fname = expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    JITSymbol *callable_sym =
        lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    if (!callable_sym) {
      fprintf(stderr, "Symbol to curry not found\n");
      return NULL;
    }

    Type *ftype = deep_copy_type(expr->data.AST_APPLICATION.function->md);
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
    callable_type = expr->data.AST_APPLICATION.function->md;
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
    args[i] = LLVMBuildLoad2(
        builder,

        type_to_llvm_type(recordt->data.T_CONS.args[i], &fn_ctx, module),

        LLVMBuildStructGEP2(builder, closure_rec_type, record, i + 1,
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

LLVMValueRef compile_lambda_as_closure(Ast *expr, Type *expected_clos_type,
                                       LLVMTypeRef closure_rec_type,
                                       LLVMTypeRef clos_fn_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  Type *clos_type = expr->md;
  clos_type = resolve_type_in_env(clos_type, ctx->env);

  Type *callable_type;

  START_FUNC(module, "curried_fn", clos_fn_type);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx);
  int len = fn_type_args_len(expected_clos_type);
  LLVMValueRef args[len + 1];
  Type *recordt = clos_type->closure_meta;
  LLVMValueRef record = LLVMGetParam(func, 0);

  int i = 0;
  for (AstList *closed_vals = expr->data.AST_LAMBDA.closed_vals; closed_vals;
       closed_vals = closed_vals->next, i++) {

    Ast *cl = closed_vals->ast;

    LLVMValueRef closed_val = LLVMBuildLoad2(
        builder,

        type_to_llvm_type(recordt->data.T_CONS.args[i], &fn_ctx, module),

        LLVMBuildStructGEP2(builder, closure_rec_type, record, i + 1,
                            "closure_record_val_ptr"),
        "closure_record_val");
    codegen_pattern_binding(cl, closed_val, recordt->data.T_CONS.args[i],
                            &fn_ctx, module, builder);
  }

  Type *ef = expected_clos_type;

  int j = 1;
  for (AstList *fn_params = expr->data.AST_LAMBDA.params; fn_params;
       fn_params = fn_params->next, ef = ef->data.T_FN.to, j++) {
    Ast *param = fn_params->ast;
    if (param->tag == AST_VOID) {
      continue;
    }
    LLVMValueRef param_val = LLVMGetParam(func, j);
  }

  LLVMValueRef body = codegen_lambda_body(expr, &fn_ctx, module, builder);

  if (fn_return_type(clos_type)->kind == T_VOID) {
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  END_FUNC;
  destroy_ctx(&fn_ctx);

  return func;
}

LLVMValueRef call_function_returning_closure() {}

LLVMValueRef store_closure_record_values(LLVMValueRef rec_alloc, Ast *expr,
                                         Type *rec_type, LLVMValueRef fn_ptr,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {

  // Convert the record type to LLVM type for GEP operations
  LLVMTypeRef llvm_rec_type = type_to_llvm_type(rec_type, ctx, module);

  // Store function pointer at offset 0
  LLVMValueRef fn_ptr_slot =
      LLVMBuildStructGEP2(builder, llvm_rec_type, rec_alloc, 0, "fn_ptr_slot");
  LLVMBuildStore(builder, fn_ptr, fn_ptr_slot);

  if (expr->tag == AST_APPLICATION) {
    for (int i = 0; i < expr->data.AST_APPLICATION.len; i++) {
      LLVMValueRef arg_val =
          codegen(expr->data.AST_APPLICATION.args + i, ctx, module, builder);

      LLVMValueRef field_slot = LLVMBuildStructGEP2(
          builder, llvm_rec_type, rec_alloc, i + 1, "app_arg_slot");
      LLVMBuildStore(builder, arg_val, field_slot);
    }

    return rec_alloc;
  }

  if (expr->tag == AST_LAMBDA) {
    int i = 0;
    for (AstList *closed_vals = expr->data.AST_LAMBDA.closed_vals; closed_vals;
         closed_vals = closed_vals->next, i++) {

      Ast *cl = closed_vals->ast;
      LLVMValueRef closed_val = codegen(cl, ctx, module, builder);

      LLVMValueRef field_slot = LLVMBuildStructGEP2(
          builder, llvm_rec_type, rec_alloc, i + 1, "closed_val_slot");
      LLVMBuildStore(builder, closed_val, field_slot);
    }

    return rec_alloc;
  }

  return NULL;
}

LLVMValueRef expr_to_closure_rec(Ast *expr, Type *clos_type, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef rec_type = closure_record_type(clos_type, ctx, module);
  LLVMTypeRef clos_fn_type = closure_fn_type(clos_type, rec_type, ctx, module);

  LLVMValueRef rec_storage;

  if (find_allocation_strategy(expr, ctx) == EA_STACK_ALLOC) {
    rec_storage = LLVMBuildAlloca(builder, rec_type, "closure_obj_alloc_stacc");
  } else {
    rec_storage = LLVMBuildMalloc(builder, rec_type, "closure_obj_alloc_heap");
  }

  if (expr->tag == AST_APPLICATION) {
    LLVMValueRef closure_fn = compile_curried_fn(
        expr, clos_type, rec_type, clos_fn_type, ctx, module, builder);

    rec_storage =
        store_closure_record_values(rec_storage, expr, clos_type->closure_meta,
                                    closure_fn, ctx, module, builder);
    return rec_storage;
  }

  if (expr->tag == AST_LAMBDA) {

    LLVMValueRef closure_fn = compile_lambda_as_closure(
        expr, clos_type, rec_type, clos_fn_type, ctx, module, builder);

    rec_storage =
        store_closure_record_values(rec_storage, expr, clos_type->closure_meta,
                                    closure_fn, ctx, module, builder);

    return rec_storage;
  }
  return NULL;
}

LLVMValueRef create_closure_symbol(Ast *binding, Ast *expr, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  Type *clos_type = expr->md;

  if (expr->tag == AST_APPLICATION && is_generic(clos_type)) {
    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, clos_type, NULL, NULL);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = expr;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame = ctx->frame;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env = ctx->env;

    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                sym);
    return NULL;
  }

  LLVMValueRef closure =
      expr_to_closure_rec(expr, clos_type, ctx, module, builder);

  if (!closure) {
    fprintf(stderr, "Error: could not compile closure obj\n");
    print_ast_err(expr);
    return NULL;
  }
  LLVMTypeRef llvm_closure_rec_type =
      closure_record_type(clos_type, ctx, module);

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, clos_type, closure,
                              LLVMPointerType(llvm_closure_rec_type, 0));

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);

  return sym->val;
}

LLVMTypeRef closure_record_type(Type *clos_type, JITLangCtx *ctx,
                                LLVMModuleRef module) {
  Type *obj_type = clos_type->closure_meta;
  int len = obj_type->data.T_CONS.num_args;

  LLVMTypeRef rec_members[len + 1];
  rec_members[0] = GENERIC_PTR;

  for (int i = 0; i < len; i++) {
    Type *mtype = obj_type->data.T_CONS.args[i];
    rec_members[i + 1] = mtype->kind == T_FN
                             ? GENERIC_PTR
                             : type_to_llvm_type(mtype, ctx, module);
  }

  LLVMTypeRef rec = LLVMStructType(rec_members, len + 1, 0);
  return rec;
}

LLVMTypeRef closure_fn_type(Type *clos_type, LLVMTypeRef closure_rec_type,
                            JITLangCtx *ctx, LLVMModuleRef module) {

  LLVMTypeRef closure_rec_ptr_type = LLVMPointerType(closure_rec_type, 0);
  if (is_void_func(clos_type)) {

    Type *ret_type = clos_type->data.T_FN.to;
    LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx, module);
    LLVMTypeRef ftype =
        LLVMFunctionType(llvm_ret_type, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
    return ftype;
  }

  int args_len = 1;

  Type *t = clos_type;

  while (!is_closure(t->data.T_FN.from) && t->kind == T_FN) {
    args_len++;
    t = t->data.T_FN.to;
  }

  LLVMTypeRef arg_types[args_len];
  arg_types[0] = closure_rec_ptr_type;
  Type *f = clos_type;

  for (int i = 1; i < args_len; i++, f = f->data.T_FN.to) {
    Type *at = f->data.T_FN.from;
    arg_types[i] =
        at->kind == T_FN ? GENERIC_PTR : type_to_llvm_type(at, ctx, module);
  }

  Type *ret_type = f;

  if (is_void_func(clos_type)) {
    ret_type = clos_type->data.T_FN.to;
  }

  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx, module);

  return LLVMFunctionType(llvm_ret_type, arg_types, args_len, 0);
}

LLVMValueRef codegen_curried_fn_closure(Type *original_fn_type, Ast *ast,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  Type *closure_type = ast->md;
  LLVMTypeRef rec_type = closure_record_type(closure_type, ctx, module);
  LLVMTypeRef fn_type = closure_fn_type(closure_type, rec_type, ctx, module);
  LLVMValueRef rec =
      expr_to_closure_rec(ast, closure_type, ctx, module, builder);

  return rec;
}

LLVMValueRef codegen_lambda_closure(Type *fn_type, Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  Type *rec_struct_type = fn_type->closure_meta;
  LLVMTypeRef rec_type = closure_record_type(fn_type, ctx, module);
  LLVMTypeRef llvm_clos_fn_type =
      closure_fn_type(fn_type, rec_type, ctx, module);

  char name[32];
  snprintf(name, 32, "lambda.closure.\\%s",
           ast->data.AST_LAMBDA.fn_name.chars
               ? ast->data.AST_LAMBDA.fn_name.chars
               : ast->data.AST_LAMBDA.params->ast->data.AST_IDENTIFIER.value);

  START_FUNC(module, name, llvm_clos_fn_type);
  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
  LLVMValueRef inner_closure_rec = LLVMGetParam(func, 0);

  Type *clos_type = fn_type->closure_meta;
  AST_LIST_ITER(ast->data.AST_LAMBDA.closed_vals, ({
                  // TODO - is the order here reversed? ie use n - i for the
                  // index instead
                  //
                  // ast->data.AST_LAMBDA.closed_vals is a list of vals in
                  // reverse order of usage
                  Ast *param_ast = l->ast;
                  // printf("closed val\n");
                  // print_ast(param_ast);
                  // print_type(param_ast->md);
                  // print_type_env(ctx->env);
                  // print_type(clos_type);
                  LLVMValueRef param_val =
                      LLVMBuildStructGEP2(builder, rec_type, inner_closure_rec,
                                          i + 1, "closed_val_from_rec");
                  Type *ptype = clos_type->data.T_CONS.args[i];
                  LLVMTypeRef llvm_ptype =
                      type_to_llvm_type(ptype, ctx, module);

                  param_val = LLVMBuildLoad2(builder, llvm_ptype, param_val,
                                             "load_closed_val_from_rec");

                  bind_fn_param(param_val, ptype, param_ast, ctx, &fn_ctx,
                                module, builder);
                }));

  AST_LIST_ITER(ast->data.AST_LAMBDA.params, ({
                  LLVMValueRef param_val = LLVMGetParam(func, i + 1);
                  Ast *param_ast = l->ast;
                  Type *param_type = fn_type->data.T_FN.from;

                  bind_fn_param(param_val, param_type, param_ast, ctx, &fn_ctx,
                                module, builder);

                  fn_type = fn_type->data.T_FN.to;
                }));

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  if (fn_type->kind == T_VOID) {
    // printf("build ret for some reason???\n");
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  END_FUNC
  destroy_ctx(&fn_ctx);

  LLVMValueRef rec_storage;
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    rec_storage = LLVMBuildAlloca(builder, rec_type, "closure_obj_alloc_stacc");
  } else {
    rec_storage = LLVMBuildMalloc(builder, rec_type, "closure_obj_alloc_heap");
  }

  int size = rec_struct_type->data.T_CONS.num_args + 1;
  // LLVMValueRef vals[size];
  // vals[0] = func;
  // AST_LIST_ITER(ast->data.AST_LAMBDA.closed_vals, ({
  //                 // TODO - is the order here reversed? ie use n - i for the
  //                 // index instead
  //                 //
  //                 // ast->data.AST_LAMBDA.closed_vals is a list of vals in
  //                 // reverse order of usage
  //                 Ast *param_ast = l->ast;
  //                 LLVMValueRef param_val =
  //                     codegen(param_ast, ctx, module, builder);
  //                 vals[i + 1] = param_val;
  //               }));
  // Store function pointer at offset 0
  LLVMValueRef fn_ptr_slot =
      LLVMBuildStructGEP2(builder, rec_type, rec_storage, 0, "fn_ptr_slot");
  LLVMBuildStore(builder, func, fn_ptr_slot);

  AST_LIST_ITER(ast->data.AST_LAMBDA.closed_vals, ({
                  Ast *param_ast = l->ast;
                  LLVMValueRef param_val =
                      codegen(param_ast, ctx, module, builder);

                  // Store at offset i+1 (offset 0 is the function pointer)
                  LLVMValueRef field_slot = LLVMBuildStructGEP2(
                      builder, rec_type, rec_storage, i + 1, "closed_val_slot");
                  LLVMBuildStore(builder, param_val, field_slot);
                }));

  return rec_storage;
}

LLVMValueRef codegen_const_curried_fn(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  Type *fn_type = ast->md;

  int fn_len = fn_type_args_len(fn_type);
  LLVMTypeRef prototype = codegen_fn_type(fn_type, fn_len, ctx, module);

  if (!prototype) {
    return NULL;
  }

  START_FUNC(module, "curried_fn_with_const_params", prototype)

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  Type *inner_fn_type = ast->data.AST_APPLICATION.function->md;
  inner_fn_type = resolve_type_in_env(inner_fn_type, ctx->env);
  int inner_args_len = fn_type_args_len(inner_fn_type);

  LLVMValueRef inner_args[inner_args_len];
  Type *f = inner_fn_type;
  int i = 0;
  for (i = 0; i < ast->data.AST_APPLICATION.len; i++, f = f->data.T_FN.to) {
    inner_args[i] =
        codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);
    inner_args[i] = handle_type_conversions(
        inner_args[i], ast->data.AST_APPLICATION.args[i].md, f->data.T_FN.from,
        ctx, module, builder);
  }

  for (i = 0; i < fn_len; i++) {
    inner_args[inner_args_len - 1 + i] = LLVMGetParam(func, i);
  }

  LLVMValueRef inner_fn =
      codegen(ast->data.AST_APPLICATION.function, ctx, module, builder);

  LLVMTypeRef llvm_inner_fn_type =
      type_to_llvm_type(inner_fn_type, ctx, module);

  LLVMValueRef body =
      LLVMBuildCall2(builder, llvm_inner_fn_type, inner_fn, inner_args,
                     inner_args_len, "curried_fn_inner_call");

  Type *res_type = fn_return_type(fn_type);
  if (res_type->kind == T_VOID) {
    // printf("build ret for some reason???\n");
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  END_FUNC
  destroy_ctx(&fn_ctx);

  // LLVMDumpValue(func);
  // printf("\n");
  return func;
}

LLVMValueRef codegen_create_closure(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  Type *fn_type;

  if (ast->tag == AST_APPLICATION) {
    fn_type = ast->data.AST_APPLICATION.function->md;
    return codegen_curried_fn_closure(fn_type, ast, ctx, module, builder);
  }

  if (ast->tag == AST_LAMBDA) {
    fn_type = ast->md;
    return codegen_lambda_closure(fn_type, ast, ctx, module, builder);
  }

  return NULL;
}

bool is_lambda_with_closures(Ast *ast) {
  return ast->tag == AST_LAMBDA &&
         (ast->data.AST_LAMBDA.num_closure_free_vars > 0);
}

void add_recursive_closure_ref(ObjString fn_name, LLVMValueRef func,
                               Type *fn_type, JITLangCtx *fn_ctx) {}

LLVMValueRef call_closure_rec(LLVMValueRef rec, Type *closure_type, Ast *app,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  int num_args = fn_type_args_len(closure_type);

  LLVMTypeRef rec_type = closure_record_type(closure_type, ctx, module);
  LLVMTypeRef clos_fn_type =
      closure_fn_type(closure_type, rec_type, ctx, module);

  LLVMValueRef fn =
      LLVMBuildStructGEP2(builder, rec_type, rec, 0, "fn_ptr_gep");
  fn = LLVMBuildLoad2(builder, GENERIC_PTR, fn,
                      "fn_ptr"); // extract from rec as just generic ptr
  //
  LLVMTypeRef fn_type = clos_fn_type;
  LLVMValueRef args[num_args + 1];

  args[0] = rec;
  Type *ff = closure_type;

  for (int i = 0; i < num_args; i++, ff = ff->data.T_FN.to) {
    Type *arg_type = deep_copy_type(app->data.AST_APPLICATION.args[i].md);
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

LLVMValueRef call_generic_closure_sym(Ast *app, Type *expected_fn_type,
                                      JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMValueRef closure = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type);

  if (!closure) {

    Ast expr = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    // no closure found, create new
    TICtx _ctx = {};

    unify(sym->symbol_type, expected_fn_type, &_ctx);
    Subst *subst = solve_constraints(_ctx.constraints);
    Type *closure_type = deep_copy_type(sym->symbol_type);
    closure_type = expected_fn_type;

    JITLangCtx compilation_ctx = *ctx;
    compilation_ctx.stack_ptr =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;
    TypeEnv *env = sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env;
    env = create_env_from_subst(env, subst);
    compilation_ctx.env = env;
    closure = expr_to_closure_rec(&expr, closure_type, &compilation_ctx, module,
                                  builder);

    if (!closure) {
      fprintf(
          stderr,
          "Error: could not compile specific instance of generic closure\n");
      print_ast_err(app);
      return NULL;
    }

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns = specific_fns_extend(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type,
        closure);
  }

  return call_closure_rec(closure, expected_fn_type, app, ctx, module, builder);
}

LLVMValueRef call_closure_sym(Ast *app, Type *expected_fn_type, JITSymbol *sym,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return call_generic_closure_sym(app, expected_fn_type, sym, ctx, module,
                                    builder);
  }

  Closure closure;

  return call_closure_rec(sym->val, sym->symbol_type, app, ctx, module,
                          builder);
}
