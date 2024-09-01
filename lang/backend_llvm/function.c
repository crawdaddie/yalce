#include "backend_llvm/function.h"
#include "backend_llvm/function_currying.h"
#include "backend_llvm/match_values.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/types.h"
#include "serde.h"
#include "types/type.h"

#include "backend_llvm/util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef find_fn_variant(FnVariants *variants, Type *type);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

SpecificFns *specific_fns_extend(SpecificFns *list, const char *serialized_type,
                                 LLVMValueRef func) {
  SpecificFns *new_specific_fn = malloc(sizeof(SpecificFns));

  new_specific_fn->serialized_type = serialized_type;
  new_specific_fn->func = func;
  new_specific_fn->next = list;
  return new_specific_fn;
};

LLVMValueRef specific_fns_lookup(SpecificFns *list,
                                 const char *serialized_type) {
  while (list) {
    if (strcmp(serialized_type, list->serialized_type) == 0) {
      return list->func;
    }
    list = list->next;
  }
  return NULL;
};
static void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func,
                                 Type *fn_type, JITLangCtx *fn_ctx) {

  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = LLVMTypeOf(func),
                   .type = STYPE_FUNCTION,
                   .val = func,
                   .symbol_data = {.STYPE_FUNCTION = {.fn_type = fn_type}},
                   .symbol_type = fn_type};

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, v);
}

TypeEnv *create_replacement_env(Type *generic_fn_type, Type *specific_fn_type,
                                TypeEnv *env) {
  Type *l = generic_fn_type;
  Type *r = specific_fn_type;

  while (l->kind == T_FN && r->kind == T_FN) {
    Type *from_l = l->data.T_FN.from;
    Type *from_r = r->data.T_FN.from;

    // Check if left type is T_VAR and right type is not T_VAR
    if (from_l->kind == T_VAR && from_r->kind != T_VAR) {
      const char *name = from_l->data.T_VAR;
      if (!env_lookup(env, name)) {
        env = env_extend(env, name, from_r);
      }
    }

    // Move to the next level in the function types
    l = l->data.T_FN.to;
    r = r->data.T_FN.to;
  }

  // Handle the final return type
  if (l->kind == T_VAR && r->kind != T_VAR) {

    const char *name = l->data.T_VAR;
    if (!env_lookup(env, name)) {
      env = env_extend(env, name, r);
    }
  }

  return env;
}
LLVMTypeRef fn_proto_type(Type *fn_type, int fn_len, TypeEnv *env) {

  // Create argument list.
  LLVMTypeRef llvm_param_types[fn_len];

  for (int i = 0; i < fn_len; i++) {
    llvm_param_types[i] = type_to_llvm_type(fn_type->data.T_FN.from, env);
    fn_type = fn_type->data.T_FN.to;
  }

  Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
  LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(return_type, env);

  // Create function type with return.
  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
  return llvm_fn_type;
}

LLVMValueRef codegen_fn_proto(Type *fn_type, int fn_len, const char *fn_name,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  // Create function type with return.
  LLVMTypeRef llvm_fn_type = fn_proto_type(fn_type, fn_len, ctx->env);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, fn_name, llvm_fn_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  return func;
}

// compile an AST_LAMBDA node into a function
LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  // Generate the prototype first.
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  Type *fn_type = ast->md;
  int fn_len = ast->data.AST_LAMBDA.len;

  LLVMValueRef func =
      codegen_fn_proto(fn_type, fn_len, fn_name.chars, ctx, module, builder);

  if (func == NULL) {
    return NULL;
  }

  // Create basic block.
  JITLangCtx fn_ctx = ctx_push(*ctx);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  for (int i = 0; i < fn_len; i++) {
    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    Type *param_type = fn_type->data.T_FN.from;

    LLVMValueRef param_val = LLVMGetParam(func, i);
    LLVMValueRef _true = LLVMConstInt(LLVMInt1Type(), 1, 0);
    match_values(param_ast, param_val, param_type, &_true, &fn_ctx, module,
                 builder);
    fn_type = fn_type->data.T_FN.to;
  }

  add_recursive_fn_ref(fn_name, func, fn_type, &fn_ctx);

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  return func;
}

static bool is_void_fn(LLVMValueRef fn) { return LLVMCountParams(fn) == 0; }
static bool is_void_application(LLVMValueRef fn, Ast *ast) {
  int app_len = ast->data.AST_APPLICATION.len;
  return is_void_fn(fn) && app_len == 1 &&
         ast->data.AST_APPLICATION.args[0].tag == AST_VOID;
}

static LLVMTypeRef llvm_type_id(Ast *id, TypeEnv *env) {
  if (id->tag == AST_VOID) {
    return LLVMVoidType();
  }

  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  Type *lookup_type = get_type(env, id);
  LLVMTypeRef t = type_to_llvm_type(lookup_type, env);
  return t;
}

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);
  int params_count = ast->data.AST_EXTERN_FN.len - 1;

  Ast *signature_types = ast->data.AST_EXTERN_FN.signature_types;
  if (params_count == 0) {

    LLVMTypeRef ret_type = llvm_type_id(signature_types, ctx->env);
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, NULL, 0, false);

    LLVMValueRef func = LLVMAddFunction(module, name, fn_type);
    return func;
  }

  LLVMTypeRef llvm_param_types[params_count];
  for (int i = 0; i < params_count; i++) {
    llvm_param_types[i] = llvm_type_id(signature_types + i, ctx->env);
  }

  LLVMTypeRef ret_type = llvm_type_id(signature_types + params_count, ctx->env);
  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, llvm_param_types, params_count, false);

  LLVMValueRef func = get_extern_fn(name, fn_type, module);
  return func;
}

static TypeSerBuf *get_specific_fn_args_key(size_t len, Ast *args) {
  TypeSerBuf *specific_fn_buf = create_type_ser_buffer(10);

  for (size_t i = 0; i < len; i++) {
    serialize_type(args[i].md, specific_fn_buf);
  }
  return specific_fn_buf;
}

static Type *create_specific_fn_type(size_t len, Ast *args, Type *ret_type) {

  Type *specific_arg_types[len];
  for (size_t i = 0; i < len; i++) {
    specific_arg_types[i] = (Type *)deep_copy_type(args[i].md);
  }
  return create_type_multi_param_fn(len, specific_arg_types,
                                    deep_copy_type(ret_type));
}

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

LLVMValueRef create_new_specific_fn(int len, Ast *args, Ast *fn_ast,
                                    Type *ret_type, JITLangCtx *compilation_ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  Type *specific_fn_type = create_specific_fn_type(len, args, ret_type);

  // compile new variant
  Ast *specific_ast = get_specific_fn_ast_variant(fn_ast, specific_fn_type);

  Type *original_type = fn_ast->md;
  TypeEnv *env = create_replacement_env(original_type, specific_fn_type, NULL);
  compilation_ctx->env = env;
  LLVMValueRef func =
      codegen_fn(specific_ast, compilation_ctx, module, builder);

  free_type_env(env);
  // free(specific_ast);
  return func;
}

static Type *get_application_type(size_t len, Ast *args, Type *ret_type) {}

static LLVMValueRef codegen_fn_application_callee(Ast *ast, JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder,
                                                  Type **fn_type) {

  Ast *fn_id = ast->data.AST_APPLICATION.function;
  const char *fn_name = fn_id->data.AST_IDENTIFIER.value;
  int fn_name_len = fn_id->data.AST_IDENTIFIER.length;

  Type *ret_type = ast->md;

  JITSymbol *res = lookup_id_ast(fn_id, ctx);

  if (!res) {

    fprintf(stderr,
            "codegen identifier failed symbol '%s' not found in scope %d\n",
            fn_name, ctx->stack_ptr);
    return NULL;
  }

  if (res->type == STYPE_TOP_LEVEL_VAR) {
    LLVMValueRef glob = LLVMGetNamedGlobal(module, fn_name);
    LLVMValueRef val = LLVMGetInitializer(glob);
    return val;
  } else if (res->type == STYPE_LOCAL_VAR) {
    LLVMValueRef val = res->val;
    return val;
  } else if (res->type == STYPE_FN_PARAM) {
    return res->val;
  } else if (res->type == STYPE_GENERIC_FUNCTION) {
    Ast *args = ast->data.AST_APPLICATION.args;
    size_t len = ast->data.AST_APPLICATION.len;

    TypeSerBuf *serialized_fn_buf = get_specific_fn_args_key(len, args);

    SpecificFns *specific_fns =
        res->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;

    LLVMValueRef func =
        specific_fns_lookup(specific_fns, (char *)serialized_fn_buf->data);

    if (func == NULL) {
      size_t fn_md_key_len = serialized_fn_buf->size;
      char *serialized_type = malloc(sizeof(char) * (fn_md_key_len + 1));
      strcpy(serialized_type, (const char *)serialized_fn_buf->data);

      JITLangCtx compilation_ctx = {
          ctx->stack,
          res->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr,
      };

      // save back in its own context (not in the call-site context)
      func = create_new_specific_fn(
          len, args, res->symbol_data.STYPE_GENERIC_FUNCTION.ast, ret_type,
          &compilation_ctx, module, builder);

      res->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
          specific_fns_extend(specific_fns, serialized_type, func);

      ht *scope = compilation_ctx.stack + compilation_ctx.stack_ptr;
      ht_set_hash(scope, fn_name, hash_string(fn_name, fn_name_len), res);
    }
    return func;
  } else if (res->type == STYPE_FUNCTION) {
    if (is_generic(res->symbol_type)) {
      fprintf(stderr,
              "Error: fn application result is generic - result unknown\n");
      return NULL;
    }
    return res->val;
  } else if (res->type == STYPE_FN_VARIANTS) {
    Ast *args = ast->data.AST_APPLICATION.args;
    size_t len = ast->data.AST_APPLICATION.len;
    Type *specific_fn_type = create_specific_fn_type(len, args, ret_type);

    LLVMValueRef specific_fn =
        find_fn_variant(&res->symbol_data.STYPE_FN_VARIANTS, specific_fn_type);
    if (specific_fn) {
      *fn_type = specific_fn_type;
    }
    return specific_fn;
  }
}

LLVMValueRef codegen_constructor(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *cons = ast->data.AST_APPLICATION.function->md;

  Type *val_type = ast->data.AST_APPLICATION.args->md;
  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  if (cons->constructor != NULL) {
    ConsMethod constructor = cons->constructor;
    val = constructor(val, ast->data.AST_APPLICATION.args->md, module, builder);
    if (val == NULL) {
      fprintf(stderr, "Error: constructor method for type %s didn't work",
              cons->data.T_CONS.name);
      return NULL;
    }
  }

  Type *t = find_variant_type(ctx->env, cons->data.T_CONS.name);

  if (t) {
    int i;
    find_variant_index(t, cons->data.T_CONS.name, &i);

    LLVMTypeRef element_types[2] = {
        LLVMInt32Type(),
        LLVMTypeOf(val),
    };

    LLVMTypeRef struct_type = LLVMStructType(element_types, 2, 0);
    LLVMValueRef str = LLVMGetUndef(struct_type);

    str =
        LLVMBuildInsertValue(builder, str, LLVMConstInt(LLVMInt32Type(), i, 0),
                             0, "variant_tag"); // variant tag
    str = LLVMBuildInsertValue(builder, str, val, 1,
                               "variant_contained_val"); // variant tag
    //
    return str;
  } else {
    return val;
  }
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  if (((Type *)ast->data.AST_APPLICATION.function->md)->kind == T_CONS) {
    return codegen_constructor(ast, ctx, module, builder);
  }

  if (((Type *)ast->data.AST_APPLICATION.function->md)->constructor != NULL &&
      ast->data.AST_APPLICATION.len == 1) {
    Ast *cons_input = ast->data.AST_APPLICATION.args;
    LLVMValueRef val = codegen(cons_input, ctx, module, builder);
    return attempt_value_conversion(val, cons_input->md,
                                    ast->data.AST_APPLICATION.function->md,
                                    module, builder);
  }

  Type *fn_type = NULL;
  LLVMValueRef func =
      codegen_fn_application_callee(ast, ctx, module, builder, &fn_type);

  if (!func) {
    printf("no function found for ");
    print_ast(ast);
    return NULL;
  }

  int app_len = ast->data.AST_APPLICATION.len;

  if (is_void_application(func, ast)) {
    LLVMValueRef args[] = {};
    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, args, 0,
                          "call_func");
  }

  unsigned int args_len =
      fn_type_args_len(ast->data.AST_APPLICATION.function->md);

  if (fn_type == NULL) {
    fn_type = ast->data.AST_APPLICATION.function->md;
  }

  if (app_len == args_len) {
    LLVMValueRef app_vals[app_len];

    for (int i = 0; i < app_len; i++) {
      Ast *app_val_ast = ast->data.AST_APPLICATION.args + i;

      LLVMValueRef app_val = codegen(app_val_ast, ctx, module, builder);

      if (!types_equal(app_val_ast->md, fn_type->data.T_FN.from)) {

        app_val = attempt_value_conversion(
            app_val, app_val_ast->md, fn_type->data.T_FN.from, module, builder);

        if (!app_val) {
          fprintf(stderr, "Error: attempted type conversion failed\n");
          return NULL;
        }
      }
      app_vals[i] = app_val;
      fn_type = fn_type->data.T_FN.to;
    }

    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, app_vals,
                          app_len, "call_func");
  }

  if (app_len < args_len) {
    return codegen_curry_fn(ast, func, args_len, ctx, module, builder);
  }
  return NULL;
}

JITSymbol *create_generic_fn_symbol(Ast *binding_identifier, Ast *fn_ast,
                                    JITLangCtx *ctx) {
  JITSymbol *sym = malloc(sizeof(JITSymbol));

  *sym = (JITSymbol){STYPE_GENERIC_FUNCTION,
                     .symbol_data = {.STYPE_GENERIC_FUNCTION = {
                                         fn_ast,
                                         ctx->stack_ptr,
                                     }}};

  return sym;
}

JITSymbol generic_fn_symbol(Ast *binding_identifier, Ast *fn_ast,
                            JITLangCtx *ctx) {

  return (JITSymbol){STYPE_GENERIC_FUNCTION,
                     .symbol_data = {.STYPE_GENERIC_FUNCTION = {
                                         fn_ast,
                                         ctx->stack_ptr,
                                     }}};
}

FnVariants *fn_variants_extend(FnVariants *list, Type *type,
                               LLVMValueRef func) {
  FnVariants *new_specific_fn = malloc(sizeof(FnVariants));
  new_specific_fn->type = type;
  new_specific_fn->func = func;
  new_specific_fn->next = list;
  return new_specific_fn;
};

LLVMValueRef find_fn_variant(FnVariants *variants, Type *type) {
  if (types_equal(type, variants->type)) {
    return variants->func;
  }
  if (variants->next == NULL) {
    return NULL;
  }
  return find_fn_variant(variants->next, type);
}

JITSymbol extern_variants_symbol(Ast *variants_ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  FnVariants *variants = NULL;
  for (int i = 0; i < variants_ast->data.AST_LIST.len; i++) {
    Ast *variant_ast = variants_ast->data.AST_LIST.items + i;
    LLVMValueRef variant_fn =
        codegen_extern_fn(variant_ast, ctx, module, builder);
    variants = fn_variants_extend(variants, variant_ast->md, variant_fn);
  }

  return (JITSymbol){STYPE_FN_VARIANTS,
                     .symbol_data = {.STYPE_FN_VARIANTS = *variants}};
}
