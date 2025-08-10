#include "backend_llvm/function.h"
#include "binding.h"
#include "symbols.h"
#include "types.h"
#include "types/inference.h"
#include "types/type.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>
LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef cor_inst_struct_type();

void codegen_fn_type_arg_types(Type *fn_type, int fn_len,
                               LLVMTypeRef *llvm_param_types,
                               LLVMTypeRef *llvm_return_type_ref,
                               JITLangCtx *ctx, LLVMModuleRef module) {

  for (int i = 0; i < fn_len; i++) {

    Type *t = fn_type->data.T_FN.from;
    if (t->kind == T_FN) {
      llvm_param_types[i] = GENERIC_PTR;
    } else if (is_pointer_type(t) && t->data.T_CONS.num_args == 0) {
      llvm_param_types[i] = GENERIC_PTR;
    } else if (is_pointer_type(t)) {
      llvm_param_types[i] = LLVMPointerType(
          type_to_llvm_type(t->data.T_CONS.args[0], ctx, module), 0);
    } else {
      llvm_param_types[i] = type_to_llvm_type(t, ctx, module);
    }

    fn_type = fn_type->data.T_FN.to;
  }

  Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;

  *llvm_return_type_ref = type_to_llvm_type(return_type, ctx, module);
}

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, JITLangCtx *ctx,
                            LLVMModuleRef module) {
  // TODO: return a type for a coroutine - is this necessary?

  LLVMTypeRef llvm_param_types[fn_len];
  LLVMTypeRef llvm_fn_type;

  if (fn_len == 1 && fn_type->data.T_FN.from->kind == T_VOID) {

    LLVMTypeRef ret_type =
        type_to_llvm_type(fn_type->data.T_FN.to, ctx, module);
    llvm_fn_type = LLVMFunctionType(ret_type, NULL, 0, false);
  } else {
    LLVMTypeRef llvm_return_type_ref;
    codegen_fn_type_arg_types(fn_type, fn_len, llvm_param_types,
                              &llvm_return_type_ref, ctx, module);
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

  if (params_count == 1 && fn_type->data.T_FN.from->kind == T_VOID) {

    LLVMTypeRef ret_type =
        type_to_llvm_type(fn_type->data.T_FN.to, ctx, module);

    LLVMTypeRef llvm_fn_type = LLVMFunctionType(ret_type, NULL, 0, false);
    return get_extern_fn(name, llvm_fn_type, module);
  }

  print_ast(ast);
  for (int i = 0; i < params_count; i++) {
    Type *param_type =
        ast->data.AST_EXTERN_FN.signature_types->data.AST_LIST.items[i].md;
    printf("arg %d: ", i);
    print_type(param_type);

    llvm_param_types[i] = param_type->kind == T_FN
                              ? GENERIC_PTR
                              : type_to_llvm_type(param_type, ctx, module);
  }
  printf("---\n");
  LLVMTypeRef ret_type = type_to_llvm_type(
      ast->data.AST_EXTERN_FN.signature_types->data.AST_LIST.items[params_count]
          .md,
      ctx, module);

  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(ret_type, llvm_param_types, params_count, 0);

  LLVMValueRef val = get_extern_fn(name, llvm_fn_type, module);
  return val;
}

void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func, Type *fn_type,
                          JITLangCtx *fn_ctx) {
  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;
  sym->symbol_data.STYPE_FUNCTION.recursive_ref = true;

  ht *scope = fn_ctx->frame->table;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef body;

  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    Ast *stmt = ast->data.AST_LAMBDA.body;

    stmt->is_body_tail = true;
    body = codegen(stmt, fn_ctx, module, builder);

  } else {
    int len = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
    for (int i = 0; i < len; i++) {

      Ast *stmt = ast->data.AST_LAMBDA.body->data.AST_BODY.stmts[i];

      if (i == 0 && stmt->tag == AST_STRING) {
        continue;
      }

      if (i == len - 1) {
        stmt->is_body_tail = true;
      }
      body = codegen(stmt, fn_ctx, module, builder);
      if (body == NULL && i == 1 && stmt->tag == AST_APPLICATION) {
        // print_ast(stmt);
        // print_type(stmt->data.AST_APPLICATION.args[0].md);
        // print_type(stmt->data.AST_APPLICATION.args[1].md);
      }
    }
  }
  return body;
}

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  if (ast->tag == AST_EXTERN_FN) {
    return codegen_extern_fn(ast, ctx, module, builder);
  }

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }
  Type *fn_type = ast->md;

  int fn_len = ast->data.AST_LAMBDA.len;
  int num_closure_vars = ast->data.AST_LAMBDA.num_closure_free_vars;

  LLVMTypeRef prototype =
      codegen_fn_type(fn_type, fn_len + num_closure_vars, ctx, module);

  START_FUNC(module, is_anon ? "anonymous_func" : fn_name.chars, prototype)

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  if (!is_anon) {
    add_recursive_fn_ref(fn_name, func, fn_type, &fn_ctx);
  }

  AST_LIST_ITER(ast->data.AST_LAMBDA.params, ({
                  Ast *param_ast = l->ast;
                  Type *param_type = fn_type->data.T_FN.from;

                  if (param_type->kind == T_VAR) {
                    param_type = resolve_type_in_env(param_type, ctx->env);
                  }

                  LLVMValueRef param_val = LLVMGetParam(func, i);

                  if (param_type->kind == T_FN) {
                    const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
                    int id_len = param_ast->data.AST_IDENTIFIER.length;
                    LLVMTypeRef llvm_type =
                        type_to_llvm_type(param_type, ctx, module);

                    JITSymbol *sym = new_symbol(STYPE_FUNCTION, param_type,
                                                param_val, llvm_type);

                    ht_set_hash(fn_ctx.frame->table, id_chars,
                                hash_string(id_chars, id_len), sym);

                  } else {
                    codegen_pattern_binding(param_ast, param_val, param_type,
                                            &fn_ctx, module, builder);
                  }
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
  // LLVMDumpValue(func);
  return func;
}

static Substitution *create_fn_arg_subst(Substitution *subst, Type *gen,
                                         Type *spec) {

  if (!spec) {
    return NULL;
  }

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

TypeEnv *codegen_bind_in_env(TypeEnv *env, Type *f, Type *t) {
  switch (f->kind) {
  case T_VAR: {
    return env_extend(env, f->data.T_VAR, t);
  }
  case T_CONS: {
    for (int i = 0; i < f->data.T_CONS.num_args; i++) {
      env = codegen_bind_in_env(env, f->data.T_CONS.args[i],
                                t->data.T_CONS.args[i]);
    }
    break;
  }
  default: {
  }
  }
  return env;
}

TypeEnv *create_env_from_subst(TypeEnv *env, Substitution *subst) {
  if (subst == NULL) {
    return env;
  }
  Type *f = subst->from;
  Type *t = subst->to;
  env = codegen_bind_in_env(env, f, t);
  return create_env_from_subst(env, subst->next);
}

TypeEnv *create_env_for_generic_fn(TypeEnv *env, Type *generic_type,
                                   Type *specific_type) {

  Substitution *subst = NULL;

  TypeConstraint *constraints = NULL;
  while (generic_type->kind == T_FN) {
    Type *gen = generic_type->data.T_FN.from;
    Type *spec = specific_type->data.T_FN.from;
    constraints = constraints_extend(constraints, gen, spec);

    specific_type = specific_type->data.T_FN.to;
    generic_type = generic_type->data.T_FN.to;
  }

  subst = solve_constraints(constraints);

  env = create_env_from_subst(env, subst);

  return env;
}

LLVMValueRef create_builtin_func_wrapper(Type *specific_type, JITSymbol *sym,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  int args_len = fn_type_args_len(specific_type);
  LLVMTypeRef fn_type = codegen_fn_type(specific_type, args_len, ctx, module);

  Type *ft = specific_type;
  Ast args[args_len];
  int i = 0;

  while (i < args_len) {
    args[i] = (Ast){AST_GET_ARG, .data = {.AST_GET_ARG = {.i = i}},
                    .md = ft->data.T_FN.from};
    ft = ft->data.T_FN.to;
    i++;
  }

  Ast app = {
      AST_APPLICATION,
      .data = {.AST_APPLICATION = {.function = &(Ast){AST_IDENTIFIER,
                                                      .md = specific_type},
                                   .args = args,
                                   .len = args_len}},
      .md = fn_return_type(specific_type)};

  START_FUNC(module, "anonymous_func", fn_type);
  LLVMValueRef res = sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
      &app, ctx, module, builder);
  LLVMBuildRet(builder, res);

  END_FUNC;

  return func;
}

LLVMValueRef compile_specific_fn(Type *specific_type, JITSymbol *sym,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  JITLangCtx compilation_ctx = *ctx;
  if (!sym->symbol_data.STYPE_GENERIC_FUNCTION.ast) {
    if (sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
      return create_builtin_func_wrapper(specific_type, sym, ctx, module,
                                         builder);
    }

    return NULL;
  }

  Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;
  fn_ast.md = specific_type;

  Type *generic_type = sym->symbol_type;
  compilation_ctx.stack_ptr = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
  compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

  TypeEnv *env = sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env;

  // TypeConstraint *constraints = NULL;
  // Type *f;
  // for (f = generic_type; f->kind == T_FN; f = f->data.T_FN.to) {
  //   Type *ff = f->data.T_FN.from;
  //   if (is_generic(f->data.T_FN.from)) {
  //     Type *r = resolve_type_in_env(ff, ctx->env);
  //     if (r) {
  //       constraints = constraints_extend(constraints, ff, r);
  //     }
  //   }
  // }
  //
  // if (is_generic(f)) {
  //   Type *r = resolve_type_in_env(f, ctx->env);
  //   if (r) {
  //     constraints = constraints_extend(constraints, f, r);
  //   }
  // }
  //
  // Substitution *subst = NULL;
  // subst = solve_constraints(constraints);
  // env = create_env_from_subst(env, subst);

  compilation_ctx.env =
      create_env_for_generic_fn(env, generic_type, specific_type);

  while (specific_type->kind == T_FN) {
    Type *f = specific_type->data.T_FN.from;
    if (is_generic(f)) {
      Type *r = resolve_type_in_env(f, ctx->env);
      if (r) {
        compilation_ctx.env = codegen_bind_in_env(compilation_ctx.env, f, r);
      }
    }

    specific_type = specific_type->data.T_FN.to;
  }

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

  // printf("if following gets compiled it gets cached\n");
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
