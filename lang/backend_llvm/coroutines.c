#include "coroutines.h"
#include "match.h"
#include "serde.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_generic_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {}

LLVMValueRef coroutine_instance_counter_gep(LLVMValueRef instance_ptr,
                                            LLVMTypeRef instance_type,
                                            LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 1,
                             "instance_counter_ptr");
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  if (!ctx->_coroutine_ctx.func) {
    fprintf(stderr,
            "Error: yield can only appear in a coroutine function context\n");
    return NULL;
  }

  LLVMValueRef func = ctx->_coroutine_ctx.func;
  LLVMTypeRef instance_type = ctx->_coroutine_ctx.instance_type;
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  ctx->_coroutine_ctx.current_branch++;

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance_ptr, instance_type, builder);
  LLVMBuildStore(
      builder,
      LLVMConstInt(LLVMInt32Type(), ctx->_coroutine_ctx.current_branch, 0),
      counter_gep);

  Ast *expr = ast->data.AST_YIELD.expr;

  LLVMValueRef val = codegen(expr, ctx, module, builder);
  LLVMValueRef ret_opt = codegen_option(val, builder);
  LLVMBuildRet(builder, ret_opt);
  LLVMPositionBuilderAtEnd(
      builder,
      ctx->_coroutine_ctx.block_refs[ctx->_coroutine_ctx.current_branch]);
  return ret_opt;
}

LLVMValueRef coroutine_array_iter_generator_fn(Type *expected_type, bool inf,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {}

LLVMValueRef array_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
}

LLVMValueRef coroutine_list_iter_generator_fn(Type *expected_type,
                                              JITLangCtx *ctx,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder) {}

LLVMValueRef list_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMTypeRef params_obj_type_to_llvm_type(Type *param, JITLangCtx *ctx,
                                         LLVMModuleRef module) {
  if (param->kind == T_FN) {
    LLVMTypeRef t = type_to_llvm_type(param, ctx->env, module);
    return LLVMPointerType(t, 0);
    // return t;
  }

  if (param->kind == T_COROUTINE_INSTANCE) {
    LLVMTypeRef t = type_to_llvm_type(param, ctx->env, module);
    return LLVMPointerType(t, 0);
  }
  if (is_tuple_type(param)) {
    int len = param->data.T_CONS.num_args;
    LLVMTypeRef types[len];
    for (int i = 0; i < len; i++) {
      types[i] =
          params_obj_type_to_llvm_type(param->data.T_CONS.args[i], ctx, module);
    }
    return LLVMStructType(types, len, 0);
  }
  return type_to_llvm_type(param, ctx->env, module);
}

LLVMTypeRef coroutine_instance_type(LLVMTypeRef params_obj_type) {
  if (LLVMGetTypeKind(params_obj_type) == LLVMVoidTypeKind) {

    return LLVMStructType(
        (LLVMTypeRef[]){
            GENERIC_PTR,     // coroutine generator function type
            LLVMInt32Type(), // coroutine counter
            GENERIC_PTR,     // pointer to 'parent instance' ie previous top of
                             // stack
        },
        3, 0);
  }
  return LLVMStructType(
      (LLVMTypeRef[]){
          GENERIC_PTR,     // coroutine generator function type (generic - go
          LLVMInt32Type(), // coroutine counter
          GENERIC_PTR, // pointer to 'parent instance' ie previous top of stack
          params_obj_type, // params tuple always last
      },
      4, 0);
}
LLVMValueRef coroutine_yield_end(LLVMValueRef instance,
                                 LLVMTypeRef instance_type,
                                 LLVMTypeRef ret_opt_type,
                                 LLVMBuilderRef builder) {

  // return None
  LLVMValueRef str = codegen_option(NULL, builder);
  LLVMBuildRet(builder, str);
}

LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder,
                           LLVMTypeRef *_llvm_def_type) {
  Type *fn_type = fn_ast->md;
  print_type(fn_type);
  Type *instance_type = fn_return_type(fn_type);
  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  Type *ret_opt =
      fn_return_type(instance_type->data.T_COROUTINE_INSTANCE.yield_interface);
  LLVMTypeRef llvm_ret_opt_type = type_to_llvm_type(ret_opt, ctx->env, module);
  LLVMTypeRef llvm_params_obj_type =
      params_obj_type_to_llvm_type(params_obj_type, ctx, module);

  LLVMTypeRef llvm_instance_type =
      coroutine_instance_type(llvm_params_obj_type);

  LLVMTypeRef llvm_def_type =
      LLVMFunctionType(llvm_ret_opt_type, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
  *_llvm_def_type = llvm_def_type;

  ObjString fn_name = fn_ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  LLVMValueRef func = LLVMAddFunction(
      module, !is_anon ? fn_name.chars : "anonymous_coroutine_def",
      llvm_def_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  int num_yields = fn_ast->data.AST_LAMBDA.num_yields;
  fn_ctx._coroutine_ctx = (coroutine_ctx_t){.func = func,
                                            .func_type = llvm_def_type,
                                            .instance_type = llvm_instance_type,
                                            .num_branches = num_yields + 1,
                                            .current_branch = 0};

  LLVMBasicBlockRef _block_refs[num_yields];
  fn_ctx._coroutine_ctx.block_refs = _block_refs;

  for (int i = 0; i < num_yields; i++) {
    fn_ctx._coroutine_ctx.block_refs[i] =
        LLVMAppendBasicBlock(func, "yield_case");
  }

  fn_ctx._coroutine_ctx.block_refs[num_yields] =
      LLVMAppendBasicBlock(func, "default");

  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  if (!types_equal(params_obj_type, &t_void)) {
    LLVMValueRef params_tuple =
        codegen_tuple_access(3, instance_ptr, llvm_instance_type, builder);
    if (params_obj_type->data.T_CONS.num_args > 1) {
      size_t args_len = params_obj_type->data.T_CONS.num_args;

      for (size_t i = 0; i < args_len; i++) {
        Ast *param_ast = fn_ast->data.AST_LAMBDA.params + i;

        LLVMValueRef _param_val = codegen_tuple_access(
            i, params_tuple, llvm_params_obj_type, builder);

        match_values(param_ast, _param_val,
                     params_obj_type->data.T_CONS.args[i], &fn_ctx, module,
                     builder);
      }
    } else {
      Ast *param_ast = fn_ast->data.AST_LAMBDA.params;

      LLVMValueRef param = LLVMBuildStructGEP2(builder, llvm_instance_type,
                                               instance_ptr, 3, "get_param");

      LLVMValueRef val =
          LLVMBuildLoad2(builder, llvm_params_obj_type, param, "");

      match_values(param_ast, val, params_obj_type, &fn_ctx, module, builder);
    }
  }

  LLVMValueRef switch_val =
      codegen_tuple_access(1, instance_ptr, llvm_instance_type, builder);

  LLVMValueRef switch_inst = LLVMBuildSwitch(
      builder, switch_val, fn_ctx._coroutine_ctx.block_refs[num_yields],
      num_yields); // switch to default branch
  for (int i = 0; i < num_yields; i++) {
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt32Type(), i, 0),
                fn_ctx._coroutine_ctx.block_refs[i]);
  }

  LLVMPositionBuilderAtEnd(builder, fn_ctx._coroutine_ctx.block_refs[0]);

  LLVMValueRef body =
      codegen(fn_ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  // coroutine_default_block(instance_ptr, llvm_instance_type,
  // llvm_ret_opt_type,
  //                         builder);
  coroutine_yield_end(instance_ptr, llvm_instance_type, llvm_ret_opt_type,
                      builder);

  LLVMDumpValue(func);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef codegen_coroutine_instance(LLVMValueRef _inst, Type *instance_type,
                                        LLVMValueRef func, LLVMValueRef *params,
                                        int num_params, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);
  LLVMTypeRef llvm_instance_type =
      coroutine_instance_type(llvm_params_obj_type);

  LLVMValueRef instance;
  if (_inst == NULL) {
    instance = heap_alloc(llvm_instance_type, ctx, builder);
  }

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, llvm_instance_type, builder);
  LLVMBuildStore(builder, func, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, llvm_instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);
}

LLVMValueRef coroutine_instance_from_def_symbol(
    LLVMValueRef _instance, JITSymbol *sym, Ast *args, int args_len,
    Type *expected_fn_type, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {
  LLVMValueRef func = sym->val;
  LLVMTypeRef llvm_def_type = sym->llvm_type;
  Type *instance_type = fn_return_type(sym->symbol_type);

  LLVMValueRef param_args[args_len];
  for (int i = 0; i < args_len; i++) {
    param_args[i] = codegen(args + i, ctx, module, builder);
  }

  LLVMValueRef instance =
      codegen_coroutine_instance(_instance, instance_type, func, param_args,
                                 args_len, ctx, module, builder);

  print_type(instance_type);
  printf("instance from def\n");
  return NULL;
}

LLVMValueRef coroutine_next(LLVMValueRef instance, LLVMTypeRef instance_type,
                            LLVMTypeRef def_fn_type, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMTypeRef llvm_def_type_of_instance(Type *instance_type, JITLangCtx *ctx,
                                      LLVMModuleRef module) {}
LLVMValueRef coroutine_def_from_generic(JITSymbol *sym, Type *expected_fn_type,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {}
