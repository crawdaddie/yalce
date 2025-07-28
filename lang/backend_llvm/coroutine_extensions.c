#include "array.h"
#include "coroutines.h"
#include "function.h"
#include "list.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

LLVMTypeRef cor_inst_struct_type();
LLVMTypeRef cor_coroutine_fn_type();
LLVMValueRef get_instance_state_gep(LLVMValueRef instance_ptr,
                                    LLVMBuilderRef builder);
LLVMValueRef _cor_next(LLVMValueRef instance_ptr, LLVMValueRef ret_val_ref,
                       LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef null_cor_inst();

LLVMValueRef _cor_map(LLVMValueRef instance_ptr, LLVMValueRef map_fn,
                      LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_alloc(LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_loop(LLVMValueRef instance_ptr, LLVMModuleRef module,
                       LLVMBuilderRef builder);

LLVMValueRef get_instance_counter_gep(LLVMValueRef instance_ptr,
                                      LLVMBuilderRef builder);

LLVMValueRef _cor_replace(LLVMValueRef this, LLVMValueRef other,
                          LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_stop(LLVMValueRef this, LLVMModuleRef module,
                       LLVMBuilderRef builder);

LLVMValueRef _cor_wrap_effect(LLVMValueRef instance_ptr,
                              LLVMValueRef effect_handler, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  // cor *cor_wrap_effect(cor *this, EffectWrapper effect_fn);
  //
  LLVMValueRef cor_wrap_effect_func =
      LLVMGetNamedFunction(module, "cor_wrap_effect");

  LLVMTypeRef inst_type = cor_inst_struct_type();

  LLVMTypeRef cor_wrap_effect_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0), GENERIC_PTR}, 2, false);

  if (!cor_wrap_effect_func) {
    cor_wrap_effect_func =
        LLVMAddFunction(module, "cor_wrap_effect", cor_wrap_effect_type);
  }

  return LLVMBuildCall2(builder, cor_wrap_effect_type, cor_wrap_effect_func,
                        (LLVMValueRef[]){instance_ptr, effect_handler}, 2,
                        "call_cor_wrap_effect");
}

LLVMValueRef WrapCoroutineWithEffectHandler(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  Type *coroutine_type = ast->md;
  Type *ret_val_type = fn_return_type(coroutine_type);

  ret_val_type = type_of_option(ret_val_type);
  Type *expected_wrapper_type = type_fn(ret_val_type, &t_void);

  Ast *instance_ptr_arg = ast->data.AST_APPLICATION.args + 1;
  LLVMValueRef instance_ptr = codegen(instance_ptr_arg, ctx, module, builder);

  Ast wrapper_arg = *ast->data.AST_APPLICATION.args;
  wrapper_arg.md = expected_wrapper_type;

  // if (is_generic(wrapper_arg->md)) {
  //   Type *new_spec_type = type_fn(ret_val_type, &t_void);
  //   wrapper_arg->md = new_spec_type;
  // }

  LLVMValueRef wrapper_func = codegen(&wrapper_arg, ctx, module, builder);

  LLVMTypeRef llvm_ret_val_type = type_to_llvm_type(ret_val_type, ctx, module);

  LLVMValueRef func = LLVMAddFunction(
      module, "coroutine_effect_wrapper",
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){LLVMPointerType(llvm_ret_val_type, 0)},
                       1, false));

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);
  LLVMValueRef ret_val_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val =
      LLVMBuildLoad2(builder, llvm_ret_val_type, ret_val_ptr, "ret_val");

  LLVMBuildCall2(builder,
                 LLVMFunctionType(LLVMVoidType(),
                                  (LLVMTypeRef[]){llvm_ret_val_type}, 1, false),
                 wrapper_func, (LLVMValueRef[]){ret_val}, 1,
                 "call_wrapper_func");

  LLVMBuildRet(builder, LLVMGetUndef(LLVMVoidType()));
  LLVMPositionBuilderAtEnd(builder, prev_block);

  return _cor_wrap_effect(instance_ptr, func, module, builder);
}

LLVMValueRef MapCoroutineHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  Type *map_func_type = expected_fn_type->data.T_FN.from;
  Type *from = map_func_type->data.T_FN.from;
  Type *to = map_func_type->data.T_FN.to;

  Ast map_func_arg = *ast->data.AST_APPLICATION.args;

  map_func_arg.md = map_func_type;

  LLVMValueRef map_func = codegen(&map_func_arg, ctx, module, builder);

  LLVMValueRef original_coroutine =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMTypeRef llvm_to_val_type = type_to_llvm_type(to, ctx, module);
  LLVMTypeRef llvm_from_val_type = type_to_llvm_type(from, ctx, module);

  LLVMTypeRef llvm_map_func_type = LLVMFunctionType(
      llvm_to_val_type, (LLVMTypeRef[]){llvm_from_val_type}, 1, false);

  LLVMTypeRef map_wrapper_func_type = cor_coroutine_fn_type();
  LLVMValueRef map_wrapper_func = LLVMAddFunction(
      module, "coroutine_map_wrapper_func", map_wrapper_func_type);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(map_wrapper_func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef mapped_instance_ptr = LLVMGetParam(map_wrapper_func, 0);

  LLVMValueRef mapped_ret_val_ptr = LLVMGetParam(map_wrapper_func, 1);
  LLVMValueRef state_gep = get_instance_state_gep(mapped_instance_ptr, builder);
  LLVMValueRef original_instance_ptr =
      LLVMBuildLoad2(builder, LLVMPointerType(cor_inst_struct_type(), 0),
                     state_gep, "follow_gep_to_pointer");

  LLVMValueRef from_ret_val_ptr =
      LLVMBuildAlloca(builder, llvm_from_val_type, "from_ret_val_ptr");

  LLVMValueRef next =
      _cor_next(original_instance_ptr, from_ret_val_ptr, module, builder);

  LLVMValueRef mapped_val =
      LLVMBuildCall2(builder, llvm_map_func_type, map_func,
                     (LLVMValueRef[]){
                         LLVMBuildLoad2(builder, llvm_from_val_type,
                                        from_ret_val_ptr, "from_val"),
                     },
                     1, "map_call_to_val");

  LLVMValueRef phi = LLVM_IF_ELSE(
      builder,
      (LLVMBuildICmp(builder, LLVMIntEQ, next, null_cor_inst(), "is_null")),
      null_cor_inst(), ({
        LLVMBuildStore(builder, mapped_val, mapped_ret_val_ptr);
        mapped_instance_ptr;
      }));

  LLVMBuildRet(builder, phi);
  LLVMPositionBuilderAtEnd(builder, prev_block);

  return _cor_map(original_coroutine, map_wrapper_func, module, builder);
}

LLVMValueRef IterOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  Type *ltype = ast->data.AST_APPLICATION.args->md;
  Type *list_el_type = ltype->data.T_CONS.args[0];

  LLVMTypeRef llvm_list_type = type_to_llvm_type(ltype, ctx, module);

  EscapeMeta m = {.status = EA_HEAP_ALLOC};
  ast->data.AST_APPLICATION.args->ea_md = &m;
  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef func =
      LLVMAddFunction(module, "iter_of_list_cor_func", cor_coroutine_fn_type());
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);
  LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);
  LLVMValueRef list_ptr =
      LLVMBuildLoad2(builder, llvm_list_type, state_gep, "get_single_cor_arg");

  LLVMTypeRef llvm_list_el_type = type_to_llvm_type(list_el_type, ctx, module);

  LLVMValueRef is_null = ll_is_null(list_ptr, llvm_list_el_type, builder);

  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");

  LLVMBuildCondBr(builder, is_null, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMBuildRet(builder, null_cor_inst());

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef list_head =
      ll_get_head_val(list_ptr, llvm_list_el_type, builder);
  LLVMBuildStore(builder, list_head, ret_val_ref);
  LLVMBuildStore(builder, ll_get_next(list_ptr, llvm_list_el_type, builder),
                 state_gep);
  LLVMBuildRet(builder, instance_ptr);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef cor_struct = LLVMGetUndef(cor_struct_type);
  cor_struct = LLVMBuildInsertValue(builder, cor_struct,
                                    LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                    "insert_counter");
  cor_struct = LLVMBuildInsertValue(builder, cor_struct, null_cor_inst(), 2,
                                    "insert_next_null");
  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, list, 3, "insert_data");

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, func, 1, "insert_next_null");

  // LLVMValueRef alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, cor_struct_type,
  //         "cor_instance_alloca") : _cor_alloc(module, builder);

  LLVMValueRef alloca = ctx->stack_ptr > 0
                            // ? LLVMBuildAlloca(builder, cor_struct_type,
                            // "cor_instance_alloca")
                            ? _cor_alloc(module, builder)
                            : _cor_alloc(module, builder);

  // LLVMValueRef alloca =
  //         _cor_alloc(module, builder);

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef IterOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  Type *array_type = ast->data.AST_APPLICATION.args->md;
  Type *array_el_type = array_type->data.T_CONS.args[0];

  LLVMTypeRef llvm_array_el_type =
      type_to_llvm_type(array_el_type, ctx, module);

  LLVMTypeRef llvm_array_type = codegen_array_type(llvm_array_el_type);

  LLVMValueRef func =
      LLVMAddFunction(module, "iter_of_array_func", cor_coroutine_fn_type());

  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);
  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");

  LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);

  LLVMValueRef array_ptr =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_array_type, 0), state_gep,
                     "get_single_cor_arg");

  LLVMValueRef array =
      LLVMBuildLoad2(builder, llvm_array_type, array_ptr, "get_single_cor_arg");

  LLVMValueRef array_size =
      codegen_get_array_size(builder, array, llvm_array_el_type);

  LLVMValueRef is_null = LLVMBuildICmp(builder, LLVMIntULT, counter, array_size,
                                       "counter_in_bounds");

  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");

  LLVMBuildCondBr(builder, is_null, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef array_val =
      get_array_element(builder, array, counter, llvm_array_el_type);
  LLVMBuildStore(builder, array_val, ret_val_ref);
  LLVMBuildRet(builder, instance_ptr);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMBuildRet(builder, null_cor_inst());

  LLVMPositionBuilderAtEnd(builder, prev_block);

  EscapeMeta m = {.status = EA_HEAP_ALLOC};
  ast->data.AST_APPLICATION.args->ea_md = &m;
  LLVMValueRef _array =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  // LLVMValueRef array_alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, llvm_array_type,
  //         "array_struct_alloca") : LLVMBuildMalloc(builder,
  //         llvm_array_type, "array_struct_malloc");
  //
  LLVMValueRef array_alloca =
      LLVMBuildMalloc(builder, llvm_array_type, "array_struct_malloc");

  LLVMBuildStore(builder, _array, array_alloca);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef cor_struct = LLVMGetUndef(cor_struct_type);

  cor_struct = LLVMBuildInsertValue(builder, cor_struct,
                                    LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                    "insert_counter");

  cor_struct = LLVMBuildInsertValue(builder, cor_struct, null_cor_inst(), 2,
                                    "insert_next_null");

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, array_alloca, 3, "insert_data");

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, func, 1, "insert_next_null");

  // LLVMValueRef alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, cor_struct_type,
  //         "cor_instance_alloca") : _cor_alloc(module, builder);

  LLVMValueRef alloca = ctx->stack_ptr > 0 ? _cor_alloc(module, builder)
                                           : _cor_alloc(module, builder);

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  Type *arg_type = ast->data.AST_APPLICATION.args->md;
  if (is_list_type(arg_type)) {
    return IterOfListHandler(ast, ctx, module, builder);
  }

  if (is_array_type(arg_type)) {
    return IterOfArrayHandler(ast, ctx, module, builder);
  }
  return NULL;
}

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef instance_ptr =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  return _cor_loop(instance_ptr, module, builder);
}
#define INSERT_PRINTF(num_args, fmt_str, ...)                                  \
  ({                                                                           \
    LLVMValueRef format_str =                                                  \
        LLVMBuildGlobalStringPtr(builder, fmt_str, "format");                  \
    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");           \
    if (!printf_fn) {                                                          \
      LLVMTypeRef printf_type = LLVMFunctionType(                              \
          LLVMInt32Type(),                                                     \
          (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, true);       \
      printf_fn = LLVMAddFunction(module, "printf", printf_type);              \
    }                                                                          \
    LLVMBuildCall2(builder, LLVMGlobalGetValueType(printf_fn), printf_fn,      \
                   (LLVMValueRef[]){format_str, __VA_ARGS__}, num_args + 1,    \
                   "");                                                        \
  })

LLVMValueRef _build_wrapper_for_scheduled_fn(
    Type *generator_type, LLVMTypeRef llvm_generator_type,
    Type *value_struct_type, LLVMValueRef scheduler, LLVMTypeRef scheduler_type,
    LLVMValueRef effect_fn, Type *effect_fn_type, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){
                           LLVMPointerType(llvm_generator_type, 0),
                           LLVMInt64Type(),
                       },
                       2, 0);

  START_FUNC(module, "scheduler_wrapper", wrapper_fn_type)

  LLVMValueRef generator_ptr = LLVMGetParam(func, 0);
  LLVMValueRef time = LLVMGetParam(func, 1);

  LLVMTypeRef val_type = type_to_llvm_type(value_struct_type, ctx, module);

  LLVMValueRef val_ptr = LLVMBuildAlloca(builder, val_type, "val_struct_alloc");

  for (int i = 0; i < generator_type->data.T_CONS.num_args; i++) {
    Type *item_type = generator_type->data.T_CONS.args[i];

    LLVMValueRef callable_item =
        codegen_tuple_access(i, generator_ptr, llvm_generator_type, builder);

    LLVMValueRef val_gep = codegen_tuple_gep(i, val_ptr, val_type, builder);

    if (is_coroutine_type(item_type)) {
      LLVMValueRef instance_ptr = callable_item;

      Type *ret_opt_type = fn_return_type(item_type);
      Type *ret_type = type_of_option(ret_opt_type);

      LLVMValueRef _instance_ptr =
          _cor_next(instance_ptr, val_gep, module, builder);

      // Check if the returned instance pointer is null
      LLVMValueRef instance_ptr_is_null = LLVMBuildICmp(
          builder, LLVMIntEQ, _instance_ptr, null_cor_inst(), "is_null");

      LLVMBuildStore(
          builder, _instance_ptr,
          codegen_tuple_gep(i, generator_ptr, llvm_generator_type, builder));

      LLVMBasicBlockRef continue_block =
          LLVMAppendBasicBlock(func, "continue_execution");
      LLVMBasicBlockRef return_block =
          LLVMAppendBasicBlock(func, "early_return");

      LLVMBuildCondBr(builder, instance_ptr_is_null, return_block,
                      continue_block);

      LLVMPositionBuilderAtEnd(builder, return_block);
      LLVMBuildRetVoid(builder);

      LLVMPositionBuilderAtEnd(builder, continue_block);
    } else if (is_void_func(item_type)) {
      LLVMValueRef val_from_callable =
          LLVMBuildCall2(builder, type_to_llvm_type(item_type, ctx, module),
                         callable_item, NULL, 0, "call_void_item");
      LLVMBuildStore(builder, val_from_callable, val_gep);
    } else {
      LLVMBuildStore(builder, callable_item, val_gep);
    }
  }

  LLVMValueRef dur = codegen_tuple_access(0, val_ptr, val_type, builder);

  LLVMTypeRef llvm_effect_type = type_to_llvm_type(effect_fn_type, ctx, module);

  LLVMValueRef val = LLVMBuildLoad2(builder, val_type, val_ptr, "");
  LLVMBuildCall2(builder, llvm_effect_type, effect_fn,
                 (LLVMValueRef[]){val, time}, 2, "");

  LLVMValueRef scheduler_call =
      LLVMBuildCall2(builder, scheduler_type, scheduler,
                     (LLVMValueRef[]){
                         time,
                         dur,
                         func,
                         generator_ptr,
                     },
                     4, "schedule_next");

  LLVMBuildRetVoid(builder);

  END_FUNC
  return func;
}

LLVMValueRef RunInSchedulerHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  Ast *fo_ast = ast->data.AST_APPLICATION.args;
  LLVMValueRef base_time = codegen(fo_ast, ctx, module, builder);

  Ast *scheduler_ast = ast->data.AST_APPLICATION.args + 1;
  Type *scheduler_type = scheduler_ast->md;

  Ast *effect_ast = ast->data.AST_APPLICATION.args + 2;
  Type *effect_type = effect_ast->md;

  Ast *generator_ast = ast->data.AST_APPLICATION.args + 3;
  Type *generator_type = generator_ast->md;

  LLVMValueRef scheduler =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *value_struct_type = effect_type->data.T_FN.from;

  LLVMTypeRef llvm_generator_type =
      type_to_llvm_type(generator_type, ctx, module);

  LLVMValueRef generator_alloca =
      LLVMBuildMalloc(builder, llvm_generator_type, "");

  LLVMValueRef generator = codegen(generator_ast, ctx, module, builder);
  LLVMBuildStore(builder, generator, generator_alloca);

  LLVMValueRef effect_fn = codegen(effect_ast, ctx, module, builder);

  LLVMTypeRef llvm_scheduler_type =
      type_to_llvm_type(scheduler_type, ctx, module);

  LLVMValueRef wrapper_fn = _build_wrapper_for_scheduled_fn(
      generator_type, llvm_generator_type, value_struct_type, scheduler,
      llvm_scheduler_type, effect_fn, effect_type, ctx, module, builder);

  // compile value generator (args[2]) to struct 'U
  // create wrapper function 'U -> Int -> ()
  // in wrapper function, get 1st arg 'U, & Int frame_offset
  // for each member of 'U, call member and construct
  // struct 'V with values
  // call sink function args[1] with 'V & frame_offset
  // later take first value from 'V (Float - dur) and
  // call schedule_event (args[0]) with wrapper function, dur & 'U
  return LLVMBuildCall2(builder, llvm_scheduler_type, scheduler,
                        (LLVMValueRef[]){
                            base_time,
                            LLVMConstReal(LLVMDoubleType(), 0.),
                            wrapper_fn,
                            generator_alloca,
                        },
                        4, "");
}

LLVMValueRef _build_wrapper_for_scheduled_routine(
    Type *generator_type, LLVMTypeRef llvm_generator_type,
    LLVMValueRef scheduler, LLVMTypeRef scheduler_type, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){
                           LLVMPointerType(llvm_generator_type, 0),
                           LLVMInt64Type(),
                       },
                       2, 0);

  START_FUNC(module, "scheduler_wrapper", wrapper_fn_type)

  LLVMValueRef generator_ptr = LLVMGetParam(func, 0);

  LLVMValueRef timestamp = LLVMGetParam(func, 1);
  LLVMTypeRef val_type = type_to_llvm_type(&t_num, ctx, module);
  LLVMValueRef val_ptr = LLVMBuildAlloca(builder, val_type, "val_struct_alloc");

  LLVMValueRef instance_ptr =
      _cor_next(generator_ptr, val_ptr, module, builder);

  LLVMValueRef val = LLVMBuildLoad2(builder, val_type, val_ptr, "");

  LLVMValueRef scheduler_call =
      LLVMBuildCall2(builder, scheduler_type, scheduler,
                     (LLVMValueRef[]){
                         timestamp,
                         val,
                         func,
                         instance_ptr,
                     },
                     4, "schedule_next");

  LLVMBuildRetVoid(builder);

  END_FUNC
  return func;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  Ast *timestamp_ast = ast->data.AST_APPLICATION.args;
  LLVMValueRef ts_val = codegen(timestamp_ast, ctx, module, builder);

  Ast *scheduler_ast = ast->data.AST_APPLICATION.args + 1;
  Type *scheduler_type = scheduler_ast->md;

  Ast *generator_ast = ast->data.AST_APPLICATION.args + 2;
  Type *generator_type = generator_ast->md;

  LLVMValueRef scheduler = codegen(scheduler_ast, ctx, module, builder);

  LLVMTypeRef llvm_generator_type =
      type_to_llvm_type(generator_type, ctx, module);

  LLVMValueRef generator = codegen(generator_ast, ctx, module, builder);

  LLVMTypeRef llvm_scheduler_type =
      type_to_llvm_type(scheduler_type, ctx, module);

  LLVMValueRef wrapper_fn = _build_wrapper_for_scheduled_routine(
      generator_type, llvm_generator_type, scheduler, llvm_scheduler_type, ctx,
      module, builder);

  // compile value generator (args[2]) to struct 'U
  // create wrapper function 'U -> Int -> ()
  // in wrapper function, get 1st arg 'U, & Int frame_offset
  // for each member of 'U, call member and construct
  // struct 'V with values
  // call sink function args[1] with 'V & frame_offset
  // later take first value from 'V (Float - dur) and
  // call schedule_event (args[0]) with wrapper function, dur & 'U
  return LLVMBuildCall2(builder, llvm_scheduler_type, scheduler,
                        (LLVMValueRef[]){
                            ts_val,
                            LLVMConstReal(LLVMDoubleType(), 0.),
                            wrapper_fn,
                            generator,
                        },
                        4, "");
  // return generator;
}
LLVMValueRef CorReplaceHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef other_cor =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMValueRef this_cor =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  return _cor_replace(this_cor, other_cor, module, builder);
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef cor =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  return _cor_stop(cor, module, builder);
}

LLVMValueRef CoroutineEndHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMBasicBlockRef current_case_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_case_block);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  // Store null in the return value to indicate end
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), ret_val_ref);

  LLVMBasicBlockRef end_block = ctx->coro_ctx->switch_default;

  LLVMBuildBr(builder, end_block);
  return LLVMConstNull(type_to_llvm_type(ast->md, ctx, module));
}

LLVMValueRef UseOrFinishHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef result =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef is_none = codegen_option_is_none(result, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_block);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  LLVMBasicBlockRef end_block = ctx->coro_ctx->switch_default;

  if (!end_block) {
    fprintf(stderr, "Error: coroutine_iter_end block not found\n");
    return NULL;
  }

  LLVMBasicBlockRef none_block = LLVMAppendBasicBlock(func, "option_is_none");
  LLVMBasicBlockRef some_block = LLVMAppendBasicBlock(func, "option_has_value");

  LLVMBuildCondBr(builder, is_none, none_block, some_block);

  LLVMPositionBuilderAtEnd(builder, none_block);
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), ret_val_ref);
  LLVMBuildBr(builder, end_block);

  LLVMPositionBuilderAtEnd(builder, some_block);
  LLVMValueRef value_field = LLVMBuildExtractValue(builder, result, 1, "value");

  return value_field;
}
