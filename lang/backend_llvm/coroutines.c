#include "backend_llvm/coroutines.h"
#include "adt.h"
#include "array.h"
#include "function.h"
#include "list.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMTypeRef cor_coroutine_fn_type() {
  // typedef void *(*CoroutineFn)(void *coroutine, void *ret_val);
  return LLVMFunctionType(GENERIC_PTR,
                          (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);
}

LLVMTypeRef cor_inst_struct_type() {
  // typedef struct cor {
  //   int counter;
  //   CoroutineFn fn_ptr;
  //   struct cor *next;
  //   void *argv;
  // } cor;
  LLVMTypeRef types[4] = {
      LLVMInt32Type(), // counter @ 0
      GENERIC_PTR,     // fn ptr @ 1
      GENERIC_PTR,     // next ptr
      GENERIC_PTR,     // void *argv - state
  };
  LLVMTypeRef instance_struct_type = LLVMStructType(types, 4, 0);
  return instance_struct_type;
}

LLVMTypeRef create_coroutine_state_type(Type *constructor_type, JITLangCtx *ctx,
                                        LLVMModuleRef module) {

  int args_len = fn_type_args_len(constructor_type);

  Type *state_arg_types[args_len];

  if (args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    args_len = 0;
    return NULL;
  }

  Type *f = constructor_type;
  LLVMTypeRef llvm_state_arg_types[args_len];
  for (int i = 0; i < args_len; i++) {
    Type *arg_type = f->data.T_FN.from;
    state_arg_types[i] = arg_type;
    llvm_state_arg_types[i] = type_to_llvm_type(arg_type, ctx->env, module);
    f = f->data.T_FN.to;
  }
  if (args_len == 1) {
    return llvm_state_arg_types[0];
  }

  LLVMTypeRef instance_state_struct_type =
      LLVMStructType(llvm_state_arg_types, args_len, 0);

  return instance_state_struct_type;
}

LLVMValueRef get_instance_counter_gep(LLVMValueRef instance_ptr,
                                      LLVMBuilderRef builder) {
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, cor_inst_struct_type(), instance_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 0, 0)  // Get counter
                    },
                    2, "instance_counter_gep");
  return element_ptr;
}

LLVMValueRef get_instance_state_gep(LLVMValueRef instance_ptr,
                                    LLVMBuilderRef builder) {
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, cor_inst_struct_type(), instance_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 3, 0)  // Get counter
                    },
                    2, "instance_state_gep");
  return element_ptr;
}

LLVMValueRef _cor_next(LLVMValueRef instance_ptr, LLVMValueRef ret_val_ref,
                       LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_next_func = LLVMGetNamedFunction(module, "cor_next");
  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef ret_val_ref_type = GENERIC_PTR;

  LLVMTypeRef cor_next_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0), ret_val_ref_type}, 2,
      false);

  if (!cor_next_func) {
    cor_next_func = LLVMAddFunction(module, "cor_next", cor_next_type);
  }

  return LLVMBuildCall2(builder, cor_next_type, cor_next_func,
                        (LLVMValueRef[]){instance_ptr, ret_val_ref}, 2,
                        "call_cor_next");
}

LLVMValueRef _cor_init(LLVMValueRef instance_ptr, LLVMValueRef new_fn,
                       LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_init_func = LLVMGetNamedFunction(module, "cor_init");
  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef fn_type = cor_coroutine_fn_type();

  LLVMTypeRef cor_init_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0), fn_type}, 2, false);

  if (!cor_init_func) {
    cor_init_func = LLVMAddFunction(module, "cor_init", cor_init_type);
  }
  return LLVMBuildCall2(builder, cor_init_type, cor_init_func,
                        (LLVMValueRef[]){instance_ptr, new_fn}, 2,
                        "call_cor_init");
}

LLVMValueRef _cor_alloc(LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_alloc_func = LLVMGetNamedFunction(module, "cor_alloc");
  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef cor_alloc_type = LLVMFunctionType(LLVMPointerType(inst_type, 0),
                                                (LLVMTypeRef[]){}, 0, false);

  if (!cor_alloc_func) {
    cor_alloc_func = LLVMAddFunction(module, "cor_alloc", cor_alloc_type);
  }
  return LLVMBuildCall2(builder, cor_alloc_type, cor_alloc_func,
                        (LLVMValueRef[]){}, 0, "call_cor_alloc");
}

LLVMValueRef _cor_defer(LLVMValueRef instance_ptr, LLVMValueRef next_struct,
                        LLVMValueRef ret_val_ref, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  // cor *cor_defer(cor *this, cor next_struct, void *ret_val);
  LLVMValueRef cor_defer_func = LLVMGetNamedFunction(module, "cor_defer");
  LLVMTypeRef inst_type = cor_inst_struct_type();

  LLVMTypeRef cor_defer_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){
          LLVMPointerType(inst_type, 0),
          LLVMPointerType(
              inst_type, 0), // LLVM Implicitly converts struct args to pointers
          GENERIC_PTR},
      3, false);

  if (!cor_defer_func) {
    cor_defer_func = LLVMAddFunction(module, "cor_defer", cor_defer_type);
  }

  return LLVMBuildCall2(
      builder, cor_defer_type, cor_defer_func,
      (LLVMValueRef[]){instance_ptr, next_struct, ret_val_ref}, 3,
      "call_cor_defer");
}
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
LLVMValueRef _cor_map(LLVMValueRef instance_ptr, LLVMValueRef map_fn,
                      LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_map_func = LLVMGetNamedFunction(module, "cor_map");
  LLVMTypeRef inst_type = cor_inst_struct_type();

  LLVMTypeRef cor_map_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0), cor_coroutine_fn_type()},
      2, false);

  if (!cor_map_func) {
    cor_map_func = LLVMAddFunction(module, "cor_map", cor_map_type);
  }

  return LLVMBuildCall2(builder, cor_map_type, cor_map_func,
                        (LLVMValueRef[]){instance_ptr, map_fn}, 2,
                        "call_cor_map");
}

LLVMValueRef _cor_loop(LLVMValueRef instance_ptr, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  LLVMValueRef cor_loop_func = LLVMGetNamedFunction(module, "cor_loop");
  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef fn_type = cor_coroutine_fn_type();

  LLVMTypeRef cor_loop_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0)}, 1, false);

  if (!cor_loop_func) {
    cor_loop_func = LLVMAddFunction(module, "cor_loop", cor_loop_type);
  }

  return LLVMBuildCall2(builder, cor_loop_type, cor_loop_func,
                        (LLVMValueRef[]){instance_ptr}, 1, "call_cor_loop");
}

LLVMValueRef null_cor_inst() {
  LLVMValueRef null_ptr =
      LLVMConstNull(LLVMPointerType(cor_inst_struct_type(), 0));
  return null_ptr;
}

LLVMValueRef _cor_reset(LLVMValueRef instance_ptr, LLVMValueRef next_struct,
                        LLVMValueRef ret_val_ref, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  // cor *cor_reset(cor *this, cor next_struct, void *ret_val);
  LLVMValueRef cor_reset_func = LLVMGetNamedFunction(module, "cor_reset");
  LLVMTypeRef inst_type = cor_inst_struct_type();

  LLVMTypeRef cor_reset_type = LLVMFunctionType(
      LLVMPointerType(inst_type, 0),
      (LLVMTypeRef[]){
          LLVMPointerType(inst_type, 0),
          LLVMPointerType(
              inst_type, 0), // LLVM Implicitly converts struct args to pointers
          GENERIC_PTR},
      3, false);

  if (!cor_reset_func) {
    cor_reset_func = LLVMAddFunction(module, "cor_reset", cor_reset_type);
  }
  return LLVMBuildCall2(
      builder, cor_reset_type, cor_reset_func,
      (LLVMValueRef[]){instance_ptr, next_struct, ret_val_ref}, 3,
      "call_cor_reset");
}

static LLVMValueRef compile_coroutine_fn(Type *constructor_type, Ast *ast,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {

  Type *ret_opt_type = fn_return_type(constructor_type);

  Type *ret_type = type_of_option(ret_opt_type);

  LLVMTypeRef fn_type = cor_coroutine_fn_type();

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  LLVMValueRef func = LLVMAddFunction(
      module, is_anon ? "anon_coroutine_func" : fn_name.chars, fn_type);
  if (func == NULL) {
    fprintf(stderr,
            "Error: failure creating coroutine switch-resume function\n");
    print_ast_err(ast);
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);
  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
  fn_ctx.num_coroutine_yields = ast->data.AST_LAMBDA.num_yields;
  fn_ctx.current_yield = 0;

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  if (!is_anon) {
    add_recursive_fn_ref(fn_name, func, constructor_type, &fn_ctx);
  }

  LLVMTypeRef state_struct_type =
      create_coroutine_state_type(constructor_type, ctx, module);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);
  int fn_len = ast->data.AST_LAMBDA.len;

  if (fn_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
  } else {
    LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);
    state_gep = LLVMBuildLoad2(builder, LLVMPointerType(state_struct_type, 0),
                               state_gep, "follow_gep_to_pointer");
    Type *f = constructor_type;

    if (fn_len == 1 && f->data.T_FN.from->kind != T_VOID) {
      LLVMValueRef param_val = LLVMBuildLoad2(builder, state_struct_type,
                                              state_gep, "get_single_cor_arg");
      codegen_pattern_binding(ast->data.AST_LAMBDA.params, param_val,
                              f->data.T_FN.from, &fn_ctx, module, builder);
    } else {

      LLVMValueRef state_struct = LLVMBuildLoad2(
          builder, state_struct_type, state_gep, "get_full_state_struct");

      for (int i = 0; i < fn_len; i++) {
        Ast *param_ast = ast->data.AST_LAMBDA.params + i;
        Type *param_type = f->data.T_FN.from;

        codegen_pattern_binding(
            param_ast,
            LLVMBuildExtractValue(builder, state_struct, i, "get_state_param"),
            param_type, &fn_ctx, module, builder);
        f = f->data.T_FN.to;
      }
    }
  }

  // set up default block, ie coroutine end -> returns NULL
  LLVMBasicBlockRef switch_default_block =
      LLVMAppendBasicBlock(func, "coroutine_iter_end");
  LLVMPositionBuilderAtEnd(builder, switch_default_block);

  LLVMBuildRet(builder, null_cor_inst());
  LLVMPositionBuilderAtEnd(builder, block);

  // construct switch which takes the coroutine instance counter as the value
  // to switch on
  LLVMValueRef switch_ref = LLVMBuildSwitch(
      builder, counter, switch_default_block, ast->data.AST_LAMBDA.num_yields);
  fn_ctx.yield_switch_ref = switch_ref;
  LLVMBasicBlockRef case_0 = LLVMAppendBasicBlock(func, "coroutine_iter_0");
  LLVMPositionBuilderAtEnd(builder, case_0);

  // add first case for initial 0th yield - all computations up to the first
  // yield occur in this block
  LLVMAddCase(switch_ref, LLVMConstInt(LLVMInt32Type(), 0, 0), case_0);
  LLVMPositionBuilderAtEnd(builder, case_0);

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, block);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);
  return func;
}

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *fn_ast,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {
  Type *constructor_type = fn_ast->md;
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  if (is_generic(constructor_type)) {
    // TODO: compile generic coroutine functions
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, constructor_type, NULL, NULL);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame = ctx->frame;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env = ctx->env;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = fn_ast;

    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                sym);
    return NULL;
  }

  LLVMValueRef constructor =
      compile_coroutine_fn(constructor_type, fn_ast, ctx, module, builder);

  JITSymbol *sym =
      new_symbol(STYPE_FUNCTION, constructor_type, constructor, NULL);
  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);
  return NULL;
}

LLVMValueRef create_cor_inst_struct(LLVMBuilderRef builder, LLVMValueRef fn,
                                    LLVMValueRef state) {
  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  LLVMValueRef cor_struct = LLVMGetUndef(cor_struct_type);
  cor_struct = LLVMBuildInsertValue(builder, cor_struct,
                                    LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                    "insert_counter");

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, fn, 1, "insert_fn_ptr");

  cor_struct = LLVMBuildInsertValue(builder, cor_struct, null_cor_inst(), 2,
                                    "insert_next_null");
  cor_struct = LLVMBuildInsertValue(
      builder, cor_struct,
      state ? state : LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)), 3,
      "insert_state_ptr");
  // LLVMConstStruct();
  return cor_struct;
}

LLVMValueRef create_coroutine_state_ptr(Type *constructor_type, Ast *args,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  int args_len = fn_type_args_len(constructor_type);
  Type *state_arg_types[args_len];

  if (args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    args_len = 0;
    return NULL;
  }

  Type *f = constructor_type;
  LLVMTypeRef llvm_state_arg_types[args_len];
  for (int i = 0; i < args_len; i++) {
    Type *arg_type = f->data.T_FN.from;
    state_arg_types[i] = arg_type;
    llvm_state_arg_types[i] = type_to_llvm_type(arg_type, ctx->env, module);
    f = f->data.T_FN.to;
  }

  if (args_len == 1) {
    LLVMValueRef state_ptr_alloca =
        LLVMBuildAlloca(builder, llvm_state_arg_types[0], "");
    LLVMBuildStore(builder, codegen(args, ctx, module, builder),
                   state_ptr_alloca);
    return state_ptr_alloca;
  }

  LLVMTypeRef instance_state_struct_type =
      LLVMStructType(llvm_state_arg_types, args_len, 0);

  LLVMValueRef inst_state_struct = LLVMGetUndef(instance_state_struct_type);

  for (int i = 0; i < args_len; i++) {
    Ast *arg_ast = args + i;
    LLVMValueRef state_arg_val = codegen(arg_ast, ctx, module, builder);
    inst_state_struct =
        LLVMBuildInsertValue(builder, inst_state_struct, state_arg_val, i,
                             "initial_instance_state_arg");
  }

  LLVMValueRef state_ptr_alloca =
      LLVMBuildAlloca(builder, instance_state_struct_type, "");

  LLVMBuildStore(builder, inst_state_struct, state_ptr_alloca);
  return state_ptr_alloca;
}

LLVMValueRef create_coroutine_instance_from_generic_constructor(
    JITSymbol *sym, Type *expected_type, Ast *args, int args_len,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef func = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_type);

  if (!func) {

    JITLangCtx compilation_ctx = *ctx;

    Type *generic_type = sym->symbol_type;
    compilation_ctx.stack_ptr =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

    compilation_ctx.env = create_env_for_generic_fn(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env, generic_type,
        expected_type);

    Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    fn_ast.md = expected_type;

    LLVMValueRef specific_fn = compile_coroutine_fn(
        expected_type, &fn_ast, &compilation_ctx, module, builder);

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns = specific_fns_extend(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_type,
        specific_fn);
    func = specific_fn;
  }

  Type *constructor_type = expected_type;

  LLVMValueRef state_struct_ptr =
      create_coroutine_state_ptr(constructor_type, args, ctx, module, builder);

  // Create and initialize the cor struct
  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, func, state_struct_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef create_coroutine_instance_from_constructor(
    JITSymbol *sym, Ast *args, int args_len, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *constructor_type = sym->symbol_type;

  LLVMValueRef state_struct_ptr =
      create_coroutine_state_ptr(constructor_type, args, ctx, module, builder);

  // Create and initialize the cor struct
  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, sym->val, state_struct_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : _cor_alloc(module, builder);
  // LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

static LLVMValueRef wrap_yield_result_in_option(LLVMValueRef instance_ptr,
                                                LLVMValueRef ret_val_ref,
                                                LLVMTypeRef ret_val_type,
                                                LLVMBuilderRef builder) {
  LLVMValueRef sel = LLVMBuildSelect(
      builder,
      LLVMBuildICmp(builder, LLVMIntEQ, instance_ptr, null_cor_inst(),
                    "is_null"),
      codegen_none_typed(builder, ret_val_type),
      codegen_some(LLVMBuildLoad2(builder, ret_val_type, ret_val_ref,
                                  "load_from_ret_val_ref"),
                   builder),
      "select Some ret_val or None");
  return sel;
}

LLVMValueRef yield_from_coroutine_instance(JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  Type *coroutine_type = sym->symbol_type;
  Type *ret_opt_type = fn_return_type(coroutine_type);
  Type *ret_type = type_of_option(ret_opt_type);
  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx->env, module);
  LLVMValueRef instance_ptr = sym->val;

  LLVMValueRef ret_val_ref =
      LLVMBuildAlloca(builder, llvm_ret_type, "ret_val_ref");

  instance_ptr = _cor_next(instance_ptr, ret_val_ref, module, builder);

  // LLVMValueRef instance_struct =
  //     LLVMBuildLoad2(builder, cor_inst_struct_type(), instance_ptr, "");
  // LLVMBuildStore(builder, instance_struct, sym->val);

  return wrap_yield_result_in_option(instance_ptr, ret_val_ref, llvm_ret_type,
                                     builder);
}

static LLVMValueRef codegen_yield_nested_coroutine(
    JITSymbol *sym, bool is_recursive_ref, LLVMValueRef instance_ptr,
    Type *state_type, Ast *args, int args_len, LLVMValueRef ret_val_ref,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef llvm_state_type = type_to_llvm_type(state_type, ctx->env, module);

  LLVMValueRef new_state_ptr;

  if (args_len == 1 && state_type->kind == T_VOID) {
    new_state_ptr = NULL;
  } else {

    if (is_recursive_ref == true) {
      // reuse instance_ptr's alloced state
      new_state_ptr = get_instance_state_gep(instance_ptr, builder);
      new_state_ptr = LLVMBuildLoad2(
          builder, LLVMPointerType(llvm_state_type, 0), new_state_ptr, "");
    } else {
      new_state_ptr = LLVMBuildMalloc(builder, llvm_state_type, "");
    }
    if (args_len == 1) {
      LLVMBuildStore(builder, codegen(args, ctx, module, builder),
                     new_state_ptr);
    } else {

      LLVMValueRef state_struct = LLVMGetUndef(llvm_state_type);

      for (int i = 0; i < args_len; i++) {
        Ast *arg = args + i;
        LLVMValueRef arg_val = codegen(arg, ctx, module, builder);

        state_struct =
            LLVMBuildInsertValue(builder, state_struct, arg_val, i, "");
      }
      LLVMBuildStore(builder, state_struct, new_state_ptr);
    }
  }

  Type *constructor_type = sym->symbol_type;

  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, sym->val, new_state_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  LLVMValueRef next_instance_ptr =
      LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca");

  LLVMBuildStore(builder, cor_struct, next_instance_ptr);
  // the following functions _cor_reset & _cor_defer are declared to take a
  // struct as a second arg, but we actually need to pass pointers to structs as
  // llvm compiles struct passing as pointers implicitly
  if (is_recursive_ref == true) {
    LLVMValueRef new_instance_ptr = _cor_reset(instance_ptr, next_instance_ptr,
                                               ret_val_ref, module, builder);
    return new_instance_ptr;

  } else {
    LLVMValueRef new_instance_ptr = _cor_defer(instance_ptr, next_instance_ptr,
                                               ret_val_ref, module, builder);
    return new_instance_ptr;
  }

  return NULL;
}

JITSymbol *is_nested_coroutine_expr(Ast *ast, JITLangCtx *ctx) {

  if (ast->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

    const char *sym_name =
        ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    if (!sym) {
      fprintf(stderr, "Error callable symbol %s not found in scope\n",
              sym_name);
      return NULL;
    }

    if (is_coroutine_constructor_type(sym->symbol_type)) {
      return sym;
    }
  }
  return NULL;
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMBasicBlockRef current_case_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_case_block);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  ctx->current_yield++;
  LLVMBasicBlockRef next_case_block;
  if (ctx->current_yield < ctx->num_coroutine_yields) {
    char branch_name[19];
    sprintf(branch_name, "coroutine_iter_%d", ctx->current_yield);
    next_case_block = LLVMAppendBasicBlock(func, branch_name);

    LLVMValueRef switch_ref = ctx->yield_switch_ref;
    LLVMAddCase(switch_ref,
                LLVMConstInt(LLVMInt32Type(), ctx->current_yield, 0),
                next_case_block);
  }

  Ast *yield_expr = ast->data.AST_YIELD.expr;
  JITSymbol *sym = is_nested_coroutine_expr(yield_expr, ctx);

  if (sym != NULL) {
    bool is_recursive_ref = sym->symbol_type == STYPE_FUNCTION &&
                            sym->symbol_data.STYPE_FUNCTION.recursive_ref;

    if (sym->type == STYPE_GENERIC_FUNCTION && sym->val == NULL) {
      if (sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
        LLVMValueRef new_instance_ptr =
            sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
                yield_expr, ctx, module, builder);

        new_instance_ptr = _cor_defer(instance_ptr, new_instance_ptr,
                                      ret_val_ref, module, builder);

        LLVMBuildRet(builder, new_instance_ptr);
      }
    } else {
      Type *symbol_type = sym->symbol_type;

      int args_len = fn_type_args_len(symbol_type);

      if (args_len > 1) {
        Type *contained_types[args_len];
        Type *f = symbol_type;
        for (int i = 0; i < args_len; i++) {
          contained_types[i] = f->data.T_FN.from;
          f = f->data.T_FN.to;
        }
        Type state_type = {T_CONS,
                           {.T_CONS = {.name = "coroutine_state",
                                       .args = contained_types,
                                       .num_args = args_len}}};

        LLVMValueRef new_instance_ptr = codegen_yield_nested_coroutine(
            sym, is_recursive_ref, instance_ptr, &state_type,
            yield_expr->data.AST_APPLICATION.args,
            yield_expr->data.AST_APPLICATION.len, ret_val_ref, ctx, module,
            builder);

        LLVMBuildRet(builder, new_instance_ptr);

      } else {
        Type *state_type;
        if (args_len == 1 && symbol_type->data.T_FN.from->kind == T_VOID) {
          args_len = 1;
          state_type = &t_void;
        } else if (args_len == 1) {
          state_type = symbol_type->data.T_FN.from;
        }
        LLVMValueRef new_instance_ptr = codegen_yield_nested_coroutine(
            sym, is_recursive_ref, instance_ptr, state_type,
            yield_expr->data.AST_APPLICATION.args,
            yield_expr->data.AST_APPLICATION.len, ret_val_ref, ctx, module,
            builder);

        LLVMBuildRet(builder, new_instance_ptr);
      }
    }

  } else {
    LLVMValueRef expr_val = codegen(yield_expr, ctx, module, builder);
    LLVMBuildStore(builder, expr_val, ret_val_ref);
    LLVMBuildRet(builder, instance_ptr);
  }
  if (next_case_block) {
    LLVMPositionBuilderAtEnd(builder, next_case_block);
  }
  return NULL;
}

LLVMValueRef WrapCoroutineWithEffectHandler(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  Type *coroutine_type = ast->md;
  Type *ret_val_type = fn_return_type(coroutine_type);
  ret_val_type = type_of_option(ret_val_type);

  Ast *instance_ptr_arg = ast->data.AST_APPLICATION.args + 1;
  LLVMValueRef instance_ptr = codegen(instance_ptr_arg, ctx, module, builder);

  Ast *wrapper_arg = ast->data.AST_APPLICATION.args;

  if (is_generic(wrapper_arg->md)) {
    Type *new_spec_type = type_fn(ret_val_type, &t_void);
    wrapper_arg->md = new_spec_type;
  }

  LLVMValueRef wrapper_func = codegen(wrapper_arg, ctx, module, builder);

  LLVMTypeRef llvm_ret_val_type =
      type_to_llvm_type(ret_val_type, ctx->env, module);

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

  Ast *map_func_arg = ast->data.AST_APPLICATION.args;
  map_func_arg->md = map_func_type;
  LLVMValueRef map_func = codegen(map_func_arg, ctx, module, builder);

  LLVMValueRef original_coroutine =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMTypeRef llvm_to_val_type = type_to_llvm_type(to, ctx->env, module);
  LLVMTypeRef llvm_from_val_type = type_to_llvm_type(from, ctx->env, module);

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

LLVMValueRef _IterOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  Type *ltype = ast->data.AST_APPLICATION.args->md;
  Type *list_el_type = ltype->data.T_CONS.args[0];
  LLVMTypeRef llvm_list_type = type_to_llvm_type(ltype, ctx->env, module);

  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  LLVMValueRef cor_struct = LLVMGetUndef(cor_struct_type);
  cor_struct = LLVMBuildInsertValue(builder, cor_struct,
                                    LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                    "insert_counter");

  cor_struct = LLVMBuildInsertValue(builder, cor_struct, null_cor_inst(), 2,
                                    "insert_next_null");

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, list, 3, "insert_data");

  LLVMValueRef func =
      LLVMAddFunction(module, "iter_of_list_cor_func", cor_coroutine_fn_type());

  LLVMSetLinkage(func, LLVMExternalLinkage);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");

  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);
  LLVMValueRef list_ptr =
      LLVMBuildLoad2(builder, llvm_list_type, state_gep, "get_single_cor_arg");
  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);
  LLVMValueRef is_null = ll_is_null(list_ptr, llvm_list_el_type, builder);

  LLVMValueRef phi = ({
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "merge");

    LLVMBuildCondBr(builder, is_null, then_block, else_block);

    // Position in then block and build
    LLVMPositionBuilderAtEnd(builder, then_block);
    LLVMValueRef then_result = null_cor_inst();
    LLVMBuildBr(builder, merge_block);
    LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

    // Position in else block and build
    LLVMPositionBuilderAtEnd(builder, else_block);
    LLVMValueRef list_head =
        ll_get_head_val(list_ptr, llvm_list_el_type, builder);
    LLVMBuildStore(builder, list_head, ret_val_ref);
    LLVMBuildStore(builder, ll_get_next(list_ptr, llvm_list_el_type, builder),
                   state_gep);
    LLVMValueRef else_result = instance_ptr;
    LLVMBuildBr(builder, merge_block);
    LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

    // Build phi in merge block
    LLVMPositionBuilderAtEnd(builder, merge_block);
    LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");
    LLVMValueRef incoming_vals[] = {then_result, else_result};
    LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
    LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);
    phi;
  });
  LLVMBuildRet(builder, phi);
  LLVMPositionBuilderAtEnd(builder, prev_block);

  cor_struct =
      LLVMBuildInsertValue(builder, cor_struct, func, 1, "insert_next_null");

  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : _cor_alloc(module, builder);
  // LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}
LLVMValueRef IterOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *ltype = ast->data.AST_APPLICATION.args->md;
  Type *list_el_type = ltype->data.T_CONS.args[0];
  LLVMTypeRef llvm_list_type = type_to_llvm_type(ltype, ctx->env, module);
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
  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);
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

  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
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
  LLVMTypeRef llvm_array_type = type_to_llvm_type(array_type, ctx->env, module);
  LLVMTypeRef llvm_array_el_type =
      type_to_llvm_type(array_el_type, ctx->env, module);

  LLVMValueRef func = LLVMAddFunction(module, "iter_of_list_array_func",
                                      cor_coroutine_fn_type());
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);
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

  LLVMValueRef _array =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef array_alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, llvm_array_type, "array_struct_alloca")
          : LLVMBuildMalloc(builder, llvm_array_type, "array_struct_malloc");

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

  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : _cor_alloc(module, builder);

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef instance_ptr =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  return _cor_loop(instance_ptr, module, builder);
}

LLVMValueRef CorPlayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef instance_ptr =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  return _cor_loop(instance_ptr, module, builder);
}

LLVMValueRef codegen_struct_of_coroutines(Ast *ast, JITLangCtx *ctx,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder) {
  Type *coroutine_type = ast->md;
  Type *ret_opt_type = fn_return_type(coroutine_type);
  Type *ret_type = type_of_option(ret_opt_type);

  int len = ret_type->data.T_CONS.num_args;

  LLVMTypeRef struct_member_types[len];
  for (int i = 0; i < len; i++) {
    struct_member_types[i] =
        type_to_llvm_type(ast->data.AST_LIST.items[i].md, ctx->env, module);
  }
  LLVMTypeRef llvm_state_struct_type =
      LLVMStructType(struct_member_types, len, 0);

  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx->env, module);

  LLVMValueRef func = LLVMAddFunction(module, "struct_of_coroutines_fn",
                                      cor_coroutine_fn_type());
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");

  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);
  LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);
  LLVMValueRef state_ptr = LLVMBuildLoad2(
      builder, 
      LLVMPointerType(llvm_state_struct_type, 0),
      state_gep, 
      "load_state_ptr"
  );
  LLVMValueRef state_struct = LLVMBuildLoad2(
      builder,
      llvm_state_struct_type,
      state_ptr,
      "struct_of_coroutines_state"
  );


  LLVMValueRef coroutine_not_complete = _TRUE;

  for (int i = 0; i < len; i++) {
    LLVMValueRef ret_val_gep = LLVMBuildGEP2(builder, llvm_ret_type, ret_val_ref, (LLVMValueRef[]){
      LLVMConstInt(LLVMInt32Type(), 0, 0),
      LLVMConstInt(LLVMInt32Type(), i, 0),
    }, 2, "");

    LLVMValueRef item = LLVMBuildExtractValue(builder, state_struct, i, "extract_struct_of_coroutines_member");
    if (is_coroutine_type(ast->data.AST_LIST.items[i].md)) {
      LLVMValueRef item_result = _cor_next(item, ret_val_gep, module, builder);
      LLVMValueRef is_not_null = LLVMBuildICmp(builder, LLVMIntNE, instance_ptr, null_cor_inst(),
                    "is_not_null");
      coroutine_not_complete = LLVMBuildAnd(builder, coroutine_not_complete, is_not_null, "");
    } else {
      LLVMBuildStore(builder, item, ret_val_gep);
    }
  }

  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");

  LLVMBuildCondBr(builder, coroutine_not_complete, then_block, else_block);
  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMBuildRet(builder, instance_ptr);
  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMBuildRet(builder, null_cor_inst());
  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef outer_state_struct = LLVMGetUndef(llvm_state_struct_type);
  for (int i = 0; i < len; i++) {
    Ast *member_ast = ast->data.AST_LIST.items + i;
    LLVMValueRef item = codegen(member_ast, ctx, module, builder);
    outer_state_struct = LLVMBuildInsertValue(builder, outer_state_struct, item, i, "");
  }

  LLVMValueRef state_ptr_alloca =
      LLVMBuildAlloca(builder, llvm_state_struct_type, "");

  LLVMBuildStore(builder, outer_state_struct, state_ptr_alloca);

  LLVMValueRef instance_struct = create_cor_inst_struct(builder, func, state_ptr_alloca);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder,instance_struct, alloca);
  return alloca;
}
