#include "backend_llvm/codegen_function.h"
#include "backend_llvm/codegen_symbols.h"
#include "codegen_function_currying.h"
#include "codegen_types.h"
#include "serde.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

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

LLVMValueRef codegen_fn_proto(Type *fn_type, int fn_len, const char *fn_name,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  // Create argument list.
  LLVMTypeRef llvm_param_types[fn_len];

  for (int i = 0; i < fn_len; i++) {
    llvm_param_types[i] = type_to_llvm_type(fn_type->data.T_FN.from);
    fn_type = fn_type->data.T_FN.to;
  }

  Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
  LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(return_type);

  // Create function type with return.
  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);

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
  JITLangCtx fn_ctx = {.stack = ctx->stack, .stack_ptr = ctx->stack_ptr + 1};
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  for (int i = 0; i < fn_len; i++) {
    Type *param_type = fn_type->data.T_FN.from;
    fn_type = fn_type->data.T_FN.to;

    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    LLVMValueRef param_val = LLVMGetParam(func, i);
    codegen_multiple_assignment(param_ast, param_val, param_type, &fn_ctx,
                                module, builder, true, i);
  }

  // add function as recursive ref
  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = LLVMTypeOf(func),
                   .type = STYPE_FUNCTION,
                   .val = func,
                   .symbol_data = {.STYPE_FUNCTION = {.fn_type = ast->md}}};
  // LLVMDumpValue(func);
  // printf("recursive fn ref '%s' %llu\n", fn_name.chars, fn_name.hash);

  ht *scope = fn_ctx.stack + fn_ctx.stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, v);

  // Generate body.
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

#define LLVM_TYPE_int LLVMInt32Type()
#define LLVM_TYPE_bool LLVMInt1Type()
#define LLVM_TYPE_float LLVMFloatType()
#define LLVM_TYPE_double LLVMDoubleType()
#define LLVM_TYPE_void LLVMVoidType()
#define LLVM_TYPE_str LLVMPointerType(LLVMInt8Type(), 0)
#define LLVM_TYPE_ptr(type) LLVMPointerType(LLVM_TYPE_##type, 0)

static LLVMTypeRef llvm_type_id(Ast *id) {
  if (id->tag == AST_VOID) {
    return LLVM_TYPE_void;
  }

  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }
  char *id_chars = id->data.AST_IDENTIFIER.value;

  if (strcmp(id_chars, "int") == 0) {
    return LLVM_TYPE_int;
  } else if (strcmp(id_chars, "double") == 0) {
    return LLVM_TYPE_float;
  } else if (strcmp(id_chars, "bool") == 0) {
    return LLVM_TYPE_bool;
  } else if (strcmp(id_chars, "string") == 0) {
    return LLVM_TYPE_str;
  }

  return NULL;
}
LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  const char *name = ast->data.AST_EXTERN_FN.fn_name.chars;
  int name_len = strlen(name);
  int params_count = ast->data.AST_EXTERN_FN.len - 1;

  Ast *signature_types = ast->data.AST_EXTERN_FN.signature_types;
  if (params_count == 0) {
    // LLVMTypeRef llvm_param_types[] = {};

    LLVMTypeRef ret_type = llvm_type_id(signature_types);
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, NULL, 0, false);

    LLVMValueRef func = LLVMAddFunction(module, name, fn_type);
    return func;
  }

  LLVMTypeRef llvm_param_types[params_count];
  for (int i = 0; i < params_count; i++) {
    llvm_param_types[i] = llvm_type_id(signature_types + i);
  }

  LLVMTypeRef ret_type = llvm_type_id(signature_types + params_count);
  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, llvm_param_types, params_count, false);

  LLVMValueRef func = LLVMAddFunction(module, name, fn_type);
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
                                        Type *specific_fn_type,
                                        size_t fn_name_len,
                                        size_t fn_md_key_len,
                                        const char *fn_md_key) {
  const char *fn_name = original_fn_ast->data.AST_LAMBDA.fn_name.chars;

  Ast *specific_ast = malloc(sizeof(Ast));
  *specific_ast = *(original_fn_ast);
  specific_ast->md = specific_fn_type;
  // int total_fn_name_len = fn_name_len + 1 + fn_md_key_len + 1;
  //
  // specific_ast->data.AST_LAMBDA.fn_name =
  //     (ObjString){.chars = malloc(sizeof(char) * (total_fn_name_len + 1))};
  //
  // snprintf(specific_ast->data.AST_LAMBDA.fn_name.chars, total_fn_name_len +
  // 1,
  //          "%s[%s]", fn_name, fn_md_key);
  return specific_ast;
}

static LLVMValueRef codegen_fn_application_callee(Ast *ast, JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {
  Ast *fn_id = ast->data.AST_APPLICATION.function;
  const char *fn_name = fn_id->data.AST_IDENTIFIER.value;
  int fn_name_len = fn_id->data.AST_IDENTIFIER.length;

  JITSymbol *res = NULL;

  if (codegen_lookup_id(fn_name, fn_name_len, ctx, &res)) {

    printf("codegen identifier failed symbol %s not found in scope %d\n",
           fn_name, ctx->stack_ptr);
    return NULL;
  }

  if (res->type == STYPE_TOP_LEVEL_VAR) {
    LLVMValueRef glob = LLVMGetNamedGlobal(module, fn_name);
    LLVMValueRef val = LLVMGetInitializer(glob);
    return val;
  } else if (res->type == STYPE_LOCAL_VAR) {
    LLVMValueRef val = LLVMBuildLoad2(builder, res->llvm_type, res->val, "");
    return val;
  } else if (res->type == STYPE_FN_PARAM) {
    return res->val;
  } else if (res->type == STYPE_FUNCTION) {
    // printf("found function {recursive??}\n");
    // print_type(res->symbol_data.STYPE_FUNCTION.fn_type);
    if (is_generic(res->symbol_data.STYPE_FUNCTION.fn_type)) {
      // if (is_generic(application_result_type)) {
      fprintf(stderr,
              "Error: fn application result is generic - result unknown\n");
      return NULL;
    }
    return res->val;
  } else if (res->type == STYPE_GENERIC_FUNCTION) {

    Ast *args = ast->data.AST_APPLICATION.args;
    size_t len = ast->data.AST_APPLICATION.len;

    TypeSerBuf *specific_fn_buf = get_specific_fn_args_key(len, args);

    SpecificFns *specific_fns =
        res->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;

    LLVMValueRef func =
        specific_fns_lookup(specific_fns, (char *)specific_fn_buf->data);

    if (func == NULL) {
      Type *specific_fn_type = create_specific_fn_type(len, args, ast->md);

      // get metadata key for caching compiled variant
      size_t fn_md_key_len = specific_fn_buf->size;
      char *fn_md_key = malloc(sizeof(char) * (fn_md_key_len + 1));
      strncpy(fn_md_key, (const char *)specific_fn_buf->data, fn_md_key_len);

      // compile new variant & save
      Ast *specific_ast = get_specific_fn_ast_variant(
          res->symbol_data.STYPE_GENERIC_FUNCTION.ast, specific_fn_type,
          fn_name_len, fn_md_key_len, fn_md_key);

      JITLangCtx compilation_ctx = {
          ctx->stack, res->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr};

      func = codegen_fn(specific_ast, &compilation_ctx, module, builder);

      res->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
          specific_fns_extend(specific_fns, fn_md_key, func);

      // save back in its own context (not in the call-site context)
      ht *scope = compilation_ctx.stack + compilation_ctx.stack_ptr;
      ht_set_hash(scope, fn_name, hash_string(fn_name, fn_name_len), res);
      free(specific_ast);
    }
    free(specific_fn_buf->data);
    free(specific_fn_buf);
    return func;
  }
}

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  LLVMValueRef func = codegen_fn_application_callee(ast, ctx, module, builder);

  if (!func) {
    return NULL;
  }

  int app_len = ast->data.AST_APPLICATION.len;

  if (is_void_application(func, ast)) {
    LLVMValueRef args[] = {};
    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, args, 0,
                          "call_func");
  }

  unsigned int args_len = LLVMCountParams(func);

  if (app_len == args_len) {
    LLVMValueRef app_vals[app_len];
    for (int i = 0; i < app_len; i++) {
      app_vals[i] =
          codegen(ast->data.AST_APPLICATION.args + i, ctx, module, builder);
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
