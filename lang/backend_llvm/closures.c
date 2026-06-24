#include "./closures.h"
#include "application.h"
#include "binding.h"
#include "call_lowering.h"
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

LLVMValueRef materialize_generic_closure_value(JITSymbol *sym,
                                               Type *expected_fn_type,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder);

static JITSymbol *lookup_application_callable_symbol(Ast *expr,
                                                     JITLangCtx *ctx);

static Type *resolve_application_callable_type(Ast *expr, JITLangCtx *ctx) {
  if (!expr || expr->tag != AST_APPLICATION ||
      !expr->data.AST_APPLICATION.function) {
    return NULL;
  }

  JITSymbol *sym = lookup_application_callable_symbol(expr, ctx);
  Type *callable_type = expr->data.AST_APPLICATION.function->type;

  if (sym && sym->symbol_type) {
    bool is_captured_closure =
        (sym->type == STYPE_FUNCTION &&
         sym->symbol_data.STYPE_FUNCTION.closure_env_type != NULL) ||
        (sym->type == STYPE_GENERIC_FUNCTION &&
         sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type != NULL);
    if (is_captured_closure) {
      callable_type = sym->symbol_type;
    }
  }
  if (!callable_type) {
    return NULL;
  }

  return specialize_type_for_codegen(callable_type, ctx);
}

static JITSymbol *lookup_application_callable_symbol(Ast *expr,
                                                     JITLangCtx *ctx) {
  if (!expr || expr->tag != AST_APPLICATION ||
      !expr->data.AST_APPLICATION.function || !ctx) {
    return NULL;
  }

  return lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);
}

static bool application_function_needs_capture(Ast *expr, JITLangCtx *ctx) {
  JITSymbol *sym = lookup_application_callable_symbol(expr, ctx);
  if (!sym) {
    return false;
  }

  if (sym->type == STYPE_FUNCTION) {
    return sym->symbol_data.STYPE_FUNCTION.closure_env_type != NULL;
  }

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type != NULL;
  }

  return false;
}

static Type *application_function_capture_type(Ast *expr, JITLangCtx *ctx) {
  Type *capture_type = resolve_application_callable_type(expr, ctx);
  JITSymbol *sym = lookup_application_callable_symbol(expr, ctx);

  if (!capture_type || !sym) {
    return capture_type;
  }

  Type *closure_env_type = NULL;
  if (sym->type == STYPE_FUNCTION) {
    closure_env_type = sym->symbol_data.STYPE_FUNCTION.closure_env_type;
  } else if (sym->type == STYPE_GENERIC_FUNCTION) {
    closure_env_type = sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type;
  }

  if (!closure_env_type) {
    return capture_type;
  }

  capture_type = deep_copy_type(capture_type);
  capture_type->closure_meta = closure_env_type;
  return capture_type;
}

static Type *curried_closure_env_type(Ast *expr, JITLangCtx *ctx) {
  if (!expr || expr->tag != AST_APPLICATION || expr->type->kind != T_FN) {
    return NULL;
  }

  int len = (int)expr->data.AST_APPLICATION.len;
  if (len <= 0) {
    return NULL;
  }

  bool capture_callable = application_function_needs_capture(expr, ctx);
  int env_len = len + (capture_callable ? 1 : 0);
  Type **cl_vals = t_alloc(sizeof(Type *) * (size_t)env_len);
  Type *ftype = resolve_application_callable_type(expr, ctx);
  int dst_idx = 0;
  if (capture_callable) {
    cl_vals[dst_idx++] = application_function_capture_type(expr, ctx);
  }
  for (int i = 0; i < len; i++) {
    if (!ftype || ftype->kind != T_FN) {
      return NULL;
    }
    cl_vals[dst_idx++] = ftype->data.T_FN.from;
    ftype = ftype->data.T_FN.to;
  }

  return create_tuple_type(env_len, cl_vals);
}

static Type *lambda_closure_env_type(Ast *expr) {
  if (!expr || expr->tag != AST_LAMBDA) {
    return NULL;
  }

  int len = expr->data.AST_LAMBDA.num_closed_vals;
  if (len <= 0) {
    return NULL;
  }

  Type **closed_types = t_alloc(sizeof(Type *) * (size_t)len);
  int i = 0;
  for (AstList *closed_vals = expr->data.AST_LAMBDA.closed_vals; closed_vals;
       closed_vals = closed_vals->next, i++) {
    closed_types[i] = closed_vals->ast->type;
  }
  return create_tuple_type(len, closed_types);
}

static Type *closure_env_type_from_expr(Ast *expr, JITLangCtx *ctx) {
  if (!expr) {
    return NULL;
  }
  if (expr->tag == AST_APPLICATION) {
    return curried_closure_env_type(expr, ctx);
  }
  if (expr->tag == AST_LAMBDA) {
    return lambda_closure_env_type(expr);
  }
  return NULL;
}

static LLVMTypeRef closure_record_type_from_env(Type *obj_type, JITLangCtx *ctx,
                                                LLVMModuleRef module) {
  if (!obj_type) {
    return LLVMStructType(NULL, 0, 0);
  }

  int len = obj_type->data.T_CONS.num_args;
  LLVMTypeRef rec_members[len ? len : 1];

  for (int i = 0; i < len; i++) {
    Type *mtype = obj_type->data.T_CONS.args[i];
    rec_members[i] = mtype->kind == T_FN && !is_closure(mtype)
                         ? GENERIC_PTR
                         : type_to_llvm_type(mtype, ctx, module);
  }

  return LLVMStructType(rec_members, len, 0);
}

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

LLVMValueRef compile_curried_fn(Ast *expr, Type *expected_clos_type,
                                Type *recordt, LLVMTypeRef closure_rec_type,
                                LLVMTypeRef clos_fn_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *clos_type = expr->type;
  clos_type = resolve_type_in_env(clos_type, ctx->env);
  bool capture_callable = application_function_needs_capture(expr, ctx);

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

    Type *ftype = resolve_application_callable_type(expr, ctx);
    callable_type = ftype;
    llvm_callable_type = type_to_llvm_type(callable_type, ctx, module);

    if (callable_sym->type == STYPE_GENERIC_FUNCTION &&
        callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type) {
      callable_val = materialize_generic_closure_value(
          callable_sym, callable_type, ctx, module, builder);
    } else if (is_generic(callable_sym->symbol_type) ||
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
  Type *captured_callable_type = NULL;
  if (capture_callable) {
    captured_callable_type = application_function_capture_type(expr, ctx);
    LLVMTypeRef llvm_captured_callable_type =
        type_to_llvm_type(captured_callable_type, &fn_ctx, module);
    callable_val = LLVMBuildLoad2(builder, llvm_captured_callable_type,
                                  LLVMBuildStructGEP2(builder, closure_rec_type,
                                                      LLVMGetParam(func, 0), 0,
                                                      "captured_callable_ptr"),
                                  "captured_callable");
  }
  int len = fn_type_args_len(callable_type);
  LLVMValueRef args[len];
  LLVMValueRef record = LLVMGetParam(func, 0);

  int i;
  int record_len = recordt ? recordt->data.T_CONS.num_args : 0;
  int record_arg_start = capture_callable ? 1 : 0;
  for (i = 0; i < record_len - record_arg_start; i++) {
    Type *rt = recordt->data.T_CONS.args[i + record_arg_start];

    LLVMTypeRef lt = type_to_llvm_type(rt, &fn_ctx, module);

    args[i] = LLVMBuildLoad2(
        builder, rt->kind == T_FN && (!is_closure(rt)) ? GENERIC_PTR : lt,
        LLVMBuildStructGEP2(builder, closure_rec_type, record,
                            i + record_arg_start, "closure_record_val_ptr"),
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

  Type *lowered_type =
      capture_callable ? captured_callable_type : callable_type;
  LoweredCallable lowered =
      lower_callable_value(callable_val, lowered_type, ctx, module, builder);
  LLVMTypeRef lowered_callable_type =
      lowered_callable_llvm_type(lowered, lowered_type, ctx, module);
  LLVMValueRef call_args[len + (lowered.has_env ? 1 : 0)];
  unsigned arg_offset = 0;
  if (lowered.has_env) {
    call_args[0] = lowered.env;
    arg_offset = 1;
  }
  for (int j = 0; j < len; j++) {
    call_args[arg_offset + j] = args[j];
  }

  LLVMValueRef body =
      LLVMBuildCall2(builder, lowered_callable_type, lowered.fn, call_args,
                     len + arg_offset, "curried_fn_call");

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
                                       Type *recordt,
                                       LLVMTypeRef closure_rec_type,
                                       LLVMTypeRef clos_fn_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  //
  ObjString fn_name = expr->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }
  Type *clos_type = expr->type;
  clos_type = resolve_type_in_env(clos_type, ctx->env);

  Type *callable_type;

  START_FUNC(module, "lambda", clos_fn_type);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx);

  LLVMValueRef record = LLVMGetParam(func, 0);

  if (!is_anon) {
    add_recursive_closure_fn_ref(fn_name, func, expected_clos_type, record,
                                 closure_rec_type, &fn_ctx, module, builder);
  }
  // int len = fn_type_args_len(expected_clos_type);
  // LLVMValueRef args[len + 1];
  int i = 0;
  for (AstList *closed_vals = expr->data.AST_LAMBDA.closed_vals; closed_vals;
       closed_vals = closed_vals->next, i++) {

    Ast *cl = closed_vals->ast;

    LLVMValueRef closed_val = LLVMBuildLoad2(
        builder,

        type_to_llvm_type(recordt->data.T_CONS.args[i], &fn_ctx, module),

        LLVMBuildStructGEP2(builder, closure_rec_type, record, i,
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
    bind_fn_param(param_val, ef->data.T_FN.from, param, ctx, &fn_ctx, module,
                  builder);
  }

  LLVMValueRef body = codegen_lambda_body(expr, &fn_ctx, module, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  if (current_block && !LLVMGetBasicBlockTerminator(current_block)) {
    if (LLVMIsACallInst(body)) {
      LLVMSetTailCall(body, true);
    }

    if (fn_return_type(clos_type)->kind == T_VOID) {
      LLVMBuildRetVoid(builder);
    } else {
      LLVMBuildRet(builder, body);
    }
  }

  END_FUNC;
  destroy_ctx(&fn_ctx);

  return func;
}

LLVMValueRef call_function_returning_closure() {}

static bool closure_env_should_stack_alloc(Ast *expr, JITLangCtx *ctx) {
  return find_allocation_strategy(expr, ctx) == EA_STACK_ALLOC;
}

LLVMValueRef store_closure_record_values(LLVMValueRef rec_alloc, Ast *expr,
                                         Type *rec_type, LLVMValueRef fn_ptr,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {

  // Convert the record type to LLVM type for GEP operations
  LLVMTypeRef llvm_rec_type = type_to_llvm_type(rec_type, ctx, module);

  // Store function pointer at offset 0
  // LLVMValueRef fn_ptr_slot =
  //     LLVMBuildStructGEP2(builder, llvm_rec_type, rec_alloc, 0,
  //     "fn_ptr_slot");
  // LLVMBuildStore(builder, fn_ptr, fn_ptr_slot);

  if (expr->tag == AST_APPLICATION) {
    int field_idx = 0;
    if (application_function_needs_capture(expr, ctx)) {
      LLVMValueRef callable_val = NULL;
      JITSymbol *callable_sym = lookup_application_callable_symbol(expr, ctx);
      Type *captured_callable_type =
          application_function_capture_type(expr, ctx);

      if (callable_sym && callable_sym->type == STYPE_GENERIC_FUNCTION &&
          callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type) {
        callable_val = materialize_generic_closure_value(
            callable_sym, captured_callable_type, ctx, module, builder);
      } else {
        callable_val =
            codegen(expr->data.AST_APPLICATION.function, ctx, module, builder);
      }

      LLVMValueRef callable_slot = LLVMBuildStructGEP2(
          builder, llvm_rec_type, rec_alloc, field_idx++, "captured_fn_slot");
      LLVMBuildStore(builder, callable_val, callable_slot);
    }

    for (int i = 0; i < expr->data.AST_APPLICATION.len; i++) {
      LLVMValueRef arg_val =
          codegen(expr->data.AST_APPLICATION.args + i, ctx, module, builder);

      LLVMValueRef field_slot = LLVMBuildStructGEP2(
          builder, llvm_rec_type, rec_alloc, field_idx + i, "app_arg_slot");
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
          builder, llvm_rec_type, rec_alloc, i, "closed_val_slot");
      LLVMBuildStore(builder, closed_val, field_slot);
    }

    return rec_alloc;
  }

  return NULL;
}

static LLVMValueRef expr_to_closure_rec_with_env(Ast *expr, Type *clos_type,
                                                 Type *env_type,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder) {
  // printf("expr to closure rec\n");
  // print_ast(expr);

  LLVMTypeRef rec_type = closure_record_type_from_env(env_type, ctx, module);
  LLVMTypeRef clos_fn_type = closure_fn_type(clos_type, rec_type, ctx, module);

  LLVMValueRef rec_storage;

  if (closure_env_should_stack_alloc(expr, ctx)) {
    rec_storage = LLVMBuildAlloca(builder, rec_type, "closure_obj_alloc_stack");
  } else {
    rec_storage = LLVMBuildMalloc(builder, rec_type, "closure_obj_alloc_heap");
    LLVMBuildMemSet(builder, rec_storage, LLVMConstInt(LLVMInt8Type(), 0, 0),
                    LLVMSizeOf(rec_type), 0);
  }

  if (expr->tag == AST_APPLICATION) {
    LLVMValueRef closure_fn =
        compile_curried_fn(expr, clos_type, env_type, rec_type, clos_fn_type,
                           ctx, module, builder);

    rec_storage = store_closure_record_values(rec_storage, expr, env_type,
                                              closure_fn, ctx, module, builder);

    LLVMValueRef str = LLVMGetUndef(
        LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0));
    str = LLVMBuildInsertValue(builder, str, closure_fn, 0, "store_closure_fn");
    str =
        LLVMBuildInsertValue(builder, str, rec_storage, 1, "store_closure_env");

    return str;
  }

  if (expr->tag == AST_LAMBDA) {

    LLVMValueRef closure_fn =
        compile_lambda_as_closure(expr, clos_type, env_type, rec_type,
                                  clos_fn_type, ctx, module, builder);

    rec_storage = store_closure_record_values(rec_storage, expr, env_type,
                                              closure_fn, ctx, module, builder);

    LLVMValueRef str = LLVMGetUndef(
        LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0));
    str = LLVMBuildInsertValue(builder, str, closure_fn, 0, "store_closure_fn");
    str =
        LLVMBuildInsertValue(builder, str, rec_storage, 1, "store_closure_env");
    return str;
  }
  return NULL;
}

LLVMValueRef expr_to_closure_rec(Ast *expr, Type *clos_type, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  return expr_to_closure_rec_with_env(expr, clos_type,
                                      closure_env_type_from_expr(expr, ctx),
                                      ctx, module, builder);
}

LLVMValueRef curried_fn_closure() {}
static LLVMValueRef create_closure_rec(Ast *expr, Type *rec_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  if (!expr || !rec_type) {
    return NULL;
  }

  LLVMTypeRef llvm_rec_type = type_to_llvm_type(rec_type, ctx, module);
  if (!llvm_rec_type) {
    return NULL;
  }

  LLVMValueRef rec_storage;
  if (closure_env_should_stack_alloc(expr, ctx)) {
    rec_storage =
        LLVMBuildAlloca(builder, llvm_rec_type, "closure_env_alloc_stack");
  } else {
    rec_storage =
        LLVMBuildMalloc(builder, llvm_rec_type, "closure_env_alloc_heap");
    LLVMBuildMemSet(builder, rec_storage, LLVMConstInt(LLVMInt8Type(), 0, 0),
                    LLVMSizeOf(llvm_rec_type), 0);
  }

  return store_closure_record_values(rec_storage, expr, rec_type, NULL, ctx,
                                     module, builder);
}

static LLVMValueRef build_closure_value(LLVMValueRef closure_fn,
                                        LLVMValueRef closure_env,
                                        LLVMBuilderRef builder) {
  LLVMValueRef closure = LLVMGetUndef(
      LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0));
  closure =
      LLVMBuildInsertValue(builder, closure, closure_fn, 0, "store_closure_fn");
  closure = LLVMBuildInsertValue(builder, closure, closure_env, 1,
                                 "store_closure_env");
  return closure;
}

static Type *strip_top_level_closure_meta(Type *type) {
  if (!type || !type->closure_meta) {
    return type;
  }

  Type *copy = deep_copy_type(type);
  copy->closure_meta = NULL;
  return copy;
}

LLVMValueRef materialize_generic_closure_value(JITSymbol *sym,
                                               Type *expected_fn_type,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  Type *compiled_type = strip_top_level_closure_meta(expected_fn_type);
  LLVMValueRef closure = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, compiled_type);

  if (closure) {
    return closure;
  }

  Ast expr = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;
  TICtx _ctx = {};

  unify(sym->symbol_type, compiled_type, &_ctx);
  Subst *subst = solve_constraints(_ctx.constraints);
  Type *closure_type = compiled_type;

  JITLangCtx compilation_ctx = *ctx;
  compilation_ctx.stack_ptr = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
  compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;
  TypeEnv *env = sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env;
  env = create_env_from_subst(env, subst);
  compilation_ctx.env = env;
  Type *env_type =
      sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type
          ? sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type
          : closure_env_type_from_expr(&expr, &compilation_ctx);

  if (sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_rec) {
    LLVMTypeRef rec_type =
        closure_record_type_from_env(env_type, &compilation_ctx, module);
    LLVMTypeRef clos_fn_type =
        closure_fn_type(closure_type, rec_type, &compilation_ctx, module);
    LLVMValueRef closure_fn =
        compile_curried_fn(&expr, closure_type, env_type, rec_type,
                           clos_fn_type, &compilation_ctx, module, builder);
    closure = build_closure_value(
        closure_fn, sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_rec,
        builder);
  } else {
    closure = expr_to_closure_rec_with_env(&expr, closure_type, env_type,
                                           &compilation_ctx, module, builder);
  }

  if (!closure) {
    return NULL;
  }

  sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
      specific_fns_extend(sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns,
                          compiled_type, closure);
  return closure;
}

LLVMValueRef create_curried_generic_closure_binding(
    Ast *binding, Type *closure_type, Ast *closure, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {
  return create_closure_symbol(binding, closure, ctx, module, builder);
}

LLVMValueRef create_curried_closure_binding(Ast *binding, Type *closure_type,
                                            Ast *closure, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  (void)closure_type;
  return create_closure_symbol(binding, closure, ctx, module, builder);
}

LLVMValueRef create_closure_symbol(Ast *binding, Ast *expr, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  Type *clos_type = expr->type;
  Type *cl_env = closure_env_type_from_expr(expr, ctx);

  if (expr->tag == AST_APPLICATION && is_generic(clos_type)) {

    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, clos_type, NULL, NULL);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = expr;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame = ctx->frame;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env = ctx->env;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type = cl_env;

    if (expr->tag == AST_APPLICATION && clos_type->kind == T_FN && cl_env) {
      LLVMValueRef closure_rec =
          create_closure_rec(expr, cl_env, ctx, module, builder);
      sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_rec = closure_rec;
    }

    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                sym);
    return NULL;
  }

  LLVMValueRef closure = expr_to_closure_rec_with_env(expr, clos_type, cl_env,
                                                      ctx, module, builder);

  if (!closure) {
    fprintf(stderr, "Error: could not compile closure obj\n");
    print_ast_err(expr);
    return NULL;
  }
  LLVMTypeRef llvm_closure_rec_type =
      closure_record_type_from_env(cl_env, ctx, module);

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, clos_type, closure,
                              LLVMPointerType(llvm_closure_rec_type, 0));
  sym->symbol_data.STYPE_FUNCTION.closure_env_type = cl_env;
  if (expr->tag == AST_APPLICATION && clos_type->kind == T_FN && cl_env) {
    sym->symbol_data.STYPE_FUNCTION.closure_rec =
        create_closure_rec(expr, cl_env, ctx, module, builder);
  }

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);

  return sym->val;
}

LLVMTypeRef closure_record_type(Type *clos_type, JITLangCtx *ctx,
                                LLVMModuleRef module) {
  return closure_record_type_from_env(
      clos_type ? clos_type->closure_meta : NULL, ctx, module);
}

LLVMTypeRef closure_fn_type(Type *clos_type, LLVMTypeRef closure_rec_type,
                            JITLangCtx *ctx, LLVMModuleRef module) {

  LLVMTypeRef closure_rec_ptr_type = LLVMPointerType(closure_rec_type, 0);
  if (is_void_func(clos_type)) {
    Type *ret_type = clos_type->data.T_FN.to;
    LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx, module);
    LLVMTypeRef ftype = LLVMFunctionType(
        llvm_ret_type, (LLVMTypeRef[]){closure_rec_ptr_type}, 1, 0);
    return ftype;
  }

  int args_len = 1;

  Type *t = clos_type;

  while (t->kind == T_FN && !is_closure(t->data.T_FN.from)) {
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
  // printf("codegen curried fn closure\n");
  // print_ast(ast);
  Type *closure_type = ast->type;
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
  // print_ast(ast);
  snprintf(name, 32, "lambda.closure.\\%s",
           ast->data.AST_LAMBDA.fn_name.chars
               ? ast->data.AST_LAMBDA.fn_name.chars
               : ast->data.AST_LAMBDA.params->ast->data.AST_IDENTIFIER.value);

  START_FUNC(module, name, llvm_clos_fn_type);
  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
  LLVMValueRef inner_closure_rec = LLVMGetParam(func, 0);

  Type *clos_type = fn_type->closure_meta;
  AST_LIST_ITER(ast->data.AST_LAMBDA.closed_vals, ({
                  Ast *param_ast = l->ast;
                  LLVMValueRef param_val =
                      LLVMBuildStructGEP2(builder, rec_type, inner_closure_rec,
                                          i, "closed_val_from_rec");
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
  if (closure_env_should_stack_alloc(ast, ctx)) {
    rec_storage = LLVMBuildAlloca(builder, rec_type, "closure_obj_alloc_stacc");
  } else {
    rec_storage = LLVMBuildMalloc(builder, rec_type, "closure_obj_alloc_heap");
    LLVMBuildMemSet(builder, rec_storage, LLVMConstInt(LLVMInt8Type(), 0, 0),
                    LLVMSizeOf(rec_type), 0);
  }

  AST_LIST_ITER(ast->data.AST_LAMBDA.closed_vals, ({
                  Ast *param_ast = l->ast;
                  LLVMValueRef param_val =
                      codegen(param_ast, ctx, module, builder);

                  LLVMValueRef field_slot = LLVMBuildStructGEP2(
                      builder, rec_type, rec_storage, i, "closed_val_slot");
                  LLVMBuildStore(builder, param_val, field_slot);
                }));

  LLVMValueRef str = LLVMGetUndef(
      LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0));
  str = LLVMBuildInsertValue(builder, str, func, 0, "store_closure_fn");
  str = LLVMBuildInsertValue(builder, str, rec_storage, 1, "store_closure_env");

  return str;
}

LLVMValueRef codegen_const_curried_fn(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  Type *fn_type = ast->type;

  int fn_len = fn_type_args_len(fn_type);
  LLVMTypeRef prototype = codegen_fn_type(NULL, fn_type, fn_len, ctx, module);

  if (!prototype) {
    return NULL;
  }

  START_FUNC(module, "curried_fn_with_const_params", prototype)

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  Type *inner_fn_type = ast->data.AST_APPLICATION.function->type;
  inner_fn_type = resolve_type_in_env(inner_fn_type, ctx->env);
  int inner_args_len = fn_type_args_len(inner_fn_type);

  LLVMValueRef inner_args[inner_args_len];
  Type *f = inner_fn_type;
  int i = 0;
  for (i = 0; i < ast->data.AST_APPLICATION.len; i++, f = f->data.T_FN.to) {
    inner_args[i] =
        codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);
    inner_args[i] = handle_type_conversions(
        inner_args[i], ast->data.AST_APPLICATION.args[i].type,
        f->data.T_FN.from, ctx, module, builder);
  }

  int const_args_len = ast->data.AST_APPLICATION.len;
  for (i = 0; i < fn_len; i++) {
    // Runtime parameters should follow the already-materialized const args.
    inner_args[const_args_len + i] = LLVMGetParam(func, i);
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
    fn_type = ast->data.AST_APPLICATION.function->type;
    return codegen_curried_fn_closure(fn_type, ast, ctx, module, builder);
  }

  if (ast->tag == AST_LAMBDA) {
    fn_type = ast->type;
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

LLVMValueRef call_generic_closure_sym(Ast *app, Type *expected_fn_type,
                                      JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  LLVMValueRef closure = materialize_generic_closure_value(
      sym, expected_fn_type, ctx, module, builder);

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

  if (sym->type == STYPE_GENERIC_FUNCTION &&
      sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type) {
    call_type = deep_copy_type(expected_fn_type);
    call_type->closure_meta =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.closure_env_type;
  }

  if (sym->type == STYPE_FUNCTION &&
      sym->symbol_data.STYPE_FUNCTION.closure_env_type) {
    call_type = deep_copy_type(call_type);
    call_type->closure_meta = sym->symbol_data.STYPE_FUNCTION.closure_env_type;
  }

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return call_generic_closure_sym(app, call_type, sym, ctx, module, builder);
  }

  return call_callable(app, call_type, sym->val, ctx, module, builder);
}
