#include "coroutines.h"
#include "coroutine_instance.h"
#include "function.h"
#include "list.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef param_struct_type(Type *fn_type, int fn_len, Type *param_obj_type,
                              Type *return_type, TypeEnv *env,
                              LLVMModuleRef module) {

  // printf("param struct type: ");
  // print_type(fn_type);

  LLVMTypeRef llvm_param_types[fn_len];

  Type **contained = talloc(sizeof(Type *) * fn_len);
  Type *f = fn_type;

  for (int i = 0; i < fn_len; i++) {
    Type *t = f->data.T_FN.from;

    contained[i] = t;

    llvm_param_types[i] = type_to_llvm_type(t, env, module);

    if (t->kind == T_FN) {
      print_type(t);
      llvm_param_types[i] = LLVMPointerType(llvm_param_types[i], 0);
    } else if (is_pointer_type(t)) {
      llvm_param_types[i] = LLVMPointerType(
          type_to_llvm_type(t->data.T_CONS.args[0], env, module), 0);
    } else {
      llvm_param_types[i] = type_to_llvm_type(t, env, module);
    }

    f = f->data.T_FN.to;
  }

  *return_type = *(f->data.T_FN.to);

  if (fn_len == 0 || fn_type->data.T_FN.from->kind == T_VOID) {
    param_obj_type->kind = T_VOID;
    return LLVMVoidType();
  }

  Type param_tuple;
  if (fn_len > 1) {
    param_tuple = (Type){T_CONS,
                         {.T_CONS = {.name = TYPE_NAME_TUPLE,
                                     .args = contained,
                                     .num_args = fn_len}}};
    *param_obj_type = param_tuple;
    return LLVMStructType(llvm_param_types, fn_len, 0);
  }

  *param_obj_type = *contained[0];
  return llvm_param_types[0];
}

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

typedef struct coroutine_ctx_t {
  LLVMValueRef switch_val;
  LLVMBasicBlockRef *block_refs;
  LLVMTypeRef ret_option_type;
  LLVMTypeRef instance_type;
  LLVMValueRef func;
  LLVMTypeRef func_type;
  int num_branches;
  int current_branch;
} coroutine_ctx_t;

static coroutine_ctx_t _coroutine_ctx = {};

static void copy_instance(LLVMValueRef dest_ptr, LLVMValueRef src_ptr,
                          LLVMTypeRef instance_type, LLVMBuilderRef builder) {
  LLVMValueRef size = LLVMSizeOf(instance_type);
  LLVMBuildMemCpy(builder, dest_ptr, 0, src_ptr, 0, size);
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef func = _coroutine_ctx.func;
  LLVMTypeRef instance_type = _coroutine_ctx.instance_type;
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  increment_instance_counter(instance_ptr, instance_type, builder);
  _coroutine_ctx.current_branch++;

  Ast *expr = ast->data.AST_YIELD.expr;
  if (expr->tag == AST_APPLICATION) {
    LLVMValueRef new_instance;
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    LLVMValueRef old_instance_ptr =
        replace_instance(heap_alloc(instance_type, ctx, builder), instance_type,
                         instance_ptr, builder);

    if (sym->type == STYPE_COROUTINE_GENERATOR) {

      instance_type = coroutine_instance_type(
          sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type);

      bool is_same_recursive_ref =
          sym->symbol_data.STYPE_COROUTINE_GENERATOR.recursive_ref;

      if (is_same_recursive_ref) {
        printf("yield rec\n");
        new_instance = coroutine_instance_from_def_symbol(
            instance_ptr, sym, expr->data.AST_APPLICATION.args,
            expr->data.AST_APPLICATION.len,
            expr->data.AST_APPLICATION.function->md, ctx, module, builder);
      }

      new_instance = coroutine_instance_from_def_symbol(
          NULL, sym, expr->data.AST_APPLICATION.args,
          expr->data.AST_APPLICATION.len,
          expr->data.AST_APPLICATION.function->md, ctx, module, builder);
    }

    if (sym->type == STYPE_GENERIC_COROUTINE_GENERATOR) {
      new_instance = codegen(expr, ctx, module, builder);
    }

    instance_ptr =
        replace_instance(instance_ptr, instance_type, new_instance, builder);

    LLVMValueRef parent_gep =
        coroutine_instance_parent_gep(instance_ptr, instance_type, builder);

    LLVMBuildStore(builder, old_instance_ptr, parent_gep);

    LLVMValueRef ret_opt =
        coroutine_next(instance_ptr, instance_type, _coroutine_ctx.func_type,
                       ctx, module, builder);
    LLVMBuildRet(builder, ret_opt);
    LLVMPositionBuilderAtEnd(
        builder, _coroutine_ctx.block_refs[_coroutine_ctx.current_branch]);
    return ret_opt;
  }

  LLVMValueRef val = codegen(expr, ctx, module, builder);
  LLVMValueRef ret_opt = codegen_option(val, builder);
  LLVMBuildRet(builder, ret_opt);
  LLVMPositionBuilderAtEnd(
      builder, _coroutine_ctx.block_refs[_coroutine_ctx.current_branch]);
  return ret_opt;
}

void add_recursive_cr_def_ref(ObjString fn_name, LLVMValueRef func,
                              Type *fn_type,
                              coroutine_generator_symbol_data_t symbol_data,
                              JITLangCtx *fn_ctx) {

  JITSymbol *sym =
      new_symbol(STYPE_COROUTINE_GENERATOR, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_COROUTINE_GENERATOR = symbol_data;
  sym->symbol_data.STYPE_COROUTINE_GENERATOR.recursive_ref = true;
  sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type =
      symbol_data.llvm_params_obj_type;

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef coroutine_default_block(LLVMValueRef instance,
                                     LLVMTypeRef instance_type,
                                     LLVMTypeRef ret_opt_type,
                                     LLVMBuilderRef builder) {

  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);

  // Load the parent pointer value
  LLVMValueRef parent_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMInt8Type(), 0), parent_gep, "parent_ptr");

  // Create the null comparison
  LLVMValueRef is_null = LLVMBuildICmp(
      builder, LLVMIntEQ, parent_ptr,
      LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)), "is_null");

  // Create the conditional branch
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  // Non-null path - Parent coroutine exists
  LLVMPositionBuilderAtEnd(builder, non_null_block);
  // Cast the parent pointer to the correct instance type
  LLVMValueRef parent_instance = LLVMBuildPointerCast(
      builder, parent_ptr, LLVMPointerType(instance_type, 0),
      "parent_instance");

  instance =
      replace_instance(instance, instance_type, parent_instance, builder);
  increment_instance_counter(instance, instance_type, builder);

  LLVMValueRef func = codegen_tuple_access(0, instance, instance_type, builder);

  LLVMValueRef ret_opt = LLVMBuildCall2(
      builder, coroutine_def_fn_type(instance_type, ret_opt_type), func,
      (LLVMValueRef[]){parent_instance}, 1, "coroutine_next");

  LLVMBuildRet(builder, ret_opt);

  // Continue with original null case
  LLVMPositionBuilderAtEnd(builder, continue_block);
  // Original null case code
  LLVMValueRef str = codegen_option(NULL, builder);
  LLVMBuildRet(builder, str);
}

LLVMTypeRef llvm_def_type_of_instance(Type *instance_type, JITLangCtx *ctx,
                                      LLVMModuleRef module) {

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  Type *ret_opt =
      fn_return_type(instance_type->data.T_COROUTINE_INSTANCE.yield_interface);
  LLVMTypeRef llvm_ret_opt_type = type_to_llvm_type(ret_opt, ctx->env, module);
  LLVMTypeRef llvm_def_type = LLVMFunctionType(
      llvm_ret_opt_type,
      (LLVMTypeRef[]){LLVMPointerType(coroutine_instance_type(type_to_llvm_type(
                                          params_obj_type, ctx->env, module)),
                                      0)

      },
      1, 0);
  return llvm_def_type;
}

LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder,
                           LLVMTypeRef *_llvm_def_type) {
  printf("COROUTINE DEF\n");
  print_ast(fn_ast);

  Type *fn_type = fn_ast->md;
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
      llvm_def_type_of_instance(instance_type, ctx, module);

  *_llvm_def_type = llvm_def_type;

  int num_yields = fn_ast->data.AST_LAMBDA.num_yields;

  coroutine_ctx_t prev_cr_ctx = _coroutine_ctx;
  _coroutine_ctx =
      (coroutine_ctx_t){.num_branches = num_yields + 1, .current_branch = 0};

  ObjString fn_name = fn_ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;

  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  size_t args_len = fn_ast->data.AST_LAMBDA.len;

  _coroutine_ctx.ret_option_type = llvm_ret_opt_type;

  LLVMValueRef func = LLVMAddFunction(
      module, !is_anon ? fn_name.chars : "anonymous_coroutine_def",
      llvm_def_type);

  _coroutine_ctx.func = func;
  _coroutine_ctx.func_type = llvm_def_type;
  _coroutine_ctx.instance_type = llvm_instance_type;

  LLVMSetLinkage(func, LLVMExternalLinkage);
  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef _block_refs[_coroutine_ctx.num_branches];
  _coroutine_ctx.block_refs = _block_refs;
  for (int i = 0; i < num_yields; i++) {
    _coroutine_ctx.block_refs[i] = LLVMAppendBasicBlock(func, "yield_case");
  }
  _coroutine_ctx.block_refs[num_yields] = LLVMAppendBasicBlock(func, "default");
  LLVMPositionBuilderAtEnd(builder, entry);

  if (!is_anon) {
    add_recursive_cr_def_ref(fn_name, func, fn_type,
                             (coroutine_generator_symbol_data_t){
                                 .ast = fn_ast,
                                 .stack_ptr = ctx->stack_ptr,
                                 .ret_option_type = ret_opt,
                                 .llvm_ret_option_type = llvm_ret_opt_type,
                                 .params_obj_type = params_obj_type,
                                 .llvm_params_obj_type = llvm_params_obj_type,
                                 .def_fn_type = llvm_def_type,
                                 .recursive_ref = true},
                             &fn_ctx);
  }

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  if (!((args_len == 1 && fn_type->data.T_FN.from->kind == T_VOID) ||
        (args_len == 0))) {


    LLVMValueRef params_tuple =
        codegen_tuple_access(3, instance_ptr, llvm_instance_type, builder);

    if (args_len > 1) {

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
  _coroutine_ctx.switch_val = switch_val;

  LLVMValueRef switch_inst = LLVMBuildSwitch(
      builder, switch_val, _coroutine_ctx.block_refs[num_yields], num_yields);

  for (int i = 0; i < num_yields; i++) {
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt32Type(), i, 0),
                _coroutine_ctx.block_refs[i]);
  }

  LLVMPositionBuilderAtEnd(builder, _coroutine_ctx.block_refs[0]);

  LLVMValueRef body =
      codegen(fn_ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  coroutine_default_block(instance_ptr, llvm_instance_type, llvm_ret_opt_type,
                          builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  _coroutine_ctx = prev_cr_ctx;
  return func;
}

LLVMValueRef codegen_generic_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *def_ast = ast->data.AST_LET.expr;
  Type *fn_type = def_ast->md;

  JITSymbol *sym =
      new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR, fn_type, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast = def_ast;
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.stack_ptr = ctx->stack_ptr;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
              hash_string(id_chars, id_len), sym);
  return NULL;
}

LLVMValueRef codegen_coroutine_instance(LLVMValueRef instance,
                                        Type *instance_type, LLVMValueRef def,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;

  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);

  LLVMTypeRef llvm_instance_type =
      coroutine_instance_type(llvm_params_obj_type);

  if (instance == NULL) {
    instance = heap_alloc(llvm_instance_type, ctx, builder);
  }

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, llvm_instance_type, builder);
  LLVMBuildStore(builder, def, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, llvm_instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  return instance;
}

LLVMValueRef set_instance_params(LLVMValueRef instance,
                                 LLVMTypeRef llvm_instance_type,
                                 LLVMTypeRef llvm_params_obj_type,
                                 LLVMValueRef *params, int params_len,
                                 LLVMBuilderRef builder) {

  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, llvm_instance_type, builder);



  if (params_gep) {
    if (params_len == 1) {
      LLVMBuildStore(builder, params[0], params_gep);
    } else {
      LLVMValueRef params_obj = LLVMGetUndef(llvm_params_obj_type);

      for (int i = 0; i < params_len; i++) {
        params_obj =
            LLVMBuildInsertValue(builder, params_obj, params[i], i, "");
      }

      printf("build store in params gep\n");
      LLVMDumpValue(params_obj);
      printf("\n");
      LLVMBuildStore(builder, params_obj, params_gep);
    }
  }
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

LLVMValueRef coroutine_array_iter_generator_fn(Type *expected_type, bool inf,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {

  Type *ret_opt_type = expected_type;
  ret_opt_type = ret_opt_type->data.T_FN.to;
  Type *array_el_type = type_of_option(ret_opt_type);
  LLVMTypeRef llvm_array_el_type =
      type_to_llvm_type(array_el_type, ctx->env, module);

  LLVMTypeRef llvm_array_type =
      codegen_array_type(array_el_type, ctx->env, module);
  LLVMTypeRef instance_type = coroutine_instance_type(llvm_array_type);
  LLVMTypeRef llvm_ret_opt_type =
      type_to_llvm_type(ret_opt_type, ctx->env, module);

  LLVMTypeRef func_type =
      coroutine_def_fn_type(instance_type, llvm_ret_opt_type);

  LLVMValueRef func = LLVMAddFunction(
      module, inf ? "array_iter_generator_inf" : "array_iter_generator",
      func_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef instance = LLVMGetParam(func, 0);

  LLVMValueRef array_ptr = LLVMBuildStructGEP2(builder, instance_type, instance,
                                               3, "get_tuple_element");
  LLVMValueRef array =
      LLVMBuildLoad2(builder, llvm_array_type, array_ptr, "tuple_element_load");

  LLVMValueRef array_size = codegen_get_array_size(builder, array);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, instance_type, builder);
  LLVMValueRef idx = LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "");

  // Create the null comparison
  LLVMValueRef is_null =
      LLVMBuildICmp(builder, LLVMIntUGE, idx, array_size, "is_null");

  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  LLVMPositionBuilderAtEnd(builder, non_null_block);

  increment_instance_counter(instance, instance_type, builder);

  // non-null path
  LLVMValueRef value =
      codegen_array_at(array, idx, llvm_array_el_type, module, builder);
  LLVMValueRef ret_opt = codegen_option(value, builder);
  // LLVMGetUndef(llvm_ret_opt_type);
  //
  // ret_opt =
  //     LLVMBuildInsertValue(builder, ret_opt, LLVMConstInt(LLVMInt8Type(), 0,
  //     0),
  //                          0, "insert Some tag");
  //
  // ret_opt =
  //     LLVMBuildInsertValue(builder, ret_opt, value, 1, "insert Some Value");
  LLVMBuildRet(builder, ret_opt);

  LLVMPositionBuilderAtEnd(builder, continue_block);
  // null path
  if (inf) {
    set_instance_counter(instance, instance_type,
                         LLVMConstInt(LLVMInt32Type(), 1, 0), builder);

    LLVMValueRef value =
        codegen_array_at(array, LLVMConstInt(LLVMInt32Type(), 0, 0),
                         llvm_array_el_type, module, builder);

    LLVMValueRef ret_opt = codegen_option(value, builder);
    // LLVMGetUndef(llvm_ret_opt_type);
    //
    // // non-null path
    // ret_opt = LLVMBuildInsertValue(builder, ret_opt,
    //                                LLVMConstInt(LLVMInt8Type(), 0, 0), 0,
    //                                "insert Some tag");
    //
    // ret_opt =
    //     LLVMBuildInsertValue(builder, ret_opt, value, 1, "insert Some
    //     Value");
    LLVMBuildRet(builder, ret_opt);

  } else {
    LLVMValueRef none = codegen_option(NULL, builder);
    LLVMBuildRet(builder, none);
  }

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

#define GENERIC_PTR LLVMPointerType(LLVMInt8Type(), 0)
LLVMValueRef array_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *array_arg = ast->data.AST_APPLICATION.args;
  Type *array_type = array_arg->md;
  LLVMTypeRef llvm_array_type = type_to_llvm_type(array_type, ctx->env, module);

  Type *array_el_type = array_type->data.T_CONS.args[0];

  LLVMValueRef array = codegen(array_arg, ctx, module, builder);

  LLVMTypeRef instance_type = coroutine_instance_type(llvm_array_type);

  LLVMValueRef instance = heap_alloc(instance_type, ctx, builder);

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, func, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), parent_gep);

  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, array, params_gep);

  return instance;
}

LLVMValueRef coroutine_list_iter_generator_fn(Type *expected_type,
                                              JITLangCtx *ctx,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder) {
  Type *ret_opt_type = expected_type;
  ret_opt_type = ret_opt_type->data.T_FN.to;
  Type *list_el_type = type_of_option(ret_opt_type);
  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);
  LLVMTypeRef llvm_list_type = list_type(list_el_type, ctx->env, module);
  LLVMTypeRef instance_type = coroutine_instance_type(llvm_list_type);
  LLVMTypeRef llvm_ret_opt_type =
      type_to_llvm_type(ret_opt_type, ctx->env, module);

  LLVMTypeRef func_type =
      coroutine_def_fn_type(instance_type, llvm_ret_opt_type);

  LLVMValueRef func = LLVMAddFunction(module, "list_iter_generator", func_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef instance = LLVMGetParam(func, 0);
  LLVMValueRef list = codegen_tuple_access(3, instance, instance_type, builder);

  // Create the null comparison
  LLVMValueRef is_null = LLVMBuildICmp(
      builder, LLVMIntEQ, list, LLVMConstNull(llvm_list_type), "is_null");

  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  LLVMPositionBuilderAtEnd(builder, non_null_block);

  // non-null path
  LLVMValueRef value = ll_get_head_val(list, llvm_list_el_type, builder);
  LLVMValueRef list_next = ll_get_next(list, llvm_list_el_type, builder);
  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, list_next, params_gep);
  increment_instance_counter(instance, instance_type, builder);

  LLVMValueRef ret_opt = codegen_option(value, builder);
  LLVMBuildRet(builder, ret_opt);

  LLVMPositionBuilderAtEnd(builder, continue_block);
  // null path

  LLVMValueRef none = codegen_option(NULL, builder);
  LLVMBuildRet(builder, none);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef list_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *list_arg = ast->data.AST_APPLICATION.args;
  Type *list_type = list_arg->md;
  LLVMTypeRef llvm_list_type = type_to_llvm_type(list_type, ctx->env, module);

  Type *list_el_type = list_type->data.T_CONS.args[0];

  LLVMValueRef list = codegen(list_arg, ctx, module, builder);

  LLVMTypeRef instance_type = coroutine_instance_type(llvm_list_type);

  LLVMValueRef instance = heap_alloc(instance_type, ctx, builder);

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, func, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), parent_gep);

  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, list, params_gep);

  return instance;
}

LLVMValueRef coroutine_def_from_generic(JITSymbol *sym, Type *expected_fn_type,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  Ast *fn_ast = sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast;
  Type *instance_type = fn_return_type(expected_fn_type);

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;
  Type *ret_opt =
      fn_return_type(instance_type->data.T_COROUTINE_INSTANCE.yield_interface);
  LLVMTypeRef llvm_ret_opt_type = type_to_llvm_type(ret_opt, ctx->env, module);
  LLVMTypeRef llvm_params_obj_type =
      type_to_llvm_type(params_obj_type, ctx->env, module);
  LLVMTypeRef llvm_instance_type =
      coroutine_instance_type(llvm_params_obj_type);

  LLVMTypeRef llvm_def_type = LLVMFunctionType(
      llvm_ret_opt_type,
      (LLVMTypeRef[]){LLVMPointerType(llvm_instance_type, 0)}, 1, 0);

  Ast *specific_ast = get_specific_fn_ast_variant(fn_ast, expected_fn_type);

  JITLangCtx compilation_ctx = {
      ctx->stack,
      sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.stack_ptr,
      .env = ctx->env,
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
  LLVMTypeRef _def_type;
  LLVMValueRef def = coroutine_def(specific_ast, &compilation_ctx, module,
                                   builder, &_def_type);

  return def;
}

// LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef
// module,
//                            LLVMBuilderRef builder,
//                            LLVMTypeRef *_llvm_def_type) {

LLVMValueRef generic_coroutine_instance(Ast *application_args, int args_len,
                                        Type *def_type, LLVMValueRef func,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  coroutine_generator_symbol_data_t symbol_data = {
      .stack_ptr = ctx->stack_ptr,
      .ret_option_type = empty_type(),
      .params_obj_type = empty_type(),
  };

  // get correct param obj type
  LLVMTypeRef llvm_params_obj_type =
      param_struct_type(def_type, args_len, symbol_data.params_obj_type,
                        symbol_data.ret_option_type, ctx->env, module);

  symbol_data.llvm_params_obj_type = llvm_params_obj_type;

  LLVMTypeRef instance_type =
      coroutine_instance_type(symbol_data.llvm_params_obj_type);

  LLVMTypeRef llvm_ret_option_type =
      type_to_llvm_type(symbol_data.ret_option_type, ctx->env, module);

  symbol_data.llvm_ret_option_type = llvm_ret_option_type;

  LLVMTypeRef llvm_def_fn_type =
      coroutine_def_fn_type(instance_type, llvm_ret_option_type);

  symbol_data.def_fn_type = llvm_def_fn_type;

  LLVMValueRef instance = heap_alloc(instance_type, ctx, builder);

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, func, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)),
                 parent_gep);

  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);

  if (params_gep) {
    if (args_len == 1 && ((Type *)application_args[0].md)->kind != T_VOID) {
      LLVMBuildStore(builder, codegen(application_args, ctx, module, builder),
                     params_gep);
    }
    LLVMValueRef params_obj = LLVMGetUndef(llvm_params_obj_type);

    for (int i = 0; i < args_len; i++) {
      Ast *arg = application_args + i;
      params_obj = LLVMBuildInsertValue(
          builder, params_obj, codegen(arg, ctx, module, builder), i, "");
    }
    LLVMBuildStore(builder, params_obj, params_gep);
  }

  return instance;
}

// returns a coroutine instance
LLVMValueRef coroutine_loop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  printf("CODEGEN LOOP\n");
  Ast *def_ast = ast->data.AST_APPLICATION.args;
  Ast *def_args_ast = ast->data.AST_APPLICATION.args + 1;

  printf("def: %d", def_ast->tag);
  print_ast(def_ast);
  LLVMValueRef func = codegen(def_ast, ctx, module, builder);
  LLVMDumpValue(func);
  printf("\n");

  printf("args: ");
  print_ast(def_args_ast);
  LLVMValueRef llvm_params_obj = codegen(def_args_ast, ctx, module, builder);
  LLVMDumpValue(llvm_params_obj);
  printf("\n");

  Type *instance_type = ast->md;
  LLVMTypeRef llvm_instance_type = coroutine_instance_type(type_to_llvm_type(
      instance_type->data.T_COROUTINE_INSTANCE.params_type, ctx->env, module));

  LLVMTypeRef wrapper_func_type =
      llvm_def_type_of_instance(instance_type, ctx, module);

  LLVMValueRef wrapper_func =
      LLVMAddFunction(module, "loop_wrapper", wrapper_func_type);
  LLVMSetLinkage(wrapper_func, LLVMExternalLinkage);
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_opt =
      coroutine_next(instance_ptr, llvm_instance_type, wrapper_func_type, ctx,
                     module, builder);

  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}

LLVMValueRef coroutine_map(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *func_ast = ast->data.AST_APPLICATION.args;
  Type *ret_opt = ast->md;
  ret_opt = ret_opt->data.T_FN.to;

  LLVMValueRef transform_func = codegen(func_ast, ctx, module, builder);

  Ast *instance_ast = ast->data.AST_APPLICATION.args + 1;
  print_ast(instance_ast);
  print_type(instance_ast->md);
  Type *in_opt = instance_ast->md;
  in_opt = in_opt->data.T_FN.to;

  LLVMValueRef in_instance = codegen(instance_ast, ctx, module, builder);
  LLVMTypeRef in_instance_type = coroutine_instance_type(GENERIC_PTR);

  LLVMValueRef wrapper_function = LLVMAddFunction(
      module, "wrapper_for_map",
      LLVMFunctionType(type_to_llvm_type(ret_opt, ctx->env, module),
                       (LLVMTypeRef[]){in_instance_type}, 1, 0));

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(wrapper_function, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef inst = LLVMGetParam(wrapper_function, 0);
  LLVMTypeRef generic_type = coroutine_instance_type(LLVMVoidType());
  LLVMValueRef in_original_func =
      codegen_tuple_access(0, inst, generic_type, builder);
  LLVMTypeRef in_opt_llvm_type = type_to_llvm_type(in_opt, ctx->env, module);
  LLVMTypeRef in_original_func_type =
      LLVMFunctionType(in_opt_llvm_type, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);

  LLVMValueRef in_opt_val =
      LLVMBuildCall2(builder, in_original_func_type, in_original_func,
                     (LLVMValueRef[]){inst}, 1, "");
  // Create basic block for the non-null path
  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");

  // Create the null comparison
  LLVMValueRef is_null = codegen_option_is_none(in_opt_val, builder);

  // Create the conditional branch
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  LLVMPositionBuilderAtEnd(builder, non_null_block);
  LLVMValueRef val =
      codegen_tuple_access(1, in_opt_val, in_opt_llvm_type, builder);

  Type *in = type_of_option(in_opt);
  Type *out = type_of_option(ret_opt);
  LLVMTypeRef transform_func_type =
      type_to_llvm_type(type_fn(in, out), ctx->env, module);
  val = LLVMBuildCall2(builder, transform_func_type, transform_func,
                       (LLVMValueRef[]){val}, 1, "");
  LLVMValueRef ret_opt_val = codegen_option(val, builder);
  LLVMBuildRet(builder, ret_opt_val);
  LLVMPositionBuilderAtEnd(builder, continue_block);
  LLVMBuildRet(builder, in_opt_val);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(in_instance, generic_type, builder);
  LLVMBuildStore(builder, wrapper_function, fn_gep);

  return in_instance;
}

LLVMValueRef coroutine_instance_from_def_symbol(
    LLVMValueRef _instance, JITSymbol *sym, Ast *args, int args_len,
    Type *expected_fn_type, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {

  LLVMValueRef def = sym->val;
  LLVMTypeRef llvm_def_type = sym->llvm_type;
  Type *instance_type = fn_return_type(sym->symbol_type);

  LLVMValueRef instance = codegen_coroutine_instance(_instance, instance_type,
                                                     def, ctx, module, builder);

  if (args_len == 1 && ((Type *)args[0].md)->kind == T_VOID) {
    return instance;
  }

  Type *params_obj_type = instance_type->data.T_COROUTINE_INSTANCE.params_type;

  LLVMTypeRef llvm_params_obj_type = params_obj_type_to_llvm_type(
    params_obj_type, ctx, module);

  LLVMTypeRef llvm_instance_type =
      coroutine_instance_type(llvm_params_obj_type);

  LLVMValueRef param_args[args_len];
  for (int i = 0; i < args_len; i++) {
    param_args[i] = codegen(args + i, ctx, module, builder);
  }

  set_instance_params(instance, llvm_instance_type, llvm_params_obj_type,
                      param_args, args_len, builder);

  return instance;
}
