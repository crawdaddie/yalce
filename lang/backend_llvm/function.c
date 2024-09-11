#include "backend_llvm/function.h"
#include "backend_llvm/types.h"
#include "common.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "types/unification.h"
#include "util.h"
#include "variant.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

#define TRY(expr)                                                              \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

#define TRY_MSG(expr, msg)                                                     \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      if (msg) {                                                               \
        fprintf(stderr, "%s\n", msg);                                          \
      }                                                                        \
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

  // Generate the prototype first.
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  Type *fn_type = ast->md;
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

    LLVMValueRef param_val = LLVMGetParam(func, i);

    // if (is_variant_type(param_type)) {
    //   LLVMValueRef alloca = LLVMBuildAlloca(
    //       builder, type_to_llvm_type(param_type, ctx->env, module), "");
    //   LLVMBuildStore(builder, param_val, alloca);
    //   param_val = alloca;
    // }
    match_values(param_ast, param_val, param_type, &fn_ctx, module, builder);

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

  // while (l->kind == T_FN && r->kind == T_FN) {
  //   Type *from_l = l->data.T_FN.from;
  //   Type *from_r = r->data.T_FN.from;
  //
  //   // Check if left type is T_VAR and right type is not T_VAR
  //   if (from_l->kind == T_VAR && from_r->kind != T_VAR) {
  //     const char *name = from_l->data.T_VAR;
  //     if (!env_lookup(env, name)) {
  //       env = env_extend(env, name, from_r);
  //     }
  //   } else if (
  //     (from_l->kind == T_CONS && from_r->kind == T_CONS)
  //   )
  //
  //
  //   // Move to the next level in the function types
  //   l = l->data.T_FN.to;
  //   r = r->data.T_FN.to;
  // }
  //
  // // Handle the final return type
  // if (l->kind == T_VAR && r->kind != T_VAR) {
  //
  //   const char *name = l->data.T_VAR;
  //   if (!env_lookup(env, name)) {
  //     env = env_extend(env, name, r);
  //   }
  // }

  return env;
}

LLVMValueRef create_new_specific_fn(int len, Ast *args, Ast *fn_ast,
                                    Type *original_type, Type *expected_type,
                                    Type *ret_type, JITLangCtx *compilation_ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  // print_type_env(compilation_ctx->env);

  // compile new variant
  Ast *specific_ast = get_specific_fn_ast_variant(fn_ast, expected_type);

  TypeEnv *env = compilation_ctx->env;
  unify(original_type, expected_type, &env);
  compilation_ctx->env = env;

  LLVMValueRef func =
      codegen_fn(specific_ast, compilation_ctx, module, builder);

  free_type_env(env);
  return func;
}

LLVMValueRef call_symbol(const char *sym_name, JITSymbol *sym, Ast *args,
                         int args_len, Type *expected_fn_type, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {
  // fprintf(stderr, "call symbol %s: args num %d %d\n", sym_name, args_len,
  //         fn_type_args_len(expected_fn_type));
  LLVMValueRef callable;
  int expected_args_len;
  Type *callable_type = sym->symbol_type;
  switch (sym->type) {
  case STYPE_FUNCTION: {
    callable = sym->val;
    expected_args_len = fn_type_args_len(sym->symbol_type);
    break;
  }

  case STYPE_GENERIC_FUNCTION: {
    // print_type(expected_fn_type);
    SpecificFns *specific_fns =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;
    callable = specific_fns_lookup(specific_fns, expected_fn_type);
    expected_args_len = fn_type_args_len(expected_fn_type);

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
      ht_set_hash(scope, sym_name, hash_string(sym_name, sym_name_len), sym);

      callable = specific_func;

      break;
    }
    break;
  }
  }

  if (args_len < expected_args_len) {
    return NULL;
  }

  if (args_len == expected_args_len) {
    LLVMValueRef app_vals[args_len];

    Type *callable_type = expected_fn_type;
    if (callable_type->kind == T_FN &&
        callable_type->data.T_FN.from->kind == T_VOID) {

      return LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable), callable,
                            (LLVMValueRef[]){}, 0, "call_func");
    }

    for (int i = 0; i < args_len; i++) {
      Ast *app_val_ast = args + i;
      Type *app_val_type = app_val_ast->md;
      app_val_type = resolve_tc_rank(app_val_type);

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);

      if (!types_equal(app_val_type, callable_type->data.T_FN.from) &&
          !(variant_contains_type(app_val_type, callable_type->data.T_FN.from,
                                  NULL))) {

        app_val = TRY_MSG(attempt_value_conversion(
                              app_val, app_val_type,
                              callable_type->data.T_FN.from, module, builder),
                          "Error: attempted type conversion failed\n");
      }
      app_vals[i] = app_val;
      callable_type = callable_type->data.T_FN.to;
    }

    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable), callable,
                          app_vals, args_len, "call_func");
  }

  // printf("\ncallable: \n");
  // LLVMDumpValue(callable);
  return NULL;
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  JITSymbol sym;

  int lookup_sym_err =
      lookup_id_ast_in_place(ast->data.AST_APPLICATION.function, ctx, &sym);

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  LLVMTypeRef tagged_union_type =
      variant_member_to_llvm_type(ast->md, ctx->env, module);

  if (tagged_union_type) {
    return tagged_union_constructor(ast, tagged_union_type, ctx, module,
                                    builder);
  }

  if (lookup_sym_err) {
    fprintf(stderr, "codegen identifier failed symbol not found in scope %d:\n",
            ctx->stack_ptr);
    print_ast_err(ast->data.AST_APPLICATION.function);
    return NULL;
  }

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  return call_symbol(sym_name, &sym, ast->data.AST_APPLICATION.args,
                     ast->data.AST_APPLICATION.len, expected_fn_type, ctx,
                     module, builder);
}
