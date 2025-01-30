#include "backend_llvm/coroutines.h"
#include "adt.h"
#include "function.h"
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
  //   void *argv[];
  // } cor;
  LLVMTypeRef types[4] = {
      LLVMInt32Type(), // counter @ 0
      GENERIC_PTR,     // fn ptr @ 1
      GENERIC_PTR,     // next ptr
      GENERIC_PTR,     // void *argv[] - state
  };
  LLVMTypeRef instance_struct_type = LLVMStructType(types, 4, 0);
  return instance_struct_type;
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
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0),
                      LLVMPointerType(inst_type, 0), // LLVM Implicitly converts struct args to pointers 
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
      (LLVMTypeRef[]){LLVMPointerType(inst_type, 0),
                      LLVMPointerType(inst_type, 0), // LLVM Implicitly converts struct args to pointers 
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

  Type *ret_opt_type =
      fn_return_type(fn_return_type(constructor_type)->data.T_CONS.args[1]);
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

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

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
    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                sym);
    return NULL;
  }

  printf("create coroutine cons binding\n");
  print_ast(fn_ast);
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
      builder, cor_struct, state ? state : LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)), 3,
      "insert_state_ptr");
  // LLVMConstStruct();
  return cor_struct;
}
LLVMTypeRef create_coroutine_state_type(Type *constructor_type, JITLangCtx *ctx, LLVMModuleRef module) {

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
LLVMValueRef create_coroutine_state_ptr(Type *constructor_type, Ast *args, JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {

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
    LLVMValueRef state_ptr_alloca = LLVMBuildAlloca(builder, llvm_state_arg_types[0], "");
    LLVMBuildStore(builder, codegen(args, ctx, module, builder), state_ptr_alloca);
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

LLVMValueRef create_coroutine_instance_from_constructor(
    JITSymbol *sym, Ast *args, int args_len, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return NULL;
  }

  printf("create coroutine instance from cons\n");
  print_ast(args);

  Type *constructor_type = sym->symbol_type;

  LLVMValueRef state_struct_ptr = create_coroutine_state_ptr(constructor_type, args, ctx, module, builder);
  if (state_struct_ptr) {
    LLVMDumpValue(state_struct_ptr);
  printf("\n");
  }

  // Create and initialize the cor struct
  LLVMValueRef cor_struct = create_cor_inst_struct(builder, sym->val, state_struct_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();
  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca")
          : LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef yield_from_coroutine_instance(JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  Type *coroutine_type = sym->symbol_type;

  int args_len =
      coroutine_type->data.T_CONS.args[0] == &t_void
          ? 0
          : coroutine_type->data.T_CONS.args[0]->data.T_CONS.num_args;
  Type **state_arg_types =
      args_len == 0 ? NULL
                    : coroutine_type->data.T_CONS.args[0]->data.T_CONS.args;

  Type *ret_opt_type = fn_return_type(coroutine_type->data.T_CONS.args[1]);
  Type *ret_type = type_of_option(ret_opt_type);
  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx->env, module);
  LLVMValueRef instance_ptr = sym->val;
  LLVMValueRef ret_val_ref =
      LLVMBuildAlloca(builder, llvm_ret_type, "ret_val_ref");

  instance_ptr = _cor_next(instance_ptr, ret_val_ref, module, builder);

  LLVMValueRef phi = LLVM_IF_ELSE(
      builder,
      (LLVMBuildICmp(builder, LLVMIntEQ, instance_ptr, null_cor_inst(),
                     "is_null")),

      (codegen_none_typed(builder, llvm_ret_type)),
      (codegen_some(LLVMBuildLoad2(builder, llvm_ret_type, ret_val_ref,
                                   "load_from_ret_val_ref"),
                    builder)));

  return phi;
}

LLVMValueRef
codegen_yield_nested_coroutine(JITSymbol *sym, LLVMValueRef instance_ptr,
                               LLVMValueRef ret_val_ref, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *constructor_type = sym->symbol_type;
  LLVMValueRef cor_struct = create_cor_inst_struct(builder, sym->val, NULL);
  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  LLVMValueRef alloca =
      LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca");
  LLVMBuildStore(builder, cor_struct, alloca);
  // the following functions _cor_reset & _cor_defer are declared to take a struct as a second arg, but we 
  // actually need to pass pointers to structs as llvm compiles struct passing as pointers implicitly

  if (sym->symbol_data.STYPE_FUNCTION.recursive_ref == true) {
    LLVMValueRef reset_res =
        _cor_reset(instance_ptr, alloca, ret_val_ref, module, builder);
    LLVMBuildRet(builder, reset_res);
  } else {
    LLVMValueRef defer_res =
        _cor_defer(instance_ptr, alloca, ret_val_ref, module, builder);
    LLVMBuildRet(builder, defer_res);
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

    Type *symbol_type = sym->symbol_type;

    if (is_coroutine_constructor_type(symbol_type)) {
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
    Type *symbol_type = sym->symbol_type;
    codegen_yield_nested_coroutine(sym, instance_ptr, ret_val_ref, ctx, module,
                                   builder);
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
