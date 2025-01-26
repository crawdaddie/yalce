#include "backend_llvm/coroutines.h"
#include "match.h"
#include "serde.h"
#include "types.h"
#include "llvm-c/Core.h"
JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

// create an instance type for a coroutine constructor function
// instance type is a struct
// { i32 counter, T0 state_arg0, T1 state_arg1, ..., void *fn_ptr }
//
// for a function T0 -> T1 -> ... TN -> (coroutine of (T0 * T1 * ... TN), () ->
// Some of Ret_type)
LLVMTypeRef instance_type(Type **state_args_list, int state_args_list_len,
                          JITLangCtx *ctx, LLVMModuleRef module) {

  LLVMTypeRef
      types[state_args_list_len + 1 + 1]; // num state args + counter + fn
  //
  types[0] = LLVMInt32Type(); // counter @ 0
  types[1] = GENERIC_PTR;     // fn ptr @ 1

  for (int i = 0; i < state_args_list_len; i++) {
    Type *arg_type = state_args_list[i];
    types[i + 2] = type_to_llvm_type(arg_type, ctx->env, module);
  }

  LLVMTypeRef instance_struct_type =
      LLVMStructType(types, state_args_list_len + 2, 0);

  return instance_struct_type;
}

LLVMValueRef get_instance_counter_gep(LLVMValueRef instance_ptr,
                                      LLVMBuilderRef builder) {
  LLVMTypeRef reduced_instance_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),
          GENERIC_PTR,
      },
      2, 0);
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, reduced_instance_type, instance_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 0, 0)  // Get counter
                    },
                    2, "instance_counter_gep");
  return element_ptr;
}

LLVMValueRef get_instance_fn_gep(LLVMValueRef instance_ptr,
                                 LLVMBuilderRef builder) {
  LLVMTypeRef reduced_instance_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),
          GENERIC_PTR,
      },
      2, 0);
  LLVMValueRef element_ptr =
      LLVMBuildGEP2(builder, reduced_instance_type, instance_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 1, 0)  // Get fn ptr
                    },
                    2, "instance_fn_gep");
  return element_ptr;
}

LLVMValueRef get_instance_state_arg_gep(LLVMValueRef instance_ptr,
                                        LLVMTypeRef instance_struct_type,
                                        int idx, LLVMBuilderRef builder) {

  LLVMValueRef element_ptr = LLVMBuildGEP2(
      builder, instance_struct_type, instance_ptr,
      (LLVMValueRef[]){
          LLVMConstInt(LLVMInt32Type(), 0, 0),      // Deref pointer
          LLVMConstInt(LLVMInt32Type(), idx + 2, 0) // Get idx-th state arg
      },
      2, "instance_state_arg_gep");
  return element_ptr;
}

LLVMTypeRef coroutine_fn_type(LLVMTypeRef instance_struct_type,
                              LLVMTypeRef return_option_type) {
  return LLVMFunctionType(
      return_option_type,
      (LLVMTypeRef[]){LLVMPointerType(instance_struct_type, 0)}, 1, 0);
}

LLVMValueRef compile_coroutine_fn(Type *constructor_type, Ast *ast,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  int args_len = fn_type_args_len(constructor_type);
  Type *state_arg_types[args_len]; // num state args + counter + fn

  if (args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    args_len = 0;
  }

  Type *f = constructor_type;
  for (int i = 0; i < args_len; i++) {
    Type *arg_type = f->data.T_FN.from;
    state_arg_types[i] = type_to_llvm_type(arg_type, ctx->env, module);
    f = f->data.T_FN.to;
  }
  LLVMTypeRef instance_struct_type =
      instance_type(state_arg_types, args_len, ctx, module);

  Type *ret_opt_type =
      fn_return_type(fn_return_type(constructor_type)->data.T_CONS.args[1]);

  LLVMTypeRef llvm_coroutine_fn_type = coroutine_fn_type(
      instance_struct_type, type_to_llvm_type(ret_opt_type, ctx->env, module));

  printf("\ncoroutine compilation\n"
         "-----------------------\n");
  LLVMDumpType(instance_struct_type);
  printf("\n");

  LLVMDumpType(llvm_coroutine_fn_type);
  printf("\n");

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;
  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  LLVMValueRef func =
      LLVMAddFunction(module, is_anon ? "anon_coroutine_func" : fn_name.chars,
                      llvm_coroutine_fn_type);
  if (func == NULL) {
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");

  LLVMValueRef instance_fn_ptr = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_fn_gep(instance_ptr, builder),
      "load_instance_fn_ptr");

  if (args_len > 0) {
    for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      LLVMValueRef param_gep = get_instance_state_arg_gep(
          instance_ptr, instance_struct_type, i, builder);

      LLVMValueRef param_val = LLVMBuildLoad2(
          builder, type_to_llvm_type(state_arg_types[i], fn_ctx.env, module),
          param_gep, "get_state_arg");

      codegen_pattern_binding(param_ast, param_val, state_arg_types[i], &fn_ctx,
                              module, builder);
    }
  }

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);
  LLVMDumpValue(func);
  return func;
}

// take a function which contains yields, and compile a coroutine instance
// constructor for it
//
// - bind it in the env to the symbol binding - the symbol will be either
// STYPE_GENERIC_FUNCTION or STYPE_FUNCTION
LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *fn_ast,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {
  Type *constructor_type = fn_ast->md;
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  printf("coroutine constructor binding\n");
  print_type(constructor_type);
  print_ast(fn_ast);

  if (is_generic(constructor_type)) {
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, constructor_type, NULL, NULL);
    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                sym);
    return NULL;
  }

  LLVMValueRef constructor =
      compile_coroutine_fn(constructor_type, fn_ast, ctx, module, builder);

  LLVMTypeRef llvm_constructor_type = NULL;

  Type *ret_opt_type =
      fn_return_type(fn_return_type(constructor_type)->data.T_CONS.args[1]);

  int args_len = fn_type_args_len(constructor_type);
  if (args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    args_len = 0;
  }

  Type *state_arg_types[args_len]; // num state args + counter + fn

  Type *f = constructor_type;
  for (int i = 0; i < args_len; i++) {
    Type *arg_type = f->data.T_FN.from;
    state_arg_types[i] = arg_type;
    f = f->data.T_FN.to;
  }

  LLVMTypeRef instance_struct_type =
      instance_type(state_arg_types, args_len, ctx, module);

  LLVMTypeRef llvm_coroutine_fn_type = coroutine_fn_type(
      instance_struct_type, type_to_llvm_type(ret_opt_type, ctx->env, module));

  LLVMDumpType(llvm_coroutine_fn_type);
  printf("\n");

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, constructor_type, constructor,
                              llvm_constructor_type);
  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);
  return NULL;
}

LLVMValueRef create_coroutine_instance_from_constructor(
    JITSymbol *sym, Ast *args, int args_len, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  printf("create coroutine instance from: ");
  print_type(sym->symbol_type);

  // printf("args: ");
  // for (int i = 0; i < args_len; i++) {
  //   print_ast(args + i);
  // }

  if (sym->type == STYPE_GENERIC_FUNCTION) {
  }

  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}

LLVMValueRef yield_coroutine_instance(JITSymbol *sym, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  printf("yield from coroutine instance: ");
  print_type(sym->symbol_type);
  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}
