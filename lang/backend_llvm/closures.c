#include "./closures.h"
#include "binding.h"
#include "symbols.h"
#include "types.h"
#include "types/closures.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

bool is_lambda_with_closures(Ast *ast) {
  return ast->tag == AST_LAMBDA &&
         (ast->data.AST_LAMBDA.num_closure_free_vars > 0);
}

LLVMValueRef create_curried_generic_closure_binding(
    Ast *binding, Type *closure_type, Ast *closure, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  JITSymbol *curried_sym =
      new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, closure_type, NULL, NULL);

  int len = closure->data.AST_LAMBDA.num_closure_free_vars;

  // closure->md = full_type;

  JITSymbol *original_sym = create_generic_fn_symbol(closure, ctx);

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym =
      original_sym;

  LLVMValueRef *app_args = malloc(sizeof(LLVMValueRef) * len);
  AstList *l = closure->data.AST_LAMBDA.params;
  for (int i = 0; i < len; i++) {
    app_args[i] = codegen(l->ast, ctx, module, builder);
    l = l->next;
  }
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len = len;
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len =
      len + closure->data.AST_LAMBDA.len;

  Type *full_type = get_full_fn_type_of_closure(closure);
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type =
      full_type;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
              curried_sym);
  return NULL;
}

LLVMValueRef create_curried_closure_binding(Ast *binding, Type *closure_type,
                                            Ast *closure, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  JITSymbol *curried_sym =
      new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, closure_type, NULL, NULL);

  int len = closure->data.AST_LAMBDA.num_closure_free_vars;
  LLVMValueRef *app_args = malloc(sizeof(LLVMValueRef) * len);

  AstList *l = closure->data.AST_LAMBDA.params;
  for (int i = 0; i < len; i++) {
    app_args[i] = codegen(l->ast, ctx, module, builder);
    l = l->next;
  }

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym =
      NULL; // <- this will be NULL, instead we compile the original lambda &
            // add it to the symbol
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len = len;
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len =
      len + closure->data.AST_LAMBDA.len;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type =
      get_full_fn_type_of_closure(closure);

  LLVMValueRef func = codegen(closure, ctx, module, builder);
  curried_sym->val = func;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
              curried_sym);
  return func;
}

LLVMTypeRef get_closure_fn_type(int num_args, Type *fn_type, Type *ret_type,
                                LLVMTypeRef llvm_closure_obj_type,
                                JITLangCtx *ctx, LLVMModuleRef module) {
  LLVMTypeRef arg_types[num_args + 1];

  arg_types[0] = LLVMPointerType(llvm_closure_obj_type, 0);
  for (int i = 0; i < num_args; i++) {
    arg_types[i + 1] = type_to_llvm_type(fn_type->data.T_FN.from, ctx, module);
    fn_type = fn_type->data.T_FN.to;
  }

  LLVMTypeRef ftype = LLVMFunctionType(type_to_llvm_type(ret_type, ctx, module),
                                       arg_types, num_args + 1, 0);

  return ftype;
}

void add_recursive_closure_ref(ObjString fn_name, LLVMValueRef func,
                               Type *fn_type, JITLangCtx *fn_ctx) {
  // JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func,
  // LLVMTypeOf(func)); sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;
  // sym->symbol_data.STYPE_FUNCTION.recursive_ref = true;
  //
  // ht *scope = fn_ctx->frame->table;
  // ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMTypeRef get_closure_obj_type(Type *fn_type, JITLangCtx *ctx,
                                 LLVMModuleRef module) {

  Type *closure_vals_type = fn_type->closure_meta;
  int num_closure_vals = closure_vals_type->data.T_CONS.num_args;
  LLVMTypeRef contained[num_closure_vals + 1];
  contained[0] = GENERIC_PTR; // fn pointer
  for (int i = 0; i < num_closure_vals; i++) {
    contained[i + 1] =
        type_to_llvm_type(closure_vals_type->data.T_CONS.args[i], ctx, module);
  }

  LLVMTypeRef llvm_closure_obj_type =
      LLVMStructType(contained, num_closure_vals + 1, 0);
  return llvm_closure_obj_type;
}

LLVMValueRef closure_closed_val_ptr(LLVMValueRef obj, LLVMTypeRef obj_type,
                                    int idx, LLVMBuilderRef builder) {

  return LLVMBuildStructGEP2(builder, obj_type, obj, idx + 1, "");
}
LLVMValueRef closure_closed_val(LLVMValueRef obj, LLVMTypeRef obj_type, int idx,
                                LLVMBuilderRef builder) {

  LLVMValueRef closed_val_ptr =
      closure_closed_val_ptr(obj, obj_type, idx, builder);

  return LLVMBuildLoad2(builder, LLVMStructGetTypeAtIndex(obj_type, idx + 1),
                        closed_val_ptr, "");
}

LLVMValueRef compile_closure(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  Type *closure_type = ast->md;

  LLVMTypeRef llvm_closure_obj_type =
      get_closure_obj_type(closure_type, ctx, module);

  LLVMTypeRef closure_obj_ptr_type = LLVMPointerType(llvm_closure_obj_type, 0);

  Type *ret_type = deep_copy_type(fn_return_type(closure_type));

  int num_args = ast->data.AST_LAMBDA.len;
  LLVMTypeRef llvm_fn_type;

  if (num_args == 1 && closure_type->data.T_FN.from->kind == T_VOID) {

    llvm_fn_type = LLVMFunctionType(
        type_to_llvm_type(ret_type, ctx, module),
        (LLVMTypeRef[]){LLVMPointerType(llvm_closure_obj_type, 0)}, 1, false);

  } else {
    llvm_fn_type = get_closure_fn_type(num_args, closure_type, ret_type,
                                       llvm_closure_obj_type, ctx, module);
  }

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;

  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  LLVMValueRef func = LLVMAddFunction(
      module, is_anon ? "anonymous_func" : fn_name.chars, llvm_fn_type);
  if (func == NULL) {
    fprintf(stderr, "Error: could not create function\n");
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  if (!is_anon) {
    add_recursive_closure_ref(fn_name, func, closure_type, &fn_ctx);
  }

  LLVMValueRef internal_closure_ref = LLVMGetParam(func, 0);

  int i = 0;
  for (AstList *l = ast->data.AST_LAMBDA.closed_vals; l; l = l->next, i++) {
    Ast *b = l->ast;
    Type *bt = closure_type->closure_meta->data.T_CONS.args[i];

    // TODO: ?????????
    // if (is_generic(bt)) {
    //   bt = resolve_type_in_env(bt, ctx->env);
    // }

    LLVMValueRef closed_val = closure_closed_val(
        internal_closure_ref, llvm_closure_obj_type, i, builder);

    LLVMTypeRef item_type = type_to_llvm_type(bt, &fn_ctx, module);

    JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, bt, NULL, item_type);
    sym->storage = closure_closed_val_ptr(internal_closure_ref,
                                          llvm_closure_obj_type, i, builder);
    const char *chars = b->data.AST_IDENTIFIER.value;
    int chars_len = b->data.AST_IDENTIFIER.length;
    ht_set_hash(fn_ctx.frame->table, chars, hash_string(chars, chars_len), sym);
  }

  i = 0;
  Type *ftype = closure_type;
  for (AstList *arg = ast->data.AST_LAMBDA.params; arg;
       arg = arg->next, i++, ftype = ftype->data.T_FN.to) {
    Ast *param_ast = arg->ast;
    Type *ptype = deep_copy_type(ftype->data.T_FN.from);

    // TODO: ?????????
    // if (is_generic(ptype)) {
    //   ptype = resolve_type_in_env(ptype, ctx->env);
    // }

    LLVMValueRef param_val = LLVMGetParam(func, i + 1);

    if (ptype->kind == T_FN) {
      const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
      int id_len = param_ast->data.AST_IDENTIFIER.length;
      LLVMTypeRef llvm_type = type_to_llvm_type(ptype, ctx, module);

      JITSymbol *sym = new_symbol(STYPE_FUNCTION, ptype, param_val, llvm_type);

      ht_set_hash(fn_ctx.frame->table, id_chars, hash_string(id_chars, id_len),
                  sym);

    } else {
      codegen_pattern_binding(param_ast, param_val, ptype, &fn_ctx, module,
                              builder);
    }
  }

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  if (ret_type->kind == T_VOID) {
    LLVMBuildRetVoid(builder);
  } else {
    LLVMBuildRet(builder, body);
  }

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);

  LLVMValueRef closure_obj;
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    closure_obj = LLVMBuildAlloca(builder, llvm_closure_obj_type,
                                  "closure_obj_alloc_stacc");
  } else {
    closure_obj = LLVMBuildMalloc(builder, llvm_closure_obj_type,
                                  "closure_obj_alloc_heap");
  }

  LLVMValueRef fn_ptr =
      LLVMBuildStructGEP2(builder, llvm_closure_obj_type, closure_obj, 0, "");
  LLVMBuildStore(builder, func, fn_ptr);

  i = 0;
  for (AstList *l = ast->data.AST_LAMBDA.closed_vals; l; l = l->next, i++) {
    Ast *b = l->ast;
    LLVMValueRef closed_val_ptr =
        closure_closed_val_ptr(closure_obj, llvm_closure_obj_type, i, builder);
    LLVMBuildStore(builder, codegen(b, ctx, module, builder), closed_val_ptr);
  }

  return closure_obj;
}

LLVMValueRef call_closure_sym(Ast *app, JITSymbol *sym, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMValueRef obj = sym->val;
  LLVMTypeRef obj_type = sym->llvm_type;
  Type *closure_type = sym->symbol_type;

  Type *ret_type = app->md;
  int num_args = fn_type_args_len(closure_type);

  if (closure_type->data.T_FN.from->kind == T_VOID) {
    num_args = 0;
  }

  LLVMTypeRef llvm_fn_type = get_closure_fn_type(
      num_args, closure_type, ret_type, obj_type, ctx, module);

  LLVMValueRef fn_ptr =
      LLVMBuildStructGEP2(builder, sym->llvm_type, obj, 0, "");
  LLVMValueRef func = LLVMBuildLoad2(builder, GENERIC_PTR, fn_ptr, "");
  LLVMValueRef args[num_args + 1];

  args[0] = obj;
  for (int i = 0; i < num_args; i++) {
    args[i + 1] =
        codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
  }
  return LLVMBuildCall2(builder, llvm_fn_type, func, args, num_args + 1, "");
}
