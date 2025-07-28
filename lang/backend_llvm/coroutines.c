#include "backend_llvm/coroutines.h"
#include "adt.h"
#include "binding.h"
#include "function.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "types/common.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdlib.h>
#include <string.h>

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
  LLVMTypeRef types[] = {
      LLVMInt32Type(), // counter @ 0
      GENERIC_PTR,     // fn ptr @ 1
      GENERIC_PTR,     // next ptr
      GENERIC_PTR,     // void *argv - state
      LLVMInt8Type(),  // SIGNAL enum
  };
  LLVMTypeRef instance_struct_type = LLVMStructType(types, 5, 0);
  return instance_struct_type;
}

LLVMTypeRef create_coroutine_state_type(Type *constructor_type,
                                        Type *state_struct, Ast *ast,
                                        JITLangCtx *ctx, LLVMModuleRef module) {

  int outer_args_len = fn_type_args_len(constructor_type);

  int inner_args_len = ast->data.AST_LAMBDA.num_yield_boundary_crossers;

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID &&
      inner_args_len == 0) {
    *state_struct = t_void;
    return NULL;
  }

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    outer_args_len = 0;
  }

  int total_state_args = outer_args_len + inner_args_len;
  Type *state_arg_types[total_state_args];

  Type *f = constructor_type;
  LLVMTypeRef llvm_state_arg_types[total_state_args];

  for (int i = 0; i < outer_args_len; i++) {
    Type *arg_type = f->data.T_FN.from;
    state_arg_types[i] = arg_type;
    llvm_state_arg_types[i] = type_to_llvm_type(arg_type, ctx, module);
    if (arg_type->kind == T_FN) {
      llvm_state_arg_types[i] = GENERIC_PTR;
    }
    f = f->data.T_FN.to;
  }

  AstList *boundary_xs = ast->data.AST_LAMBDA.yield_boundary_crossers;
  // for (int i = inner_args_len - 1; i >= 0; i--) {
  for (int i = 0; i < inner_args_len; i++) {
    Type *t = boundary_xs->ast->md;
    state_arg_types[outer_args_len + i] = t;

    llvm_state_arg_types[outer_args_len + i] =
        type_to_llvm_type(t, ctx, module);

    if (t->kind == T_FN) {
      llvm_state_arg_types[i] = GENERIC_PTR;
    }
    boundary_xs = boundary_xs->next;
  }

  if (total_state_args == 1) {
    *state_struct = *state_arg_types[0];
    return llvm_state_arg_types[0];
  }

  LLVMTypeRef instance_state_struct_type =
      LLVMStructType(llvm_state_arg_types, total_state_args, 0);

  Type **arg_types = malloc(sizeof(Type *) * total_state_args);
  for (int i = 0; i < total_state_args; i++) {
    arg_types[i] = state_arg_types[i];
  }
  *state_struct = (Type){T_CONS,
                         {.T_CONS = {.name = "state_struct",
                                     .args = arg_types,
                                     .num_args = total_state_args}}};

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
                        LLVMConstInt(LLVMInt32Type(), 3, 0)  // Get state gep
                    },
                    2, "instance_state_gep");
  return element_ptr;
}

LLVMValueRef get_instance_state_element_gep(int index,
                                            LLVMValueRef instance_ptr,
                                            LLVMTypeRef state_struct_type,
                                            LLVMBuilderRef builder) {
  // First get the pointer to the state field (index 3) in the instance struct
  LLVMValueRef state_ptr =
      LLVMBuildGEP2(builder, cor_inst_struct_type(), instance_ptr,
                    (LLVMValueRef[]){
                        LLVMConstInt(LLVMInt32Type(), 0, 0), // Deref pointer
                        LLVMConstInt(LLVMInt32Type(), 3, 0)  // Get state field
                    },
                    2, "instance_state_ptr");

  // Now get a pointer to the specific element within the state struct
  LLVMValueRef element_ptr = LLVMBuildGEP2(
      builder, state_struct_type, state_ptr,
      (LLVMValueRef[]){
          LLVMConstInt(LLVMInt32Type(), 0, 0),    // Deref pointer
          LLVMConstInt(LLVMInt32Type(), index, 0) // Get specific element
      },
      2, "state_element_ptr");

  return element_ptr;
}
LLVMValueRef get_cor_next_fn(LLVMModuleRef module) {

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
  return cor_next_func;
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

LLVMValueRef _cor_finished(LLVMValueRef instance_ptr, LLVMValueRef ret_val_ref,
                           LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_finished_func = LLVMGetNamedFunction(module, "cor_finished");
  LLVMTypeRef inst_type = cor_inst_struct_type();

  LLVMTypeRef cor_fin_type = LLVMFunctionType(
      LLVMInt1Type(), (LLVMTypeRef[]){LLVMPointerType(inst_type, 0)}, 1, false);

  if (!cor_finished_func) {
    cor_finished_func = LLVMAddFunction(module, "cor_finished", cor_fin_type);
  }

  return LLVMBuildCall2(builder, cor_fin_type, cor_finished_func,
                        (LLVMValueRef[]){
                            instance_ptr,
                        },
                        1, "call_cor_finished");
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
          LLVMPointerType(inst_type, 0), // LLVM Implicitly converts struct args
                                         // to pointers when calling C funcs
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

LLVMValueRef _cor_replace(LLVMValueRef this, LLVMValueRef other,
                          LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef cor_replace_func = LLVMGetNamedFunction(module, "cor_replace");

  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef inst_ptr_type = LLVMPointerType(inst_type, 0);
  LLVMTypeRef cor_replace_type = LLVMFunctionType(
      inst_ptr_type, (LLVMTypeRef[]){inst_ptr_type, inst_ptr_type}, 2, false);

  if (!cor_replace_func) {
    cor_replace_func = LLVMAddFunction(module, "cor_replace", cor_replace_type);
  }

  return LLVMBuildCall2(builder, cor_replace_type, cor_replace_func,
                        (LLVMValueRef[]){this, other}, 2, "");
}

LLVMValueRef _cor_stop(LLVMValueRef this, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  LLVMValueRef cor_stop_func = LLVMGetNamedFunction(module, "cor_stop");

  LLVMTypeRef inst_type = cor_inst_struct_type();
  LLVMTypeRef inst_ptr_type = LLVMPointerType(inst_type, 0);
  LLVMTypeRef cor_stop_type =
      LLVMFunctionType(inst_ptr_type, (LLVMTypeRef[]){inst_ptr_type}, 1, false);

  if (!cor_stop_func) {
    cor_stop_func = LLVMAddFunction(module, "cor_stop", cor_stop_type);
  }

  return LLVMBuildCall2(builder, cor_stop_type, cor_stop_func,
                        (LLVMValueRef[]){
                            this,
                        },
                        1, "");
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

int get_inner_state_slot(Ast *ast, JITLangCtx *ctx) {

  if (!ctx->coro_ctx) {
    return -1;
  }
  CoroutineCtx *coro_ctx = ctx->coro_ctx;

  Type *constructor_type = coro_ctx->cons_type;

  int outer_args_len = fn_type_args_len(constructor_type);

  int inner_args_len = coro_ctx->num_yield_boundary_xs;

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID &&
      inner_args_len == 0) {
    return -1;
  }

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    outer_args_len = 0;
  }

  AstList *l = coro_ctx->yield_boundary_xs;
  int li = coro_ctx->num_yield_boundary_xs - 1;

  while (l) {
    if (CHARS_EQ(ast->data.AST_IDENTIFIER.value,
                 l->ast->data.AST_IDENTIFIER.value)) {

      return outer_args_len + li;
    }

    l = l->next;
    li--;
  }
  return -1;
}

void bind_coroutine_state_vars(Type *state_type, LLVMTypeRef llvm_state_type,
                               LLVMValueRef instance_ptr, Ast *coroutine_ast,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  if (!state_type) {
    return;
  }
  CoroutineCtx *coro_ctx = ctx->coro_ctx;

  // Get pointer to the state pointer field (field 3)
  LLVMValueRef state_gep = LLVMBuildStructGEP2(
      builder, cor_inst_struct_type(), instance_ptr, 3, "instance_state_gep");

  AstList *bxs = coro_ctx->yield_boundary_xs;
  int num_bxs = coro_ctx->num_yield_boundary_xs;
  int struct_offset = 0;
  if (num_bxs > 0) {
    for (struct_offset = 0; struct_offset < num_bxs; struct_offset++) {
      Type *target_type = state_type->data.T_CONS.args[struct_offset];
      Ast *param = bxs->ast;

      LLVMValueRef state_ptr = LLVMBuildLoad2(
          builder, LLVMPointerType(llvm_state_type, 0), state_gep, "state_ptr");

      LLVMValueRef state_el_gep =
          LLVMBuildStructGEP2(builder, llvm_state_type, state_ptr,
                              struct_offset, "implicit_coroutine_state_var");

      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, target_type, NULL,
                                  type_to_llvm_type(target_type, ctx, module));

      const char *chars = param->data.AST_IDENTIFIER.value;
      uint64_t id_hash = hash_string(chars, param->data.AST_IDENTIFIER.length);
      sym->storage = state_el_gep;
      ht_set_hash(ctx->frame->table, chars, id_hash, sym);

      bxs = bxs->next;
    }
  }

  AstList *args = coroutine_ast->data.AST_LAMBDA.params;
  int args_len = coroutine_ast->data.AST_LAMBDA.len;
  for (; struct_offset < num_bxs + args_len; struct_offset++) {

    Type *target_type = state_type->data.T_CONS.args[struct_offset];
    Ast *param = args->ast;

    LLVMValueRef state_ptr = LLVMBuildLoad2(
        builder, LLVMPointerType(llvm_state_type, 0), state_gep, "state_ptr");

    LLVMValueRef state_el_gep =
        LLVMBuildStructGEP2(builder, llvm_state_type, state_ptr, struct_offset,
                            "explicit_coroutine_state_var");

    LLVMValueRef val;

    if (target_type->kind == T_FN) {
      val = state_el_gep;
    } else {
      LLVMTypeRef val_type = type_to_llvm_type(target_type, ctx, module);
      val = LLVMBuildLoad2(builder, val_type, state_el_gep, "");
    }
    codegen_pattern_binding(param, val, target_type, ctx, module, builder);
    args = args->next;
  }
}

void add_recursive_coroutine_fn_ref(ObjString fn_name, LLVMValueRef func,
                                    Type *fn_type, Type *state_type,
                                    JITLangCtx *fn_ctx) {

  JITSymbol *sym = new_symbol(STYPE_COROUTINE_CONSTRUCTOR, fn_type, func, NULL);
  sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.state_type = state_type;
  sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.recursive_ref = true;

  // JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, func,
  // LLVMTypeOf(func)); sym->symbol_data.STYPE_FUNCTION.fn_type = fn_type;

  ht *scope = fn_ctx->frame->table;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

static LLVMValueRef compile_coroutine_fn(Type *constructor_type,
                                         Type *state_type, Ast *ast,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  CoroutineCtx coro_ctx = {
      .num_yield_boundary_xs = ast->data.AST_LAMBDA.num_yield_boundary_crossers,
      .yield_boundary_xs = ast->data.AST_LAMBDA.yield_boundary_crossers,
      .num_coroutine_yields = ast->data.AST_LAMBDA.num_yields,
      .current_yield = 0,
      .cons_type = constructor_type,
  };

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
  fn_ctx.coro_ctx = &coro_ctx;

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);

  if (!is_anon) {
    add_recursive_coroutine_fn_ref(fn_name, func, constructor_type, state_type,
                                   &fn_ctx);
  }

  LLVMTypeRef state_struct_type =
      state_type ? type_to_llvm_type(state_type, ctx, module) : NULL;

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  bind_coroutine_state_vars(state_type, state_struct_type, instance_ptr, ast,
                            &fn_ctx, module, builder);

  LLVMBasicBlockRef switch_default_block =
      LLVMAppendBasicBlock(func, "coroutine_iter_end");

  coro_ctx.switch_default = switch_default_block;

  LLVMPositionBuilderAtEnd(builder, switch_default_block);

  LLVMBuildRet(builder, null_cor_inst());
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef switch_ref = LLVMBuildSwitch(
      builder, counter, switch_default_block, ast->data.AST_LAMBDA.num_yields);

  coro_ctx.yield_switch_ref = switch_ref;
  fn_ctx.coro_ctx = &coro_ctx;

  LLVMBasicBlockRef case_0 = LLVMAppendBasicBlock(func, "coroutine_iter_0");
  LLVMPositionBuilderAtEnd(builder, case_0);

  LLVMAddCase(switch_ref, LLVMConstInt(LLVMInt32Type(), 0, 0), case_0);
  LLVMPositionBuilderAtEnd(builder, case_0);

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);

  return func;
}

Type *build_coroutine_state_type(Type *constructor_type, Ast *ast) {

  int total_args_len = fn_type_args_len(constructor_type) - 1 +
                       ast->data.AST_LAMBDA.num_yield_boundary_crossers;

  int non_void_args = ast->data.AST_LAMBDA.num_yield_boundary_crossers;
  int t = total_args_len;
  Type *f = constructor_type;

  while (f->kind == T_FN) {
    if (f->data.T_FN.from->kind != T_VOID) {
      non_void_args++;
    }
    f = f->data.T_FN.to;
  }

  if (non_void_args == 0) {
    return NULL;
  }

  Type **types = talloc(sizeof(Type *) * non_void_args);
  int non_void_idx = 0;
  AstList *bxs = ast->data.AST_LAMBDA.yield_boundary_crossers;
  for (int i = 0; i < ast->data.AST_LAMBDA.num_yield_boundary_crossers; i++) {
    types[non_void_idx] = bxs->ast->md;

    bxs = bxs->next;
    non_void_idx++;
  }

  f = constructor_type;
  while (f->kind == T_FN && !f->data.T_FN.from->is_coroutine_instance) {
    Type *t = f->data.T_FN.from;
    if (t->kind != T_VOID) {
      types[non_void_idx] = t;
      non_void_idx++;
    }
    f = f->data.T_FN.to;
  }

  Type *struct_type = create_cons_type("state_struct", non_void_args, types);
  return struct_type;
}

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *fn_ast,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {
  Type *constructor_type = fn_ast->md;
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  if (is_generic(constructor_type)) {
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
  AstList *bxs = fn_ast->data.AST_LAMBDA.yield_boundary_crossers;
  while (bxs) {
    bxs = bxs->next;
  }

  Type *state_type = build_coroutine_state_type(constructor_type, fn_ast);
  LLVMValueRef constructor = compile_coroutine_fn(constructor_type, state_type,
                                                  fn_ast, ctx, module, builder);

  constructor_type->is_coroutine_constructor = true;

  JITSymbol *sym = new_symbol(STYPE_COROUTINE_CONSTRUCTOR, constructor_type,
                              constructor, NULL);
  sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.state_type = state_type;

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
  return cor_struct;
}

LLVMValueRef create_coroutine_state_ptr(Type *state_type, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  if (!state_type) {
    return NULL;
  }
  LLVMTypeRef type = type_to_llvm_type(state_type, ctx, module);
  LLVMValueRef state_ptr_alloca =
      LLVMBuildMalloc(builder, type, "coroutine_state_storage");
  return state_ptr_alloca;
}
void populate_state_struct_ptr(LLVMValueRef ptr, Type *state_type,
                               int ast_args_len, Ast *args, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  if (!state_type) {
    return;
  }
  if (!args || ast_args_len == 0) {
    return;
  }
  if (args->tag == AST_VOID) {
    return;
  }

  LLVMTypeRef llvm_state_type = type_to_llvm_type(state_type, ctx, module);

  int num_args = state_type->data.T_CONS.num_args;

  Type **state_args = state_type->data.T_CONS.args + (num_args - ast_args_len);

  for (int i = 0; i < ast_args_len; i++) {
    Type *state_arg_type = state_args[i];
    int struct_offset = (num_args - ast_args_len) + i;

    LLVMValueRef val = codegen(args + i, ctx, module, builder);

    LLVMValueRef gep =
        LLVMBuildStructGEP2(builder, llvm_state_type, ptr, struct_offset,
                            "set_explicit_coroutine_state_var");
    LLVMBuildStore(builder, val, gep);
  }
}

LLVMValueRef create_coroutine_instance_from_generic_constructor(
    JITSymbol *sym, Type *expected_type, Ast *args, int args_len,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef func = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_type);

  Type *state_type;
  if (!func) {

    JITLangCtx compilation_ctx = *ctx;

    Type *generic_type = sym->symbol_type;
    compilation_ctx.stack_ptr =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

    printf("generic coroutine\n");
    print_type(expected_type);
    print_type(sym->symbol_type);
    compilation_ctx.env = create_env_for_generic_fn(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env, generic_type,
        expected_type);

    Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    fn_ast.md = expected_type;

    AstList *bxs = fn_ast.data.AST_LAMBDA.yield_boundary_crossers;

    state_type = build_coroutine_state_type(expected_type, &fn_ast);

    LLVMValueRef specific_fn = compile_coroutine_fn(
        expected_type, state_type, &fn_ast, &compilation_ctx, module, builder);

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns = specific_fns_extend(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_type,
        specific_fn);
    func = specific_fn;
  }

  Type *constructor_type = expected_type;

  // TODO: need to save state_type for n > 1 coroutine instances from generics
  LLVMValueRef state_struct_ptr =
      create_coroutine_state_ptr(state_type, ctx, module, builder);

  populate_state_struct_ptr(state_struct_ptr, state_type, args_len, args, ctx,
                            module, builder);

  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, func, state_struct_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  // LLVMValueRef alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, cor_struct_type,
  //         "cor_instance_alloca") : LLVMBuildMalloc(builder,
  //         cor_struct_type, "cor_instance_malloc");

  LLVMValueRef alloca =
      ctx->stack_ptr > 0
          ? LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_alloca")
          : LLVMBuildMalloc(builder, cor_struct_type, "cor_instance_malloc");

  LLVMBuildStore(builder, cor_struct, alloca);
  return alloca;
}

LLVMValueRef create_coroutine_instance_from_constructor(
    JITSymbol *sym, Ast *args, int args_len, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *constructor_type = sym->symbol_type;

  LLVMValueRef state_struct_ptr = create_coroutine_state_ptr(
      sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.state_type, ctx, module,
      builder);

  populate_state_struct_ptr(
      state_struct_ptr, sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.state_type,
      args_len, args, ctx, module, builder);

  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, sym->val, state_struct_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  // LLVMValueRef alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, cor_struct_type,
  //         "cor_instance_alloca") : _cor_alloc(module, builder);
  //
  LLVMValueRef alloca = ctx->stack_ptr > 0 ? _cor_alloc(module, builder)
                                           : _cor_alloc(module, builder);

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
  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx, module);
  LLVMValueRef instance_ptr = sym->val;

  LLVMValueRef ret_val_ref =
      LLVMBuildAlloca(builder, llvm_ret_type, "ret_val_ref");

  instance_ptr = _cor_next(instance_ptr, ret_val_ref, module, builder);

  return wrap_yield_result_in_option(instance_ptr, ret_val_ref, llvm_ret_type,
                                     builder);
}

static LLVMValueRef codegen_yield_nested_coroutine(
    JITSymbol *sym, bool is_recursive_ref, LLVMValueRef instance_ptr,
    Type *state_type, Ast *args, int args_len, LLVMValueRef ret_val_ref,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef llvm_state_type = type_to_llvm_type(state_type, ctx, module);

  LLVMValueRef new_state_ptr = NULL;

  if (state_type) {
    if (is_recursive_ref) {
      new_state_ptr = get_instance_state_gep(instance_ptr, builder);
      new_state_ptr = LLVMBuildLoad2(
          builder, LLVMPointerType(llvm_state_type, 0), new_state_ptr, "");
    } else {
      new_state_ptr = LLVMBuildMalloc(builder, llvm_state_type, "");
    }
    populate_state_struct_ptr(new_state_ptr, state_type, args_len, args, ctx,
                              module, builder);
  }

  Type *constructor_type = sym->symbol_type;

  LLVMValueRef cor_struct =
      create_cor_inst_struct(builder, sym->val, new_state_ptr);

  LLVMTypeRef cor_struct_type = cor_inst_struct_type();

  LLVMValueRef next_instance_ptr =
      LLVMBuildAlloca(builder, cor_struct_type, "cor_instance_alloca");

  LLVMBuildStore(builder, cor_struct, next_instance_ptr);
  // the following functions _cor_reset & _cor_defer are declared to take a
  // struct as a second arg, but we actually need to pass pointers to structs
  // as llvm compiles struct passing as pointers implicitly
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

JITSymbol *nested_coroutine_expr(Ast *ast, JITLangCtx *ctx) {

  if (ast->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

    const char *sym_name =
        ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    if (!sym) {
      fprintf(stderr, "Error callable symbol %s not found in scope\n",
              sym_name);
      return NULL;
    }
    if (sym->type == STYPE_GENERIC_FUNCTION &&
        sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
      return sym;
    }

    if (is_coroutine_constructor_type(sym->symbol_type)) {
      return sym;
    }
  }
  return NULL;
}

LLVMValueRef codegen_coroutine_short_circuit(JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  // Get the current coroutine function parameters
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_block);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  // Option 2: Return null directly to signal completion
  // This is probably what you want - when cor_next gets null back,
  // it knows the coroutine is finished
  LLVMBuildRet(builder, null_cor_inst());

  return NULL;
}

// if (is_yield_end(ast)) {
//   return codegen_coroutine_short_circuit(ctx, module, builder);
// }

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  CoroutineCtx *coro_ctx = ctx->coro_ctx;

  LLVMBasicBlockRef current_case_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_case_block);
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  coro_ctx->current_yield++;
  LLVMBasicBlockRef next_case_block;
  if (coro_ctx->current_yield < coro_ctx->num_coroutine_yields) {
    char branch_name[19];
    sprintf(branch_name, "coroutine_iter_%d", coro_ctx->current_yield);
    next_case_block = LLVMAppendBasicBlock(func, branch_name);

    LLVMValueRef switch_ref = coro_ctx->yield_switch_ref;
    LLVMAddCase(switch_ref,
                LLVMConstInt(LLVMInt32Type(), coro_ctx->current_yield, 0),
                next_case_block);
  }

  Ast *yield_expr = ast->data.AST_YIELD.expr;
  if (!yield_expr) {
    LLVMBuildRet(builder, null_cor_inst());
    return NULL;
  }

  JITSymbol *sym = nested_coroutine_expr(yield_expr, ctx);

  if (sym != NULL) {
    bool is_recursive_ref =
        sym->type == STYPE_COROUTINE_CONSTRUCTOR &&
        sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.recursive_ref;

    if (sym->type == STYPE_GENERIC_FUNCTION && sym->val == NULL) {
      if (sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
        LLVMValueRef new_instance_ptr =
            sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
                yield_expr, ctx, module, builder);

        new_instance_ptr = _cor_defer(instance_ptr, new_instance_ptr,
                                      ret_val_ref, module, builder);

        LLVMBuildRet(builder, new_instance_ptr);
      }
    } else if (sym->type == STYPE_COROUTINE_CONSTRUCTOR) {

      Type *symbol_type = sym->symbol_type;
      Type *state_type =
          sym->symbol_data.STYPE_COROUTINE_CONSTRUCTOR.state_type;

      LLVMValueRef new_instance_ptr = codegen_yield_nested_coroutine(
          sym, is_recursive_ref, instance_ptr, state_type,
          yield_expr->data.AST_APPLICATION.args,
          yield_expr->data.AST_APPLICATION.len, ret_val_ref, ctx, module,
          builder);

      LLVMBuildRet(builder, new_instance_ptr);
    } else if (sym->type == STYPE_GENERIC_FUNCTION &&
               sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {
      // printf("make nested version of iter\n");
      return NULL;
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
