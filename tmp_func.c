#include "backend_llvm/common.h"
#include "backend_llvm/function.h"
#include "backend_llvm/match.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "backend_llvm/variant.h"
#include "parse.h"
#include "serde.h"
#include "types/unification.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

#define TRY(expr)                                                              \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef fn_prototype(Type *fn_type, int fn_len, TypeEnv *env,
                         LLVMModuleRef module) {

  LLVMTypeRef llvm_param_types[fn_len];

  for (int i = 0; i < fn_len; i++) {
    llvm_param_types[i] =
        type_to_llvm_type(fn_type->data.T_FN.from, env, module);

    fn_type = fn_type->data.T_FN.to;
  }

  Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
  LLVMTypeRef llvm_return_type_ref =
      type_to_llvm_type(return_type, env, module);

  // Create function type with return.
  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
  return llvm_fn_type;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);
  int params_count = ast->data.AST_EXTERN_FN.len - 1;

  Ast *signature_types = ast->data.AST_EXTERN_FN.signature_types;
  if (params_count == 0) {

    LLVMTypeRef ret_type =
        llvm_type_of_identifier(signature_types, ctx->env, module);
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, NULL, 0, false);

    LLVMValueRef func = LLVMAddFunction(module, name, fn_type);
    return func;
  }

  LLVMTypeRef llvm_param_types[params_count];
  for (int i = 0; i < params_count; i++) {
    llvm_param_types[i] =
        llvm_type_of_identifier(signature_types + i, ctx->env, module);
  }

  LLVMTypeRef ret_type =
      llvm_type_of_identifier(signature_types + params_count, ctx->env, module);
  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, llvm_param_types, params_count, false);

  return get_extern_fn(name, fn_type, module);
}

static void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func,
                                 Type *fn_type, JITLangCtx *fn_ctx) {

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  printf("generating fn %s: ", fn_name.chars);
  Type *fn_type = ast->md;
  print_type(fn_type);

  int fn_len = ast->data.AST_LAMBDA.len;
  LLVMTypeRef prototype = fn_prototype(ast->md, fn_len, ctx->env, module);

  LLVMValueRef func = LLVMAddFunction(module, fn_name.chars, prototype);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  add_recursive_fn_ref(fn_name, func, fn_type, &fn_ctx);

  for (int i = 0; i < fn_len; i++) {
    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    Type *param_type = fn_type->data.T_FN.from;
    printf("param type %d: ", i);
    print_type(param_type);

    LLVMValueRef param_val = LLVMGetParam(func, i);

    if (param_type->kind == T_FN) {
      const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
      int id_len = param_ast->data.AST_IDENTIFIER.length;
      LLVMTypeRef llvm_type = LLVMTypeOf(param_val);
      JITSymbol *sym =
          new_symbol(STYPE_LOCAL_VAR, param_type, param_val, llvm_type);
      ht_set_hash(fn_ctx.stack + fn_ctx.stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
    } else {
      match_values(param_ast, param_val, param_type, &fn_ctx, module, builder);
    }

    fn_type = fn_type->data.T_FN.to;
  }

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  if (body == NULL) {
    fprintf(stderr, "Error compiling function body\n");
    print_ast_err(ast);
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // clear function stack frame
  if (fn_ctx.stack_ptr > 0) {
    ht *stack_frame = fn_ctx.stack + fn_ctx.stack_ptr;
    ht_reinit(stack_frame);
  }

  return func;
}

LLVMValueRef codegen_constructor(Ast *cons_id, Ast *args, int len,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {

  int member_idx;
  Type *variant = variant_lookup(ctx->env, cons_id->md, &member_idx);

  if (variant) {
    // printf("constructor of variant? ");
    // print_type(variant);
  }

  return NULL;
}

LLVMValueRef specific_fns_lookup(SpecificFns *list, Type *key) {
  while (list) {
    if (types_equal(key, list->arg_types_key)) {
      return list->func;
    }
    list = list->next;
  }
  return NULL;
};

SpecificFns *specific_fns_extend(SpecificFns *list, Type *arg_types,
                                 LLVMValueRef func) {

  SpecificFns *new_specific_fn = malloc(sizeof(SpecificFns));
  new_specific_fn->arg_types_key = arg_types;
  new_specific_fn->func = func;
  new_specific_fn->next = list;
  return new_specific_fn;
};

static Ast *get_specific_fn_ast_variant(Ast *original_fn_ast,
                                        Type *specific_fn_type) {

  Type *generic_type = original_fn_ast->md;
  TypeEnv *replacement_env = NULL;
  const char *fn_name = original_fn_ast->data.AST_LAMBDA.fn_name.chars;

  Ast *specific_ast = malloc(sizeof(Ast));
  *specific_ast = *(original_fn_ast);

  specific_ast->md = specific_fn_type;
  return specific_ast;
}

TypeEnv *create_replacement_env(Type *generic_fn_type, Type *specific_fn_type,
                                TypeEnv *env) {
  Type *l = generic_fn_type;
  Type *r = specific_fn_type;
  unify(l, r, &env);

  return env;
}

LLVMValueRef create_new_specific_fn(int len, Ast *args, Ast *fn_ast,
                                    Type *original_type, Type *expected_type,
                                    Type *ret_type, JITLangCtx *compilation_ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  // compile new variant
  Ast *specific_ast = get_specific_fn_ast_variant(fn_ast, expected_type);

  TypeEnv *og_env = compilation_ctx->env;
  TypeEnv *_env = compilation_ctx->env;
  Type *o = original_type;
  Type *e = expected_type;
  while (o->kind == T_FN) {
    Type *of = o->data.T_FN.from;
    Type *ef = e->data.T_FN.from;
    if (of->kind == T_VAR && !(env_lookup(_env, of->data.T_VAR))) {
      _env = env_extend(_env, of->data.T_VAR, ef);
    }
    o = o->data.T_FN.to;
    e = e->data.T_FN.to;
  }
  if (o->kind == T_VAR) {
    _env = env_extend(_env, o->data.T_VAR, e);
  }

  compilation_ctx->env = _env;
  // printf("creating specific fn: ");
  // print_type(expected_type);
  LLVMValueRef func =
      codegen_fn(specific_ast, compilation_ctx, module, builder);

  compilation_ctx->env = og_env;
  return func;
}

void prepare_specific_fn(LLVMValueRef *callable,
                         LLVMTypeRef *llvm_callable_type,
                         Type *expected_fn_type, Ast *args,
                         const char *sym_name, JITSymbol *sym, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  SpecificFns *specific_fns =
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;

  *callable = specific_fns_lookup(specific_fns, expected_fn_type);

  if (!callable) {
    Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    JITLangCtx compilation_ctx = {
        ctx->stack,
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr,
    };

    LLVMValueRef specific_func = create_new_specific_fn(
        fn_ast->data.AST_LAMBDA.len, args, fn_ast, sym->symbol_type,
        expected_fn_type, fn_return_type(expected_fn_type), &compilation_ctx,
        module, builder);

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
        specific_fns_extend(specific_fns, expected_fn_type, specific_func);

    ht *scope = compilation_ctx.stack + compilation_ctx.stack_ptr;
    int sym_name_len = strlen(sym_name);

    ht_set_hash(scope, sym_name, hash_string(sym_name, sym_name_len),
                (void *)sym);

    *callable = specific_func;
    *llvm_callable_type = LLVMGlobalGetValueType(*callable);
  }
}

LLVMValueRef call_symbol(const char *sym_name, JITSymbol *sym, Ast *args,
                         int args_len, Type *expected_fn_type, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  // printf("call symbol %s %d: ", sym_name, sym->type);
  // print_type(expected_fn_type);
  LLVMValueRef callable;

  int expected_args_len = fn_type_args_len(expected_fn_type);
  Type *callable_type = sym->symbol_type;
  LLVMTypeRef llvm_callable_type = sym->llvm_type;

  switch (sym->type) {
  case STYPE_FUNCTION: {
    callable = sym->val;
    callable_type = sym->symbol_type;
    llvm_callable_type = sym->llvm_type;
    // expected_args_len = fn_type_args_len(sym->symbol_type);
    break;
  }

  case STYPE_GENERIC_FUNCTION: {
    // printf("looking up spec func %s\n", sym_name);
    // print_type(expected_fn_type);
    // prepare_generic_fn(&callable, &llvm_callable_type, expected_fn_type,
    // args,
    //                    sym_name, sym, ctx, module, builder);
    // callable_type = expected_fn_type;
    // break;
    SpecificFns *specific_fns =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;
    callable = specific_fns_lookup(specific_fns, expected_fn_type);
    callable_type = expected_fn_type;

    if (!callable) {
      printf("callable not found %s, creating new: \n", sym_name);
      print_type(expected_fn_type);
      Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

      JITLangCtx compilation_ctx = {
          ctx->stack,
          sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr,
      };

      LLVMValueRef specific_func = create_new_specific_fn(
          fn_ast->data.AST_LAMBDA.len, args, fn_ast, sym->symbol_type,
          expected_fn_type, fn_return_type(expected_fn_type), &compilation_ctx,
          module, builder);

      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
          specific_fns_extend(specific_fns, expected_fn_type, specific_func);

      ht *scope = compilation_ctx.stack + compilation_ctx.stack_ptr;
      int sym_name_len = strlen(sym_name);

      ht_set_hash(scope, sym_name, hash_string(sym_name, sym_name_len),
                  (void *)sym);

      callable = specific_func;
      llvm_callable_type = LLVMGlobalGetValueType(callable);
      break;
    }
    break;
  }
  }

  if (args_len < expected_args_len) {
    // TODO : currying
    return NULL;
  }

  if (args_len == expected_args_len) {
    LLVMValueRef app_vals[args_len];

    // Type *callable_type = expected_fn_type;
    if (callable_type->kind == T_FN &&
        callable_type->data.T_FN.from->kind == T_VOID) {

      return LLVMBuildCall2(builder, llvm_callable_type, callable,
                            (LLVMValueRef[]){}, 0, "call_func");
    }

    printf("%s : ", sym_name);
    print_type(callable_type);
    print_type(sym->symbol_type);

    LLVMDumpValue(callable);
    printf("\n");
    for (int i = 0; i < args_len; i++) {
      Ast *app_val_ast = args + i;
      Type *app_val_type = app_val_ast->md;

      if (app_val_type->kind == T_VAR) {
        app_val_type = env_lookup(ctx->env, app_val_type->data.T_VAR);
      }

      app_val_type = resolve_tc_rank(app_val_type);
      printf("%s app val %d: ", sym_name, i);
      print_type(app_val_type);

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);
      // Type *fn_from = callable_type->kind == T_FN
      //                     ? callable_type->data.T_FN.from
      //                     : callable_type;
      //
      // if (is_generic(fn_from)) {
      //   fn_from = resolve_generic_type(fn_from, ctx->env);
      // }
      //
      // if (!types_equal(app_val_type, fn_from) &&
      //     !(variant_contains_type(app_val_type, fn_from, NULL))) {
      //
      //   app_val = TRY_MSG(attempt_value_conversion(app_val, app_val_type,
      //                                              fn_from, module, builder),
      //                     "Error: attempted type conversion failed\n");
      // }
      app_vals[i] = app_val;
    }

    return LLVMBuildCall2(builder, llvm_callable_type, callable, app_vals,
                          args_len, "call_func");
  }

  return NULL;
}

typedef LLVMValueRef (*LLVMBinopMethod)(LLVMValueRef, LLVMValueRef,
                                        LLVMModuleRef, LLVMBuilderRef);
LLVMValueRef call_binop(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *ltype = ast->data.AST_APPLICATION.args->md;
  if (ltype->kind == T_VAR) {
    ltype = env_lookup(ctx->env, ltype->data.T_VAR);
  }
  Type *rtype = (ast->data.AST_APPLICATION.args + 1)->md;
  if (rtype->kind == T_VAR) {
    rtype = env_lookup(ctx->env, rtype->data.T_VAR);
  }

  const char *binop_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  LLVMValueRef lval =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef rval =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  Type *res_type = ast->md;
  if (is_generic(res_type)) {
    res_type = resolve_generic_type(res_type, ctx->env);
  }

  LLVMBinopMethod method = get_binop_method(binop_name, ltype);

  LLVMValueRef r = method(lval, rval, module, builder);
  return r;
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  // int lookup_sym_err = sym == NULL;
  // lookup_id_ast_in_place(ast->data.AST_APPLICATION.function, ctx, &sym);

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  LLVMTypeRef tagged_union_type =
      variant_member_to_llvm_type(ast->md, ctx->env, module);

  if (tagged_union_type) {
    return tagged_union_constructor(ast, tagged_union_type, ctx, module,
                                    builder);
  }

  if (!sym) {
    fprintf(stderr, "codegen identifier failed symbol not found in scope %d:\n",
            ctx->stack_ptr);
    print_ast_err(ast->data.AST_APPLICATION.function);
    print_location(ast);
    return NULL;
  }

  Type *builtin_binop = get_builtin_type(sym_name);
  if (builtin_binop) {
    return call_binop(ast, sym, ctx, module, builder);
  }

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;
  print_type(expected_fn_type);

  return call_symbol(sym_name, sym, ast->data.AST_APPLICATION.args,
                     ast->data.AST_APPLICATION.len, expected_fn_type, ctx,
                     module, builder);
}
