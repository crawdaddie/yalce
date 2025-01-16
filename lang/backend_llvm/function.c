#include "backend_llvm/function.h"
#include "match.h"
#include "symbols.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef specific_fns_lookup(SpecificFns *list, Type *key);

SpecificFns *specific_fns_extend(SpecificFns *list, Type *arg_types,
                                 LLVMValueRef func);

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
  LLVMTypeRef prototype = codegen_fn_type(ast->md, fn_len, ctx->env, module);

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

    // if (param_type->kind == T_FN) {
    //   const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
    //   int id_len = param_ast->data.AST_IDENTIFIER.length;
    //   LLVMTypeRef llvm_type = type_to_llvm_type(param_type, ctx->env,
    //   module); JITSymbol *sym =
    //       new_symbol(STYPE_LOCAL_VAR, param_type, param_val, llvm_type);
    //
    //   ht_set_hash(&ctx->frame->table, id_chars, hash_string(id_chars,
    //   id_len),
    //               sym);
    //
    // } else {
    // }
    codegen_pattern_binding(param_ast, param_val, param_type, &fn_ctx, module,
                            builder);

    fn_type = fn_type->data.T_FN.to;
  }

  LLVMValueRef body;
  for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
    Ast *stmt = ast->data.AST_BODY.stmts[i];
    if (i == 0 && stmt->tag == AST_STRING) {
      // skip docstring
      continue;
    }
    body = codegen(stmt, &fn_ctx, module, builder);
  }

  LLVMBuildRet(builder, body);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // clear function stack frame
  if (fn_ctx.frame->next != NULL) {
    ht_destroy(fn_ctx.frame->table);
    free(fn_ctx.frame);
  }

  return func;
}

LLVMValueRef get_specific_callable(JITSymbol *sym, const char *sym_name,
                                   Type *expected_fn_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return NULL;
}
