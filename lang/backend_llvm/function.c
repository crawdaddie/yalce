#include "backend_llvm/function.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "types/inference.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, TypeEnv *env,
                            LLVMModuleRef module) {

  LLVMTypeRef llvm_param_types[fn_len];
  LLVMTypeRef llvm_fn_type;

  if (fn_len == 1 && fn_type->data.T_FN.from->kind == T_VOID) {

    LLVMTypeRef ret_type =
        type_to_llvm_type(fn_type->data.T_FN.to, env, module);
    llvm_fn_type = LLVMFunctionType(ret_type, NULL, 0, false);
  } else {
    for (int i = 0; i < fn_len; i++) {
      Type *t = fn_type->data.T_FN.from;
      llvm_param_types[i] = type_to_llvm_type(t, env, module);

      if (t->kind == T_FN) {
        llvm_param_types[i] = LLVMPointerType(llvm_param_types[i], 0);
      } else if (is_pointer_type(t)) {
        llvm_param_types[i] = LLVMPointerType(
            type_to_llvm_type(t->data.T_CONS.args[0], env, module), 0);
      }

      fn_type = fn_type->data.T_FN.to;
    }
    Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
    LLVMTypeRef llvm_return_type_ref =
        type_to_llvm_type(return_type, env, module);

    llvm_fn_type =
        LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
  }

  return llvm_fn_type;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);

  int params_count =
      ast->data.AST_EXTERN_FN.signature_types->data.AST_LIST.len - 1;

  LLVMTypeRef llvm_param_types[params_count];

  Type *fn_type = ast->md;

  LLVMTypeRef llvm_fn_type =
      codegen_fn_type(fn_type, params_count, ctx->env, module);
  return get_extern_fn(name, llvm_fn_type, module);
}

void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func, Type *fn_type,
                          JITLangCtx *fn_ctx) {
  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;

  ht *scope = fn_ctx->frame->table;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}
LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef body;
  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    body = codegen(ast->data.AST_LAMBDA.body, fn_ctx, module, builder);
  } else {
    for (int i = 0; i < ast->data.AST_LAMBDA.body->data.AST_BODY.len; i++) {

      Ast *stmt = ast->data.AST_LAMBDA.body->data.AST_BODY.stmts[i];
      if (i == 0 && stmt->tag == AST_STRING) {
        continue;
      }

      body = codegen(stmt, fn_ctx, module, builder);
    }
  }
  return body;
}

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }
  Type *fn_type = ast->md;

  int fn_len = ast->data.AST_LAMBDA.len;

  LLVMTypeRef prototype = codegen_fn_type(fn_type, fn_len, ctx->env, module);

  LLVMValueRef func = LLVMAddFunction(
      module, is_anon ? "anonymous_func" : fn_name.chars, prototype);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  if (!is_anon) {
    add_recursive_fn_ref(fn_name, func, fn_type, &fn_ctx);
  }

  for (int i = 0; i < fn_len; i++) {
    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    Type *param_type = fn_type->data.T_FN.from;

    LLVMValueRef param_val = LLVMGetParam(func, i);

    if (param_type->kind == T_FN) {
      const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
      int id_len = param_ast->data.AST_IDENTIFIER.length;
      LLVMTypeRef llvm_type = type_to_llvm_type(param_type, ctx->env, module);

      JITSymbol *sym =
          new_symbol(STYPE_FUNCTION, param_type, param_val, llvm_type);

      ht_set_hash(fn_ctx.frame->table, id_chars, hash_string(id_chars, id_len),
                  sym);

    } else {
      codegen_pattern_binding(param_ast, param_val, param_type, &fn_ctx, module,
                              builder);
    }
    fn_type = fn_type->data.T_FN.to;
  }

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMBuildRet(builder, body);
  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);
  return func;
}

static Substitution *create_fn_arg_subst(Substitution *subst, Type *gen,
                                         Type *spec) {

  if (gen->kind == T_VAR) {
    subst = substitutions_extend(subst, gen, spec);
    return subst;
  }

  if (gen->kind == T_CONS ||
      gen->kind == T_TYPECLASS_RESOLVE && gen->kind == spec->kind) {
    for (int i = 0; i < gen->data.T_CONS.num_args; i++) {
      Type *gt = gen->data.T_CONS.args[i];
      Type *st = spec->data.T_CONS.args[i];
      subst = create_fn_arg_subst(subst, gt, st);
    }
    return subst;
  }

  if (gen->kind == T_TYPECLASS_RESOLVE && gen->kind != spec->kind) {
    for (int i = 0; i < gen->data.T_CONS.num_args; i++) {
      Type *gt = gen->data.T_CONS.args[i];
      Type *st = spec;
      subst = create_fn_arg_subst(subst, gt, st);
    }
    return subst;
  }

  if (gen->kind == T_FN) {
    while (gen->kind == T_FN) {
      Type *gt = gen->data.T_FN.from;
      Type *st = spec->data.T_FN.from;
      subst = create_fn_arg_subst(subst, gt, st);
      gen = gen->data.T_FN.to;
      spec = spec->data.T_FN.to;
    }

    Type *gt = gen;
    Type *st = spec;
    subst = create_fn_arg_subst(subst, gt, st);
    return subst;
  }
  return subst;
}

static TypeEnv *subst_fn_arg(TypeEnv *env, Substitution *subst, Type *arg) {

  if (arg->kind == T_VAR) {
    env = env_extend(env, arg->data.T_VAR, apply_substitution(subst, arg));
    return env;
  }

  if (arg->kind == T_CONS || arg->kind == T_TYPECLASS_RESOLVE) {
    for (int i = 0; i < arg->data.T_CONS.num_args; i++) {
      Type *t = arg->data.T_CONS.args[i];
      env = subst_fn_arg(env, subst, t);
    }
    return env;
  }

  if (arg->kind == T_FN) {
    while (arg->kind == T_FN) {
      Type *t = arg->data.T_FN.from;
      env = subst_fn_arg(env, subst, t);
      arg = arg->data.T_FN.to;
    }
    env = subst_fn_arg(env, subst, arg);
    return env;
  }
  return env;
}

TypeEnv *create_env_for_generic_fn(TypeEnv *env, Type *generic_type,
                                   Type *specific_type) {
  Substitution *subst = NULL;

  Type *gen;
  Type *spec;

  subst = create_fn_arg_subst(subst, generic_type, specific_type);

  // return types:
  gen = generic_type;
  spec = specific_type;
  subst = substitutions_extend(subst, gen, spec);

  Type *_gen = generic_type;
  while (_gen->kind == T_FN) {
    Type *f = _gen->data.T_FN.from;
    env = subst_fn_arg(env, subst, f);

    _gen = _gen->data.T_FN.to;
  }

  Type *f = _gen;
  env = subst_fn_arg(env, subst, f);

  return env;
}

LLVMValueRef compile_specific_fn(Type *specific_type, JITSymbol *sym,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  JITLangCtx compilation_ctx = *ctx;

  Type *generic_type = sym->symbol_type;
  compilation_ctx.stack_ptr = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
  compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

  compilation_ctx.env = create_env_for_generic_fn(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env, generic_type,
      specific_type);

  Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;
  fn_ast.md = specific_type;
  LLVMValueRef func = codegen_fn(&fn_ast, &compilation_ctx, module, builder);

  return func;
}

LLVMValueRef specific_fns_lookup(SpecificFns *fns, Type *key) {
  while (fns) {
    if (fn_types_match(key, fns->arg_types_key)) {
      return fns->func;
    }
    fns = fns->next;
  }
  return NULL;
}

SpecificFns *specific_fns_extend(SpecificFns *fns, Type *key,
                                 LLVMValueRef func) {
  SpecificFns *new_fns = malloc(sizeof(SpecificFns));
  new_fns->arg_types_key = key;
  new_fns->func = func;
  new_fns->next = fns;
  return new_fns;
}

LLVMValueRef get_specific_callable(JITSymbol *sym, Type *expected_fn_type,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  LLVMValueRef func = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type);

  if (func) {
    return func;
  }

  LLVMValueRef specific_fn =
      compile_specific_fn(expected_fn_type, sym, ctx, module, builder);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
      specific_fns_extend(sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns,
                          expected_fn_type, specific_fn);

  return specific_fn;
}

/*

LLVMValueRef codegen_curry_fn(Ast *curry, LLVMValueRef func,
                              unsigned int total_params_len, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  int saved_args_len = curry->data.AST_APPLICATION.len;
  int curried_fn_len = total_params_len - saved_args_len;

  Type *fn_type = curry->md;
  LLVMValueRef curried_func = codegen_fn_proto(
      fn_type, curried_fn_len, "curried_fn", ctx, module, builder);

  if (curried_func == NULL) {
    return NULL;
  }

  LLVMValueRef app_vals[total_params_len];
  for (int i = 0; i < saved_args_len; i++) {
    app_vals[i] =
        codegen(curry->data.AST_APPLICATION.args + i, ctx, module, builder);
  }
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(curried_func, "entry");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  JITLangCtx fn_ctx = ctx_push(*ctx);

  LLVMValueRef app_vals[total_params_len];
  for (int i = 0; i < saved_args_len; i++) {
    Ast *app_val_ast = curry->data.AST_APPLICATION.args + i;
    app_vals[i] = codegen(app_val_ast, &fn_ctx, module, builder);
  }

  for (int i = 0; i < curried_fn_len; i++) {
    LLVMValueRef param_val = LLVMGetParam(curried_func, i);
    app_vals[saved_args_len + i] = param_val;
  }

  LLVMValueRef body =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, app_vals,
                     total_params_len, "call_func");

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  return curried_func;
}
*/
