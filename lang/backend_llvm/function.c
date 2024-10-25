#include "backend_llvm/function.h"
#include "backend_llvm/common.h"
#include "backend_llvm/match.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "backend_llvm/variant.h"
#include "list.h"
#include "serde.h"
#include "strings.h"
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

  // Create function type with return.
  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
  return llvm_fn_type;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  // printf("codegen extern fn\n");
  // print_ast(ast);

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);

  int params_count = fn_type_args_len(ast->md);
  LLVMTypeRef llvm_param_types[params_count];
  Type *fn_type = ast->md;

  LLVMTypeRef llvm_fn_type;
  if (params_count == 1 && fn_type->data.T_FN.from->kind == T_VOID) {
    LLVMTypeRef ret_type =
        type_to_llvm_type(fn_type->data.T_FN.to, ctx->env, module);
    llvm_fn_type = LLVMFunctionType(ret_type, NULL, 0, false);
  } else {
    for (int i = 0; i < params_count; i++) {
      Type *t = fn_type->data.T_FN.from;
      llvm_param_types[i] = type_to_llvm_type(t, ctx->env, module);
      fn_type = fn_type->data.T_FN.to;
    }

    LLVMTypeRef ret_type = type_to_llvm_type(fn_type, ctx->env, module);

    llvm_fn_type =
        LLVMFunctionType(ret_type, llvm_param_types, params_count, false);
  }
  return get_extern_fn(name, llvm_fn_type, module);
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
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    // return codegen_anonymous_fn(ast, ctx, module, builder);
    is_anon = true;
  }
  Type *fn_type = ast->md;

  int fn_len = ast->data.AST_LAMBDA.len;
  LLVMTypeRef prototype = fn_prototype(ast->md, fn_len, ctx->env, module);

  LLVMValueRef func = LLVMAddFunction(
      module, is_anon ? "anonymous_func" : fn_name.chars, prototype);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  JITLangCtx fn_ctx = ctx_push(*ctx);
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
          new_symbol(STYPE_LOCAL_VAR, param_type, param_val, llvm_type);

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

    } else {
      match_values(param_ast, param_val, param_type, &fn_ctx, module, builder);
    }

    fn_type = fn_type->data.T_FN.to;
  }

  if (!is_anon) {
    Ast *fn_id = Ast_new(AST_IDENTIFIER);
    fn_id->data.AST_IDENTIFIER.value = fn_name.chars;
    fn_id->data.AST_IDENTIFIER.length = fn_name.length;
    JITSymbol *fn_ref = lookup_id_in_current_scope(fn_id, &fn_ctx);
  }

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  int blen = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  if (body == NULL &&
      ast->data.AST_LAMBDA.body->data.AST_BODY.stmts[blen - 1]->tag !=
          AST_VOID) {
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

LLVMValueRef create_new_specific_fn(int len, Ast *fn_ast, Type *original_type,
                                    Type *expected_type, Type *ret_type,
                                    JITLangCtx *compilation_ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
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
    } else if (of->kind == T_CONS) {
      for (int i = 0; i < of->data.T_CONS.num_args; i++) {
        Type *ofc = of->data.T_CONS.args[i];
        Type *efc = ef->data.T_CONS.args[i];
        if (ofc->kind == T_VAR && !(env_lookup(_env, ofc->data.T_VAR))) {
          _env = env_extend(_env, ofc->data.T_VAR, efc);
        }
      }
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
LLVMValueRef get_specific_callable(JITSymbol *sym, const char *sym_name,
                                   Type *expected_fn_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  SpecificFns *specific_fns =
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;
  LLVMValueRef callable = specific_fns_lookup(specific_fns, expected_fn_type);

  if (callable) {
    return callable;
  }

  Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

  JITLangCtx compilation_ctx = {
      ctx->stack,
      sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr,
      .env = ctx->env,
  };

  LLVMValueRef specific_func = create_new_specific_fn(
      fn_ast->data.AST_LAMBDA.len, fn_ast, sym->symbol_type, expected_fn_type,
      fn_return_type(expected_fn_type), &compilation_ctx, module, builder);

  sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
      specific_fns_extend(specific_fns, expected_fn_type, specific_func);

  ht *scope = compilation_ctx.stack + compilation_ctx.stack_ptr;
  int sym_name_len = strlen(sym_name);

  ht_set_hash(scope, sym_name, hash_string(sym_name, sym_name_len),
              (void *)sym);

  callable = specific_func;
  return callable;
}

LLVMValueRef get_specific_curried_callable(JITSymbol *sym, const char *sym_name,
                                           Type *expected_fn_type,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  SpecificFns *specific_fns =
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;
  LLVMValueRef callable = specific_fns_lookup(specific_fns, expected_fn_type);

  if (callable) {
    return callable;
  }

  Ast *application = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;
  Type *original_fn_type = application->data.AST_APPLICATION.function->md;
  int total_args_length = fn_type_args_len(original_fn_type);
  int new_args_length = application->data.AST_APPLICATION.len;

  Type *f = original_fn_type;
  Type *args[total_args_length + 1];

  int arg = 0;
  while (arg <= total_args_length - new_args_length) {
    args[arg] = f->data.T_FN.from;
    f = f->data.T_FN.to;
    arg++;
  }

  f = expected_fn_type;
  while (arg < total_args_length) {
    args[arg] = f->data.T_FN.from;
    arg++;
    f = f->data.T_FN.to;
  }

  Type *ret = f;
  Type *full_type = create_type_multi_param_fn(total_args_length, args, ret);

  JITSymbol *original_sym =
      lookup_id_ast(application->data.AST_APPLICATION.function, ctx);

  const char *original_sym_name =
      application->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  printf("compiling original callable (%s)\n", sym_name);
  LLVMValueRef original_callable = get_specific_callable(
      original_sym, original_sym_name, full_type, ctx, module, builder);

  LLVMTypeRef proto =
      fn_prototype(expected_fn_type, new_args_length, ctx->env, module);

  LLVMDumpType(proto);
  printf("<-- curry proto \n");

  callable = LLVMAddFunction(module, "curried_fn", proto);

  return NULL;
}

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);
LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (!to_type->constructor) {
    return val;
  }

  ConsMethod constructor = to_type->constructor;
  return constructor(val, from_type, module, builder);
}

LLVMValueRef call_symbol(const char *sym_name, JITSymbol *sym, Ast *args,
                         int args_len, Type *expected_fn_type, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef callable;
  int expected_args_len = fn_type_args_len(expected_fn_type);
  Type *callable_type = sym->symbol_type;
  LLVMTypeRef llvm_callable_type;

  switch (sym->type) {
  case STYPE_FUNCTION: {
    callable = sym->val;
    callable_type = sym->symbol_type;
    llvm_callable_type = LLVMGlobalGetValueType(callable);
    break;
  }

  case STYPE_LOCAL_VAR: {
    callable = sym->val;
    callable_type = sym->symbol_type;
    llvm_callable_type = sym->llvm_type;
    break;
  }

  case STYPE_GENERIC_FUNCTION: {
    Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    if (fn_ast->tag == AST_APPLICATION) {
      callable = get_specific_curried_callable(sym, sym_name, expected_fn_type,
                                               ctx, module, builder);
    } else {
      callable = get_specific_callable(sym, sym_name, expected_fn_type, ctx,
                                       module, builder);
    }

    callable_type = expected_fn_type;
    llvm_callable_type = LLVMGlobalGetValueType(callable);
    break;
  }
  }

  if (args_len < expected_args_len) {
    // printf("should curry %s\n", sym_name);
    // TODO : currying
    return NULL;
  }

  if (args_len == expected_args_len) {
    LLVMValueRef app_vals[args_len];

    if (callable_type->kind == T_FN &&
        callable_type->data.T_FN.from->kind == T_VOID) {
      // fn void type
      return LLVMBuildCall2(builder, llvm_callable_type, callable, NULL, 0,
                            "call_func");
    }

    for (int i = 0; i < args_len; i++) {
      Ast *app_val_ast = args + i;
      Type *app_val_type = app_val_ast->md;

      if (app_val_type->kind == T_VAR) {
        app_val_type = env_lookup(ctx->env, app_val_type->data.T_VAR);
      }
      app_val_ast->md = app_val_type;

      app_val_type = resolve_tc_rank(app_val_type);

      if (((Type *)app_val_ast->md)->kind == T_FN &&
          is_generic(app_val_ast->md)) {
        app_val_ast->md = callable_type->data.T_FN.from;
      }

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);
      Type *expected_type = callable_type->data.T_FN.from;

      app_val = handle_type_conversions(app_val, app_val_type, expected_type,
                                        module, builder);
      app_vals[i] = app_val;

      callable_type = callable_type->data.T_FN.to;
    }

    return LLVMBuildCall2(builder, llvm_callable_type, callable, app_vals,
                          args_len, "call_func");
  }

  return NULL;
}
Type *resolve_tcs(Type *t) {
  if (t->kind != T_TYPECLASS_RESOLVE) {
    return t;
  }
  const char *tc_name = t->data.T_TYPECLASS_RESOLVE.comparison_tc;
  if (types_equal(t->data.T_TYPECLASS_RESOLVE.dependencies[0],
                  t->data.T_TYPECLASS_RESOLVE.dependencies[1])) {
    return t->data.T_TYPECLASS_RESOLVE.dependencies[0];
  }
  return t;
}

LLVMValueRef call_binop(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *ltype = ast->data.AST_APPLICATION.args->md;

  if (ltype->kind == T_VAR) {
    ltype = env_lookup(ctx->env, ltype->data.T_VAR);
  }
  ltype = resolve_tcs(ltype);

  Type *rtype = (ast->data.AST_APPLICATION.args + 1)->md;

  if (rtype->kind == T_VAR) {
    rtype = env_lookup(ctx->env, rtype->data.T_VAR);
  }
  rtype = resolve_tcs(rtype);

  const char *binop_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
  //
  // printf("call binop\n");
  // print_type(ltype);
  // print_type(rtype);

  LLVMValueRef lval =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef rval =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  Type *res_type = ast->md;

  if (is_generic(res_type)) {
    res_type = resolve_generic_type(res_type, ctx->env);
  }

  Method *method = get_binop_method(binop_name, ltype, rtype);
  LLVMBinopMethod llvm_method = method->method;

  Type *expected_l = method->signature->data.T_FN.from;
  Type *expected_r = method->signature->data.T_FN.to->data.T_FN.from;

  lval = handle_type_conversions(lval, ltype, expected_l, module, builder);
  rval = handle_type_conversions(rval, rtype, expected_r, module, builder);

  return llvm_method(lval, rval, module, builder);
}

LLVMValueRef call_array_fn(Ast *ast, JITSymbol *sym, const char *sym_name,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  int fn_args_len = fn_type_args_len(sym->symbol_type);

  if (ast->data.AST_APPLICATION.len != fn_args_len) {
    return NULL;
  }

  if (strcmp(sym_name, "array_at") == 0) {

    LLVMValueRef array_ptr =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
    LLVMValueRef index =
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
    LLVMTypeRef el_type = type_to_llvm_type(ast->md, ctx->env, module);

    return codegen_array_at(array_ptr, index, el_type, module, builder);
  }

  if (strcmp(sym_name, "array_size") == 0) {

    LLVMValueRef array =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
    return codegen_get_array_size(builder, array);
  }

  if (strcmp(sym_name, "array_data_ptr") == 0) {

    LLVMValueRef array =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
    Type *array_type = ast->data.AST_APPLICATION.args->md;
    Type *el_type = array_type->data.T_CONS.args[0];
    print_type(el_type);
    return codegen_get_array_data_ptr(
        builder, type_to_llvm_type(el_type, ctx->env, module), array);
  }

  if (strcmp(sym_name, "array_new") == 0) {

    LLVMValueRef array_size =
        codegen(&ast->data.AST_APPLICATION.args[0], ctx, module, builder);

    LLVMValueRef array_item =
        codegen(&ast->data.AST_APPLICATION.args[1], ctx, module, builder);

    return codegen_array_init(array_size, array_item, ctx, module, builder);
  }

  if (strcmp(sym_name, "array_incr") == 0) {
    Type *array_type = ast->md;
    LLVMTypeRef el_type =
        type_to_llvm_type(array_type->data.T_CONS.args[0], ctx->env, module);

    LLVMValueRef array =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
    return codegen_array_increment(array, el_type, builder);
  }

  if (strcmp(sym_name, "array_slice") == 0) {
    // printf("call array slice\n");

    Type *array_type = (ast->data.AST_APPLICATION.args + 2)->md;
    // print_type(array_type);
    // print_ast(ast);

    // LLVMTypeRef el_type =
    //     type_to_llvm_type(array_type->data.T_CONS.args[0], ctx->env, module);
    LLVMTypeRef el_type = LLVMInt8Type();
    //
    LLVMValueRef start =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

    LLVMValueRef end =
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
    LLVMValueRef array =
        codegen(ast->data.AST_APPLICATION.args + 2, ctx, module, builder);

    return codegen_array_slice(array, el_type, start, end, builder);
  }

  if (strcmp(sym_name, "array_to_list") == 0) {
    printf("array to list call\n");
    return NULL;
  }

  return NULL;
}
LLVMValueRef call_deref_fn(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *ptr_type = ast->data.AST_APPLICATION.args->md;
  Type *pointed_to_type = ptr_type->data.T_CONS.args[0];
  LLVMTypeRef deref_type = type_to_llvm_type(pointed_to_type, ctx->env, module);
  LLVMTypeRef _ptr_type = LLVMPointerType(deref_type, 0);
  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  // Bitcast the input pointer to the correct pointer type if necessary
  LLVMValueRef casted_ptr =
      LLVMBuildBitCast(builder, val, _ptr_type, "casted_ptr");

  // Create a load instruction to dereference the pointer
  return LLVMBuildLoad2(builder, deref_type, casted_ptr, "dereferenced_value");
}
LLVMValueRef codegen_cons(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *sym_type = ast->md;
  // printf("cons\n");
  // print_type(sym_type);
  // TODO: if sym_type->constructor != NULL - use specific constructor fn
  // otherwise codegen cons is essentially a struct / tuple
  //
  LLVMTypeRef llvm_type = type_to_llvm_type(sym_type, ctx->env, module);
  LLVMValueRef v = LLVMGetUndef(llvm_type);
  int len = ast->data.AST_APPLICATION.len;
  Ast *args = ast->data.AST_APPLICATION.args;
  for (int i = 0; i < len; i++) {
    Ast *arg = args + i;
    LLVMValueRef arg_val = codegen(arg, ctx, module, builder);

    v = LLVMBuildInsertValue(builder, v, arg_val, i, "insert_struct_member");
  }
  return v;
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  LLVMTypeRef tagged_union_type =
      variant_member_to_llvm_type(ast->md, ctx->env, module);

  if (tagged_union_type) {
    return tagged_union_constructor(ast, tagged_union_type, ctx, module,
                                    builder);
  }

  if (strcmp("deref", sym_name) == 0) {
    return call_deref_fn(ast, sym, ctx, module, builder);
  }

  Type *builtin_binop = get_builtin_type(sym_name);
  if (builtin_binop) {
    return call_binop(ast, sym, ctx, module, builder);
  }

  if (strncmp("array_", sym_name, 6) == 0) {
    LLVMValueRef res = call_array_fn(ast, sym, sym_name, ctx, module, builder);
    if (res) {
      return res;
    }
  }

  if (strncmp("string_add", sym_name, 10) == 0) {
    LLVMValueRef res = codegen_string_add(
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder),
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder), ctx,
        module, builder);

    if (res) {
      return res;
    }
  }

  Type *sym_type = ast->data.AST_APPLICATION.function->md;
  // printf("application\n");
  // print_type(sym_type);
  if (sym_type->kind == T_CONS && !is_generic(sym_type)) {
    return codegen_cons(ast, ctx, module, builder);
  }

  if (!sym) {
    fprintf(stderr, "codegen identifier failed symbol not found in scope %d:\n",
            ctx->stack_ptr);
    print_ast_err(ast->data.AST_APPLICATION.function);
    return NULL;
  }

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  return call_symbol(sym_name, sym, ast->data.AST_APPLICATION.args,
                     ast->data.AST_APPLICATION.len, expected_fn_type, ctx,
                     module, builder);
}
