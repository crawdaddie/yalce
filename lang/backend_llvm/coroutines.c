#include "coroutines.h"
#include "function.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef coroutine_instance_type() {

  return LLVMStructType(
      (LLVMTypeRef[]){
          GENERIC_PTR,     // coroutine generator function type (generic - go
          LLVMInt32Type(), // coroutine counter
          GENERIC_PTR, // pointer to 'parent instance' ie previous top of stack
          GENERIC_PTR},
      4, 0);
}

LLVMTypeRef coroutine_fn_type(LLVMTypeRef ret_opt_type) {
  return LLVMFunctionType(
      ret_opt_type,
      (LLVMTypeRef[]){LLVMPointerType(coroutine_instance_type(), 0)}, 1, 0);
}

LLVMValueRef codegen_generic_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {}

LLVMValueRef coroutine_instance_counter_gep(LLVMValueRef instance_ptr,
                                            LLVMBuilderRef builder) {
  LLVMTypeRef instance_type = coroutine_instance_type();
  return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 1,
                             "instance_counter_ptr");
}

LLVMValueRef coroutine_instance_fn_gep(LLVMValueRef instance_ptr,
                                       LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coroutine_instance_type(), instance_ptr,
                             0, "instance_fn_ptr");
}

LLVMValueRef coroutine_instance_params_gep(LLVMValueRef instance_ptr,
                                           LLVMBuilderRef builder) {
  return LLVMBuildInBoundsGEP2(
      builder, coroutine_instance_type(), instance_ptr,
      (LLVMValueRef[]){
          LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
          LLVMConstInt(LLVMInt32Type(), 3, 0)  // Get nth element
      },
      2, "instance_params_gep");
}

LLVMValueRef coroutine_instance_parent_gep(LLVMValueRef instance_ptr,
                                           LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coroutine_instance_type(), instance_ptr,
                             2, "instance_parent_ptr");
}

LLVMValueRef replace_instance(LLVMValueRef instance, LLVMValueRef new_instance,
                              LLVMBuilderRef builder) {
  LLVMTypeRef instance_type = coroutine_instance_type();
  LLVMValueRef size = LLVMSizeOf(instance_type);
  LLVMBuildMemCpy(builder, instance, 0, new_instance, 0, size);
  return instance;
}

void increment_instance_counter(LLVMValueRef instance_ptr,
                                LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance_ptr, builder);

  LLVMValueRef counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "instance_counter");

  counter = LLVMBuildAdd(builder, counter, LLVMConstInt(LLVMInt32Type(), 1, 0),
                         "instance_counter++");
  LLVMBuildStore(builder, counter, counter_gep);
}

void reset_instance_counter(LLVMValueRef instance_ptr, LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance_ptr, builder);
  LLVMValueRef counter = LLVMConstInt(LLVMInt32Type(), 0, 0);
  LLVMBuildStore(builder, counter, counter_gep);
}

void set_instance_counter(LLVMValueRef instance_ptr, LLVMValueRef counter,
                          LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance_ptr, builder);
  LLVMBuildStore(builder, counter, counter_gep);
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  if (!ctx->_coroutine_ctx.func) {
    fprintf(stderr,
            "Error: yield can only appear in a coroutine function context\n");
    return NULL;
  }

  LLVMValueRef func = ctx->_coroutine_ctx.func;
  LLVMTypeRef instance_type = coroutine_instance_type();
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  increment_instance_counter(instance_ptr, builder);
  ctx->_coroutine_ctx.current_branch++;
  Ast *expr = ast->data.AST_YIELD.expr;

  if (expr->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);
    if (sym->type == STYPE_COROUTINE_GENERATOR) {
      bool is_recursive_cor_init = sym->val == ctx->_coroutine_ctx.func;
      LLVMValueRef instance_copy = replace_instance(
          heap_alloc(instance_type, ctx, builder), instance_ptr, builder);

      LLVMValueRef new_instance = coroutine_instance_from_def_symbol(
          sym, expr->data.AST_APPLICATION.args, expr->data.AST_APPLICATION.len,
          expr->data.AST_APPLICATION.function->md, ctx, module, builder);

      LLVMValueRef parent_gep =
          coroutine_instance_parent_gep(new_instance, builder);
      LLVMBuildStore(builder, instance_copy, parent_gep);

      instance_ptr = replace_instance(instance_ptr, new_instance, builder);

      LLVMValueRef ret_opt =
          coroutine_next(instance_ptr, instance_type,
                         ctx->_coroutine_ctx.func_type, ctx, module, builder);

      LLVMBuildRet(builder, ret_opt);
      LLVMPositionBuilderAtEnd(
          builder,
          ctx->_coroutine_ctx.block_refs[ctx->_coroutine_ctx.current_branch]);
      return ret_opt;
    }
  }

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

LLVMValueRef coroutine_yield_end(LLVMValueRef instance,
                                 LLVMTypeRef ret_opt_type, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");

  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");

  LLVMValueRef parent_gep = coroutine_instance_parent_gep(instance, builder);
  LLVMValueRef parent_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMInt8Type(), 0), parent_gep, "parent_ptr");

  LLVMValueRef is_null = LLVMBuildICmp(
      builder, LLVMIntEQ, parent_ptr,
      LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)), "is_null");

  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);
  LLVMPositionBuilderAtEnd(builder, non_null_block);

  LLVMValueRef parent_instance = LLVMBuildPointerCast(
      builder, parent_ptr, LLVMPointerType(coroutine_instance_type(), 0),
      "parent instance");

  instance = replace_instance(instance, parent_instance, builder);
  increment_instance_counter(instance, builder);

  LLVMTypeRef def_fn_type = coroutine_fn_type(ret_opt_type);

  LLVMValueRef func =
      codegen_tuple_access(0, instance, coroutine_instance_type(), builder);

  LLVMValueRef ret_opt =
      LLVMBuildCall2(builder, def_fn_type, func,
                     (LLVMValueRef[]){parent_instance}, 1, "coroutine_next");

  LLVMBuildRet(builder, ret_opt);

  LLVMPositionBuilderAtEnd(builder, continue_block);
  LLVMValueRef str = codegen_option(NULL, builder);
  LLVMBuildRet(builder, str);
}

void add_recursive_cor_fn_ref(ObjString fn_name, LLVMValueRef func,
                              Type *fn_type, JITLangCtx *fn_ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast binding = (Ast){AST_IDENTIFIER,
                      .data = {.AST_IDENTIFIER = {.value = fn_name.chars,
                                                  .length = fn_name.length}}};
  match_values(&binding, func, fn_type, fn_ctx, module, builder);
}

LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder,
                           LLVMTypeRef *_llvm_def_type) {
  Type *fn_type = fn_ast->md;
  Type *instance_type = fn_return_type(fn_type);
  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  Type *ret_opt =
      fn_return_type(instance_type->data.T_COROUTINE_INSTANCE.yield_interface);
  LLVMTypeRef llvm_ret_opt_type = type_to_llvm_type(ret_opt, ctx->env, module);
  LLVMTypeRef llvm_params_obj_type =
      params_obj_type_to_llvm_type(params_obj_type, ctx, module);

  LLVMTypeRef llvm_instance_type = coroutine_instance_type();

  LLVMTypeRef llvm_def_type =
      LLVMFunctionType(llvm_ret_opt_type, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);

  *_llvm_def_type = llvm_def_type;

  ObjString fn_name = fn_ast->data.AST_LAMBDA.fn_name;

  bool is_anon = fn_name.chars == NULL;

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
  if (!is_anon) {
    add_recursive_cor_fn_ref(fn_name, func, fn_type, &fn_ctx, module, builder);
  }

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  if (!types_equal(params_obj_type, &t_void)) {
    LLVMValueRef params_in_instance_ptr =
        codegen_tuple_access(3, instance_ptr, llvm_instance_type, builder);

    LLVMValueRef params_tuple = LLVMBuildLoad2(
        builder, llvm_params_obj_type, params_in_instance_ptr, "params tuple");

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
      LLVMValueRef val = params_tuple;
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

  coroutine_yield_end(instance_ptr, llvm_ret_opt_type, ctx, module, builder);
  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef set_instance_params(LLVMValueRef instance,
                                 LLVMTypeRef llvm_instance_type,
                                 LLVMTypeRef llvm_params_obj_type,
                                 LLVMValueRef *params, int params_len,
                                 LLVMBuilderRef builder) {

  LLVMValueRef params_ptr_gep =
      coroutine_instance_params_gep(instance, builder);

  // LLVMValueRef params_heap = heap_alloc(llvm_params_obj_type, ctx, builder);

  if (params_ptr_gep) {
    if (params_len == 1) {
      LLVMBuildStore(builder, params[0], params_ptr_gep);
    } else {
      LLVMValueRef params_obj = LLVMGetUndef(llvm_params_obj_type);

      for (int i = 0; i < params_len; i++) {
        params_obj =
            LLVMBuildInsertValue(builder, params_obj, params[i], i, "");
      }
    }
  }
}

LLVMValueRef codegen_coroutine_instance(LLVMValueRef _inst, Type *instance_type,
                                        LLVMValueRef func, LLVMValueRef *params,
                                        int num_params, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);
  LLVMTypeRef llvm_instance_type = coroutine_instance_type();

  LLVMValueRef instance;
  if (_inst == NULL) {
    instance = heap_alloc(llvm_instance_type, ctx, builder);
  }

  LLVMValueRef fn_gep = coroutine_instance_fn_gep(instance, builder);
  LLVMBuildStore(builder, func, fn_gep);

  LLVMValueRef counter_gep = coroutine_instance_counter_gep(instance, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  // LLVMDumpType(llvm_params_obj_type);

  if (LLVMGetTypeKind(llvm_params_obj_type) == LLVMVoidTypeKind) {
    return instance;
  }

  LLVMValueRef params_alloc = heap_alloc(llvm_params_obj_type, ctx, builder);

  if (num_params == 1) {
    LLVMBuildStore(builder, params[0], params_alloc);
  } else {

    for (int i = 0; i < num_params; i++) {
      LLVMValueRef element_ptr = LLVMBuildGEP2(
          builder, llvm_params_obj_type, params_alloc,
          (LLVMValueRef[]){

              LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
              LLVMConstInt(LLVMInt32Type(), i, 0)  // Get nth element
          },
          2, "params_ptr_element_ptr");
      LLVMBuildStore(builder, params[i], element_ptr);
    }
  }
  LLVMValueRef params_in_instance =
      LLVMBuildGEP2(builder, llvm_instance_type, instance,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 3, 0)  // Get nth element
                    },
                    2, "params_in_instance_ptr");
  LLVMBuildStore(builder, params_alloc, params_in_instance);

  return instance;
}

LLVMValueRef coroutine_instance_from_def_symbol(
    JITSymbol *sym, Ast *args, int args_len, Type *expected_fn_type,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMValueRef func = sym->val;
  LLVMTypeRef llvm_def_type = sym->llvm_type;
  Type *instance_type = fn_return_type(sym->symbol_type);

  LLVMValueRef param_args[args_len];
  for (int i = 0; i < args_len; i++) {
    param_args[i] = codegen(args + i, ctx, module, builder);
  }

  LLVMValueRef instance = codegen_coroutine_instance(
      NULL, instance_type, func, param_args, args_len, ctx, module, builder);

  return instance;
}

LLVMValueRef coroutine_next(LLVMValueRef instance, LLVMTypeRef instance_type,
                            LLVMTypeRef def_fn_type, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef func = codegen_tuple_access(0, instance, instance_type, builder);

  LLVMValueRef result =
      LLVMBuildCall2(builder, def_fn_type, func, (LLVMValueRef[]){instance}, 1,
                     "coroutine_next");
  return result;
}

LLVMTypeRef llvm_def_type_of_instance(Type *instance_type, JITLangCtx *ctx,
                                      LLVMModuleRef module) {}

LLVMValueRef coroutine_def_from_generic(JITSymbol *sym, Type *expected_fn_type,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast;
  Type *instance_type = fn_return_type(expected_fn_type);
  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  Type *ret_opt =
      fn_return_type(instance_type->data.T_COROUTINE_INSTANCE.yield_interface);
  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);
  LLVMTypeRef llvm_instance_type = coroutine_instance_type();
  LLVMTypeRef llvm_ret_opt = type_to_llvm_type(ret_opt, ctx->env, module);
  LLVMTypeRef llvm_def_type = coroutine_fn_type(llvm_ret_opt);

  Ast *specific_ast = get_specific_fn_ast_variant(fn_ast, expected_fn_type);
  JITLangCtx compilation_ctx = {
      ctx->stack,
      sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.stack_ptr,
  };

  TypeEnv *og_env = compilation_ctx.env;
  TypeEnv *_env = compilation_ctx.env;
  Type *o = sym->symbol_type;
  Type *e = expected_fn_type;

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

  compilation_ctx.env = _env;

  compilation_ctx.env = _env;
  LLVMTypeRef _def_type;
  LLVMValueRef def = coroutine_def(specific_ast, &compilation_ctx, module,
                                   builder, &_def_type);
  return def;
}

LLVMValueRef codegen_loop_coroutine(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  LLVMValueRef coroutine_def =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *def_type = ast->data.AST_APPLICATION.args->md;

  LLVMValueRef params[] = {
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder)};

  Type *instance_type = fn_return_type(def_type);
  Type *params_obj_type = ast->data.AST_APPLICATION.args[1].md;

  Type *ret_opt = instance_type->data.T_COROUTINE_INSTANCE.yield_interface;
  ret_opt = fn_return_type(ret_opt);
  LLVMTypeRef llvm_ret_opt_type = type_to_llvm_type(ret_opt, ctx->env, module);

  // build wrapper around coroutine def that calls its input instance, and if
  // that returns None, resets the instance
  LLVMTypeRef wrapper_type = coroutine_fn_type(llvm_ret_opt_type);
  LLVMValueRef wrapper = LLVMAddFunction(module, "loop_cor_", wrapper_type);
  LLVMSetLinkage(wrapper, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(wrapper, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef instance_ptr = LLVMGetParam(wrapper, 0);

  LLVMValueRef next = LLVMBuildCall2(builder, wrapper_type, coroutine_def,
                                     (LLVMValueRef[]){instance_ptr}, 1,
                                     "call_wrapped_coroutine");

  // LLVMValueRef option_tag = codegen_tuple_access(0,

  // Create basic blocks for the different paths
  LLVMBasicBlockRef some_block = LLVMAppendBasicBlock(wrapper, "case_some");
  LLVMBasicBlockRef none_block = LLVMAppendBasicBlock(wrapper, "case_none");

  // Create the conditional branch
  LLVMValueRef cmp = codegen_option_is_some(next, builder);
  LLVMBuildCondBr(builder, cmp, some_block, none_block);

  LLVMPositionBuilderAtEnd(builder, some_block);
  LLVMBuildRet(builder, next);

  LLVMPositionBuilderAtEnd(builder, none_block);
  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);

  reset_instance_counter(instance_ptr, builder);

  next = LLVMBuildCall2(builder, wrapper_type, coroutine_def,
                        (LLVMValueRef[]){instance_ptr}, 1,
                        "call_wrapped_coroutine");
  LLVMBuildRet(builder, next);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef instance = codegen_coroutine_instance(
      NULL, instance_type, wrapper, params, 1, ctx, module, builder);

  return instance;
}
