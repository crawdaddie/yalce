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
#include <string.h>

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static int const_curried_fn_counter = 0;

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
  rec_env = LLVMBuildBitCast(builder, rec_env, LLVMPointerType(env_type, 0),
                             "typed_env");

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

static LLVMValueRef emit_closure_call(LLVMValueRef closure, Type *callable_type,
                                      LLVMValueRef *args, int num_args,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  LLVMTypeRef ret_type =
      type_to_llvm_type(fn_return_type(callable_type), ctx, module);

  LLVMTypeRef param_types[num_args + 1];
  param_types[0] = GENERIC_PTR;
  for (int i = 0; i < num_args; i++) {
    Type *arg_type = callable_arg_type(callable_type, i);
    param_types[i + 1] = type_to_llvm_type(arg_type, ctx, module);
  }

  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, param_types, num_args + 1, 0);

  LLVMValueRef fn = LLVMBuildExtractValue(builder, closure, 0, "inner_fn");
  LLVMValueRef env = LLVMBuildExtractValue(builder, closure, 1, "inner_env");

  LLVMValueRef call_args[num_args + 1];
  call_args[0] = env;
  for (int i = 0; i < num_args; i++) {
    call_args[i + 1] = args[i];
  }

  return LLVMBuildCall2(builder, fn_type, fn, call_args, num_args + 1,
                        "curried_closure_call");
}

LLVMTypeRef get_named_closure_type(LLVMModuleRef module) {
  LLVMContextRef ctx = LLVMGetModuleContext(module);
  LLVMTypeRef closure_type = LLVMGetTypeByName2(ctx, "Closure");
  if (!closure_type) {
    closure_type = LLVMStructCreateNamed(ctx, "Closure");
    LLVMTypeRef body[] = {GENERIC_PTR, GENERIC_PTR};
    LLVMStructSetBody(closure_type, body, 2, 0);
  }
  return closure_type;
}

static bool value_is_closure(LLVMValueRef val) {
  if (!val) {
    return false;
  }
  LLVMTypeRef type = LLVMTypeOf(val);
  if (LLVMGetTypeKind(type) != LLVMStructTypeKind) {
    return false;
  }
  const char *name = LLVMGetStructName(type);
  return name && strcmp(name, "Closure") == 0;
}

static LLVMValueRef
build_generic_closure_value(JITSymbol *sym, Type *expected_fn_type,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder, Type **out_clos_type);

LLVMValueRef compile_curried_fn(Ast *expr, Type *expected_clos_type,
                                LLVMTypeRef closure_rec_type,
                                LLVMTypeRef clos_fn_type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *clos_type = expr->type;
  clos_type = resolve_type_in_env(clos_type, ctx->env);

  Type *callable_type;
  LLVMTypeRef llvm_callable_type;
  JITSymbol *callable_sym = NULL;
  const char *fname;

  if (expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    fname = expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    callable_sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    if (!callable_sym) {
      fprintf(stderr, "Symbol to curry not found\n");
      return NULL;
    }

    Type *ftype = deep_copy_type(expr->data.AST_APPLICATION.function->type);
    ftype = resolve_type_in_env(ftype, ctx->env);
    callable_type = ftype;
    llvm_callable_type = type_to_llvm_type(callable_type, ctx, module);

  } else if (expr->data.AST_APPLICATION.function->tag == AST_LAMBDA) {

    fname = expr->data.AST_APPLICATION.function->data.AST_LAMBDA.fn_name.chars;
    callable_type = expr->data.AST_APPLICATION.function->type;
    llvm_callable_type = type_to_llvm_type(callable_type, ctx, module);
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

    LLVMTypeRef lt = type_to_llvm_type(rt, &fn_ctx, module);

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

  LLVMValueRef callable_val;
  bool callable_is_closure = false;

  if (expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    if (is_generic(callable_sym->symbol_type)) {
      if (callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.ast &&
          callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.ast->tag ==
              AST_APPLICATION) {
        callable_val = build_generic_closure_value(callable_sym, callable_type,
                                                   ctx, module, builder, NULL);
        callable_is_closure = true;
      } else {
        callable_val = find_callable_from_generic(
            expr, callable_type, callable_type, ctx, module, builder);
        callable_is_closure = value_is_closure(callable_val);
      }
    } else {
      callable_val = callable_sym->val;
    }
  } else {
    callable_val = codegen(expr, ctx, module, builder);
    callable_is_closure = value_is_closure(callable_val);
  }

  LLVMValueRef body;
  if (callable_is_closure) {
    body = emit_closure_call(callable_val, callable_type, args, len, &fn_ctx,
                             module, builder);
  } else {
    body = LLVMBuildCall2(builder, llvm_callable_type, callable_val, args, len,
                          "curried_fn_call");
  }

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

static LLVMValueRef
build_generic_closure_value(JITSymbol *sym, Type *expected_fn_type,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder, Type **out_clos_type) {
  Type *original_type = deep_copy_type(sym->symbol_type);
  Type *expected_type = deep_copy_type(expected_fn_type);

  TypeEnv *env = sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env;

  JITLangCtx compilation_ctx = *ctx;
  compilation_ctx.type_subst =
      create_subst_for_generic_fn(original_type, expected_type);

  compilation_ctx.env = create_env_from_subst(env, compilation_ctx.type_subst);

  Type *clos_type = NULL;
  LLVMValueRef closure = NULL;

  if (sym->symbol_data.STYPE_GENERIC_FUNCTION.ast->tag == AST_APPLICATION) {

    Ast *generic_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    clos_type = deep_copy_type(generic_ast->type);
    clos_type = resolve_type_in_env(clos_type, compilation_ctx.env);

    LLVMTypeRef llvm_rec_type =
        closure_record_type(clos_type, &compilation_ctx, module);
    LLVMTypeRef llvm_fn_type =
        closure_fn_type(clos_type, llvm_rec_type, &compilation_ctx, module);

    LLVMValueRef wrapper = specific_fns_lookup_decl(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, clos_type,
        llvm_fn_type, module);

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
    LLVMValueRef env_ptr =
        LLVMBuildBitCast(builder, env_val, GENERIC_PTR, "curried_env_ptr");

    closure = LLVMGetUndef(closure_struct_type);
    closure = LLVMBuildInsertValue(builder, closure, fn_ptr, 0, "closure_fn");
    closure = LLVMBuildInsertValue(builder, closure, env_ptr, 1, "closure_env");
  }

  if (out_clos_type) {
    *out_clos_type = clos_type;
  }

  return closure;
}

LLVMValueRef call_generic_closure_sym(Ast *app, Type *expected_fn_type,
                                      JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  Type *clos_type = NULL;
  LLVMValueRef closure = build_generic_closure_value(
      sym, expected_fn_type, ctx, module, builder, &clos_type);

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

  int num_params = 0;
  for (Type *t = clos_type; t->kind == T_FN && t->data.T_FN.from->kind != T_VOID;
       t = t->data.T_FN.to) {
    num_params++;
  }

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

  // Skip past any void parameter layer to reach the return type.
  if (f->kind == T_FN && f->data.T_FN.from->kind == T_VOID) {
    f = f->data.T_FN.to;
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
  Type *fn_type = ast->type;

  int fn_len = fn_type_args_len(fn_type);
  LLVMTypeRef prototype = codegen_fn_type(NULL, fn_type, fn_len, ctx, module);

  if (!prototype) {
    return NULL;
  }

  char fn_name[64];
  snprintf(fn_name, sizeof(fn_name), "curried_fn_with_const_params.%d",
           const_curried_fn_counter++);

  START_FUNC(module, fn_name, prototype)

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  Type *inner_fn_type = ast->data.AST_APPLICATION.function->type;
  JITSymbol *inner_sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);
  if (inner_sym && inner_sym->symbol_type) {
    inner_fn_type = inner_sym->symbol_type;
  }
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
  int remaining_inner_args = inner_args_len - const_args_len;
  for (i = 0; i < fn_len && i < remaining_inner_args; i++) {
    // Runtime parameters should follow the already-materialized const args.
    inner_args[const_args_len + i] = LLVMGetParam(func, i);
  }

  LLVMValueRef inner_fn =
      codegen(ast->data.AST_APPLICATION.function, ctx, module, builder);

  LLVMTypeRef llvm_inner_fn_type =
      LLVMGlobalGetValueType(inner_fn);

  unsigned actual_inner_args_len = LLVMCountParamTypes(llvm_inner_fn_type);
  LLVMTypeRef actual_param_types[actual_inner_args_len
                                     ? actual_inner_args_len
                                     : 1];
  if (actual_inner_args_len) {
    LLVMGetParamTypes(llvm_inner_fn_type, actual_param_types);
  }

  unsigned call_args_len = (unsigned)inner_args_len;
  if (actual_inner_args_len < call_args_len) {
    call_args_len = actual_inner_args_len;
  }

  for (unsigned j = 0; j < call_args_len; j++) {
    if (LLVMTypeOf(inner_args[j]) == actual_param_types[j]) {
      continue;
    }

    Type *fallback_to = fn_return_type(fn_type);
    Type *from_type = NULL;
    if (j < ast->data.AST_APPLICATION.len) {
      from_type = ast->data.AST_APPLICATION.args[j].type;
    }
    inner_args[j] = handle_type_conversions(inner_args[j], from_type,
                                            fallback_to, ctx, module, builder);
  }

  LLVMValueRef body =
      LLVMBuildCall2(builder, llvm_inner_fn_type, inner_fn, inner_args,
                     call_args_len, "curried_fn_inner_call");

  Type *res_type = fn_return_type(fn_type);
  if (res_type->kind == T_VOID) {
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  END_FUNC
  destroy_ctx(&fn_ctx);

  return func;
}

LLVMValueRef compile_closure(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  Type *clos_type = specialize_type_for_codegen(ast->type, ctx);
  Type *recordt = clos_type->closure_meta;

  if (!recordt) {
    fprintf(stderr, "Error: closure has no closure metadata\n");
    print_ast_err(ast);
    return NULL;
  }

  LLVMTypeRef closure_rec_type = closure_record_type(clos_type, ctx, module);
  LLVMTypeRef clos_fn_type =
      closure_fn_type(clos_type, closure_rec_type, ctx, module);

  if (!clos_fn_type) {
    return NULL;
  }

  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;
  char name_buf[64];
  if (!fn_name || !fn_name[0]) {
    static int anon_closure_count = 0;
    snprintf(name_buf, sizeof(name_buf), "closure.%d", anon_closure_count++);
    fn_name = name_buf;
  }

  START_FUNC(module, fn_name, clos_fn_type);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx);

  // Bind closed values from the environment parameter.
  LLVMValueRef env_param = LLVMGetParam(func, 0);
  int closed_len = ast->data.AST_LAMBDA.num_closed_vals;
  AstList *cv = ast->data.AST_LAMBDA.closed_vals;
  for (int i = 0; i < closed_len && cv; i++, cv = cv->next) {
    Type *field_type = recordt->data.T_CONS.args[i];
    Ast *ref_ast = cv->ast;

    LLVMTypeRef llvm_field_type = type_to_llvm_type(field_type, &fn_ctx, module);
    LLVMValueRef field_ptr = LLVMBuildStructGEP2(
        builder, closure_rec_type, env_param, i, "closure_field_ptr");
    LLVMValueRef field_val = LLVMBuildLoad2(
        builder,
        field_type->kind == T_FN && !is_closure(field_type) ? GENERIC_PTR
                                                            : llvm_field_type,
        field_ptr, "closure_field");

    bind_fn_param(field_val, field_type, ref_ast, &fn_ctx, &fn_ctx, module,
                  builder);
  }

  // Bind lambda parameters (after the env parameter).
  AstList *param = ast->data.AST_LAMBDA.params;
  Type *f = clos_type;
  int llvm_param_idx = 1;
  while (param) {
    Ast *param_ast = param->ast;
    if (param_ast->tag == AST_VOID) {
      f = f->data.T_FN.to;
      param = param->next;
      continue;
    }

    LLVMValueRef param_val = LLVMGetParam(func, llvm_param_idx++);
    Type *param_type = f->data.T_FN.from;
    bind_fn_param(param_val, param_type, param_ast, &fn_ctx, &fn_ctx, module,
                  builder);
    f = f->data.T_FN.to;
    param = param->next;
  }

  set_tail_call_expressions(ast->data.AST_LAMBDA.body);

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  if (current_block && !LLVMGetBasicBlockTerminator(current_block)) {
    if (LLVMIsACallInst(body)) {
      LLVMSetTailCall(body, true);
    }
    build_ret(body, f, builder);
  }

  END_FUNC;
  destroy_ctx(&fn_ctx);

  // Build the closure environment in the caller's context.
  LLVMValueRef env_val =
      LLVMBuildMalloc(builder, closure_rec_type, "closure_env");

  cv = ast->data.AST_LAMBDA.closed_vals;
  for (int i = 0; i < closed_len && cv; i++, cv = cv->next) {
    Type *field_type = recordt->data.T_CONS.args[i];
    Ast *ref_ast = cv->ast;

    LLVMValueRef val = codegen(ref_ast, ctx, module, builder);
    if (!val) {
      fprintf(stderr, "Error: could not codegen closed value %d for closure\n",
              i);
      print_ast_err(ref_ast);
      return NULL;
    }

    Type *arg_type = specialize_type_for_codegen(ref_ast->type, ctx);
    Type *expected_field_type = specialize_type_for_codegen(field_type, ctx);
    val = handle_type_conversions(val, arg_type, expected_field_type, ctx,
                                  module, builder);

    LLVMValueRef field_ptr = LLVMBuildStructGEP2(
        builder, closure_rec_type, env_val, i, "closure_env_field_ptr");
    LLVMBuildStore(builder, val, field_ptr);
  }

  LLVMTypeRef closure_struct_type = type_to_llvm_type(clos_type, ctx, module);
  LLVMValueRef fn_ptr =
      LLVMBuildBitCast(builder, func, GENERIC_PTR, "closure_fn_ptr");
  LLVMValueRef env_ptr =
      LLVMBuildBitCast(builder, env_val, GENERIC_PTR, "closure_env_ptr");

  LLVMValueRef closure = LLVMGetUndef(closure_struct_type);
  closure = LLVMBuildInsertValue(builder, closure, fn_ptr, 0, "closure_fn");
  closure = LLVMBuildInsertValue(builder, closure, env_ptr, 1, "closure_env");

  return closure;
}

LLVMValueRef codegen_create_closure(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  if (ast->tag == AST_APPLICATION &&
      ast->data.AST_APPLICATION.is_curried_with_constants) {
    return codegen_const_curried_fn(ast, ctx, module, builder);
  }

  if (ast->tag == AST_LAMBDA) {
    return compile_closure(ast, ctx, module, builder);
  }

  return NULL;
}
