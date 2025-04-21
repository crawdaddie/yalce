#include "backend_llvm/coroutines.h"
#include "adt.h"
#include "array.h"
#include "function.h"
#include "list.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "types/common.h"
#include "util.h"
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
    llvm_state_arg_types[i] = type_to_llvm_type(arg_type, ctx->env, module);
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
        type_to_llvm_type(t, ctx->env, module);

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

static Ast *__current_coroutine_fn;

int get_inner_state_slot(Ast *ast) {
  if (!__current_coroutine_fn) {
    return -1;
  }
  Type *constructor_type = __current_coroutine_fn->md;

  int outer_args_len = fn_type_args_len(constructor_type);

  int inner_args_len =
      __current_coroutine_fn->data.AST_LAMBDA.num_yield_boundary_crossers;

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID &&
      inner_args_len == 0) {
    return -1;
  }

  if (outer_args_len == 1 && constructor_type->data.T_FN.from->kind == T_VOID) {
    outer_args_len = 0;
  }

  AstList *l = __current_coroutine_fn->data.AST_LAMBDA.yield_boundary_crossers;
  int li =
      __current_coroutine_fn->data.AST_LAMBDA.num_yield_boundary_crossers - 1;

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

LLVMValueRef get_inner_state_slot_gep(int slot, Ast *ast,
                                      LLVMBuilderRef builder) {

  // Create basic blocks for the if-then-else structure
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMValueRef instance_ptr = LLVMGetParam(function, 0);
  // LLVMValueRef state_gep =
  return NULL;
}

void bind_coroutine_state_vars(Type *state_type, LLVMTypeRef llvm_state_type,
                               LLVMValueRef instance_ptr, Ast *coroutine_ast,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  int fn_len = coroutine_ast->data.AST_LAMBDA.len;
  if (state_type->kind == T_VOID) {
    return;
  }
  if (state_type->kind != T_CONS) {
    if (fn_len > 1) {
      fprintf(stderr, "Error - inconsistent coroutine args");
      return;
    }
    Ast *param;
    if (fn_len == 1) {
      param = coroutine_ast->data.AST_LAMBDA.params;
    } else {
      param = coroutine_ast->data.AST_LAMBDA.yield_boundary_crossers->ast;
    }

    LLVMValueRef state_gep = get_instance_state_gep(instance_ptr, builder);

    state_gep = LLVMBuildLoad2(builder, LLVMPointerType(llvm_state_type, 0),
                               state_gep, "follow_gep_to_pointer");

    LLVMValueRef param_val = LLVMBuildLoad2(builder, llvm_state_type, state_gep,
                                            "get_single_cor_arg");
    codegen_pattern_binding(param, param_val, state_type, ctx, module, builder);

    return;
  }

  AstList *boundary_xs = coroutine_ast->data.AST_LAMBDA.yield_boundary_crossers;
  for (int i = 0; i < state_type->data.T_CONS.num_args; i++) {
    Type *t = state_type->data.T_CONS.args[i];
    Ast *param;
    if (i < fn_len) {
      param = coroutine_ast->data.AST_LAMBDA.params + i;
    } else {
      param = boundary_xs->ast;
      boundary_xs = boundary_xs->next;
    }

    LLVMValueRef state_gep = get_instance_state_element_gep(
        i, instance_ptr, llvm_state_type, builder);

    LLVMTypeRef llvm_type = type_to_llvm_type(t, ctx->env, module);
    if (i < fn_len) {

      // Get pointer to the state pointer field (field 3)
      LLVMValueRef state_ptr_ptr =
          LLVMBuildStructGEP2(builder, cor_inst_struct_type(), instance_ptr, 3,
                              "instance_state_gep");

      // Load the actual state pointer
      LLVMValueRef state_ptr =
          LLVMBuildLoad2(builder, LLVMPointerType(llvm_state_type, 0),
                         state_ptr_ptr, "state_ptr");

      // Now get pointer to the specific element within the state struct
      LLVMValueRef state_element_gep = LLVMBuildStructGEP2(
          builder, llvm_state_type, state_ptr, i, "state_element_ptr");

      LLVMValueRef val;

      // val = state_element_gep;
      if (t->kind == T_FN) {
        val = state_element_gep;
      } else {
        val = LLVMBuildLoad2(builder, llvm_type, state_element_gep, "");
      }

      codegen_pattern_binding(param, val, t, ctx, module, builder);

    } else {

      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, t, NULL, llvm_type);
      const char *chars = param->data.AST_IDENTIFIER.value;
      uint64_t id_hash = hash_string(chars, param->data.AST_IDENTIFIER.length);
      sym->storage = state_gep;
      ht_set_hash(ctx->frame->table, chars, id_hash, sym);
    }
  }
  return;
}

static LLVMValueRef compile_coroutine_fn(Type *constructor_type, Ast *ast,
                                         JITLangCtx *ctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  __current_coroutine_fn = ast;

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

  Type state_struct;
  LLVMTypeRef state_struct_type = create_coroutine_state_type(
      constructor_type, &state_struct, ast, ctx, module);

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);

  LLVMValueRef counter = LLVMBuildLoad2(
      builder, LLVMInt32Type(), get_instance_counter_gep(instance_ptr, builder),
      "load_instance_counter");
  LLVMValueRef ret_val_ref = LLVMGetParam(func, 1);

  bind_coroutine_state_vars(&state_struct, state_struct_type, instance_ptr, ast,
                            &fn_ctx, module, builder);

  LLVMBasicBlockRef switch_default_block =
      LLVMAppendBasicBlock(func, "coroutine_iter_end");
  LLVMPositionBuilderAtEnd(builder, switch_default_block);

  LLVMBuildRet(builder, null_cor_inst());
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef switch_ref = LLVMBuildSwitch(
      builder, counter, switch_default_block, ast->data.AST_LAMBDA.num_yields);
  fn_ctx.yield_switch_ref = switch_ref;
  LLVMBasicBlockRef case_0 = LLVMAppendBasicBlock(func, "coroutine_iter_0");
  LLVMPositionBuilderAtEnd(builder, case_0);

  LLVMAddCase(switch_ref, LLVMConstInt(LLVMInt32Type(), 0, 0), case_0);
  LLVMPositionBuilderAtEnd(builder, case_0);

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, block);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);

  __current_coroutine_fn = NULL;
  if (state_struct.kind == T_CONS) {
    free(state_struct.data.T_CONS.args);
  }
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
    if (arg_type->kind == T_FN) {
      llvm_state_arg_types[i] = GENERIC_PTR;
    }

    f = f->data.T_FN.to;
  }

  if (args_len == 1) {

    // LLVMValueRef state_ptr_alloca =
    //     LLVMBuildAlloca(builder, llvm_state_arg_types[0], "");
    // ATTENTION ?? need to create an arena for the coroutine, so when it
    // finishes state & instance can be freed together
    LLVMValueRef state_ptr_alloca =
        LLVMBuildMalloc(builder, llvm_state_arg_types[0], "");

    LLVMBuildStore(builder, codegen(args, ctx, module, builder),
                   state_ptr_alloca);
    return state_ptr_alloca;
  }

  LLVMTypeRef instance_state_struct_type =
      LLVMStructType(llvm_state_arg_types, args_len, 0);
  LLVMDumpType(instance_state_struct_type);

  LLVMValueRef inst_state_struct = LLVMGetUndef(instance_state_struct_type);

  for (int i = 0; i < args_len; i++) {
    Ast *arg_ast = args + i;
    LLVMValueRef state_arg_val = codegen(arg_ast, ctx, module, builder);
    if (state_arg_types[i]->kind == T_FN) {
      // Make sure it's stored as a pointer
      state_arg_val =
          LLVMBuildBitCast(builder, state_arg_val, GENERIC_PTR, "fn_ptr_cast");
    }

    inst_state_struct =
        LLVMBuildInsertValue(builder, inst_state_struct, state_arg_val, i,
                             "initial_instance_state_arg");
  }

  LLVMValueRef state_ptr_alloca =
      LLVMBuildMalloc(builder, instance_state_struct_type, "");
  printf("state ptr alloca b4 build store\n");

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

  LLVMValueRef state_struct_ptr =
      create_coroutine_state_ptr(constructor_type, args, ctx, module, builder);

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
  LLVMTypeRef llvm_ret_type = type_to_llvm_type(ret_type, ctx->env, module);
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

  LLVMTypeRef llvm_state_type = type_to_llvm_type(state_type, ctx->env, module);

  LLVMValueRef new_state_ptr;

  if (args_len == 1 && state_type->kind == T_VOID) {
    new_state_ptr = NULL;
  } else {
    if (is_recursive_ref == true) {
      new_state_ptr = get_instance_state_gep(instance_ptr, builder);
      new_state_ptr = LLVMBuildLoad2(
          builder, LLVMPointerType(llvm_state_type, 0), new_state_ptr, "");

    } else {
      new_state_ptr = LLVMBuildMalloc(builder, llvm_state_type, "");
    }

    if (args_len == 1) {

      LLVMValueRef yield_val = codegen(args, ctx, module, builder);
      if (!yield_val) {
        fprintf(stderr, "Error, could not yield nested coroutine %s:%d\n",
                __FILE__, __LINE__);
        return NULL;
      }

      LLVMBuildStore(builder, yield_val, new_state_ptr);

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
    bool is_recursive_ref = sym->type == STYPE_FUNCTION &&
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

  Ast map_func_arg = *ast->data.AST_APPLICATION.args;

  map_func_arg.md = map_func_type;

  LLVMValueRef map_func = codegen(&map_func_arg, ctx, module, builder);

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

  // LLVMValueRef alloca =
  //     ctx->stack_ptr > 0
  //         ? LLVMBuildAlloca(builder, cor_struct_type,
  //         "cor_instance_alloca") : _cor_alloc(module, builder);

  LLVMValueRef alloca = ctx->stack_ptr > 0 ? _cor_alloc(module, builder)
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

  LLVMTypeRef val_type = type_to_llvm_type(value_struct_type, ctx->env, module);

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
      LLVMValueRef val_from_callable = LLVMBuildCall2(
          builder, type_to_llvm_type(item_type, ctx->env, module),
          callable_item, NULL, 0, "call_void_item");
      LLVMBuildStore(builder, val_from_callable, val_gep);
    } else {
      LLVMBuildStore(builder, callable_item, val_gep);
    }
  }

  LLVMValueRef dur = codegen_tuple_access(0, val_ptr, val_type, builder);

  LLVMTypeRef llvm_effect_type =
      type_to_llvm_type(effect_fn_type, ctx->env, module);

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
      type_to_llvm_type(generator_type, ctx->env, module);

  LLVMValueRef generator_alloca =
      LLVMBuildMalloc(builder, llvm_generator_type, "");

  LLVMValueRef generator = codegen(generator_ast, ctx, module, builder);
  LLVMBuildStore(builder, generator, generator_alloca);

  LLVMValueRef effect_fn = codegen(effect_ast, ctx, module, builder);

  LLVMTypeRef llvm_scheduler_type =
      type_to_llvm_type(scheduler_type, ctx->env, module);

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
  LLVMTypeRef val_type = type_to_llvm_type(&t_num, ctx->env, module);
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
      type_to_llvm_type(generator_type, ctx->env, module);

  LLVMValueRef generator = codegen(generator_ast, ctx, module, builder);

  LLVMTypeRef llvm_scheduler_type =
      type_to_llvm_type(scheduler_type, ctx->env, module);

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
