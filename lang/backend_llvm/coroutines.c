#include "./coroutines.h"
#include "../types/builtins.h"
#include "./coroutines_private.h"
#include "adt.h"
#include "application.h"
#include "binding.h"
#include "function.h"
#include "symbols.h"
#include "types.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdlib.h>
#include <string.h>

static LLVMValueRef compile_coroutine_init(const char *name,
                                           LLVMValueRef coro_fn,
                                           LLVMValueRef func, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  LLVMTypeRef promise_type = coro_ctx->promise_type;

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, block);

  LLVMTypeRef cor_obj_type = coro_ctx->coro_obj_type;
  LLVMValueRef coro = LLVMBuildMalloc(builder, cor_obj_type, "coro_alloc");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
                 LLVMBuildStructGEP2(builder, cor_obj_type, coro,
                                     CORO_COUNTER_SLOT, "insert_coro_counter"));

  LLVMBuildStore(builder, coro_fn,
                 LLVMBuildStructGEP2(builder, cor_obj_type, coro,
                                     CORO_FN_PTR_SLOT, "insert_coro_fn_ptr"));

  LLVMValueRef promise_struct = LLVMGetUndef(promise_type);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(OPTION_TAG_TYPE, 1, 0), 0,
                                        "insert_promise_tag_none");
  LLVMBuildStore(builder, promise_struct,
                 coro_promise_gep(coro, cor_obj_type, builder));

  if (coro_ctx->state_layout) {
    LLVMValueRef state_storage =
        LLVMBuildMalloc(builder, coro_ctx->state_layout, "");

    int params = LLVMCountParams(func);
    for (int i = 0; i < params; i++) {
      LLVMValueRef param_gep = LLVMBuildStructGEP2(
          builder, coro_ctx->state_layout, state_storage, i, "");
      LLVMBuildStore(builder, LLVMGetParam(func, i), param_gep);
    }
    LLVMBuildStore(builder, state_storage,
                   coro_state_gep(coro, cor_obj_type, builder));
  }

  LLVMBuildRet(builder, coro);
  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}
static void add_recursive_coroutine_ref(Ast *ast, LLVMValueRef init_fn,
                                        LLVMTypeRef coro_init_type,
                                        JITLangCtx *fn_ctx) {
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  JITSymbol *sym =
      new_symbol(STYPE_FUNCTION, ast->type, init_fn, coro_init_type);
  sym->symbol_data.STYPE_FUNCTION.recursive_ref = true;

  ht *scope = fn_ctx->frame->table;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

void coro_terminate_block(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {

  coro_promise_set_none(coro, coro_ctx->coro_obj_type, coro_ctx->promise_type,
                        builder);
  coro_end_counter(coro, coro_ctx->coro_obj_type, builder);
}

LLVMValueRef coro_jump_to_next_block(LLVMValueRef coro, LLVMValueRef next_coro,
                                     CoroutineCtx *coro_ctx,
                                     LLVMBuilderRef builder) {

  LLVMValueRef c = coro_replace(coro, next_coro, coro_ctx, builder);
  coro_incr(coro, coro_ctx, builder);
  LLVMValueRef advanced_c = coro_advance(c, coro_ctx, builder);
  return advanced_c;
}

static LLVMValueRef coro_end_block(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  LLVMBasicBlockRef end_completely_block =
      LLVMAppendBasicBlock(coro_ctx->func, "end_coroutine_bb");
  LLVMBasicBlockRef proceed_to_chained_coro_block =
      LLVMAppendBasicBlock(coro_ctx->func, "defer_to_chained_coroutine_bb");

  LLVMPositionBuilderAtEnd(builder, coro_ctx->switch_default);
  LLVMValueRef next_coro = coro_next(coro, coro_ctx->coro_obj_type, builder);
  LLVMValueRef next_is_null =
      LLVMBuildIsNull(builder, next_coro, "is_next_null");
  LLVMBuildCondBr(builder, next_is_null, end_completely_block,
                  proceed_to_chained_coro_block);

  LLVMPositionBuilderAtEnd(builder, end_completely_block);
  coro_terminate_block(coro, coro_ctx, builder);
  LLVMBuildRet(builder, coro);

  LLVMPositionBuilderAtEnd(builder, proceed_to_chained_coro_block);
  LLVMValueRef n = coro_jump_to_next_block(coro, next_coro, coro_ctx, builder);
  LLVMBuildRet(builder, n);

  return NULL;
}
static LLVMTypeRef get_coro_init_type(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module) {
  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  Type *ftype = coro_ctx->cons_type;

  if (ast->data.AST_LAMBDA.len == 0 ||
      ast->data.AST_LAMBDA.len == 1 &&
          ast->data.AST_LAMBDA.params->ast->tag == AST_VOID) {
    LLVMTypeRef coro_init_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
    return coro_init_type;
  }

  LLVMTypeRef t[ast->data.AST_LAMBDA.len];
  int i = 0;
  for (AstList *argl = ast->data.AST_LAMBDA.params; argl != NULL;
       argl = argl->next) {
    Ast *arg = argl->ast;
    t[i] = type_to_llvm_type(ftype->data.T_FN.from, ctx, module);
    ftype = ftype->data.T_FN.to;
    i++;
  }

  LLVMTypeRef coro_init_type = LLVMFunctionType(GENERIC_PTR, t, i, 0);
  return coro_init_type;
}

// compile coroutine function ast (a function / lambda <cor_name> that contains
// yields) result is a helper function <cor_name>.init:
//   -- allocates a 'coro' object with
//   --- coroutine state (enough memory to hold the function's args, closed-over
//     values and any internal variables that cross a yield boundary (ie vars
//     that must be valid in more than one block))
//   --- a function pointer (<cor_name> function itself)
//   --- a state value that keeps track of where in the coroutine we are
//   --- a result option ptr
//
// AND the coroutine function itself <cor_name>
// which takes a 'coro' object arg and a result Option ptr
// - entry block,
// - yield blocks
//  -- in each yield block do some logic
//  -- advance the arg's position state
//  -- find the yield result
//  -- insert it into the result Option ptr
//  -- return the coro object
// - cleanup block

LLVMValueRef compile_coroutine(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *t = ast->type;
  Type *coro_cons_fn_type = t->data.T_CONS.args[0];
  Type *ret_inst_type = fn_return_type(coro_cons_fn_type);

  ret_inst_type = ret_inst_type->data.T_CONS.args[0];

  if (is_coroutine_type(ret_inst_type)) {
    ret_inst_type = ret_inst_type->data.T_CONS.args[0];
  }

  Type return_opt_type = TOPT(ret_inst_type);

  LLVMTypeRef promise_type = type_to_llvm_type(&return_opt_type, ctx, module);

  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

  CoroutineCtx coro_ctx = {
      .cons_type = coro_cons_fn_type,
      .coro_obj_type = coro_obj_type,
      .promise_type = promise_type,
      .current_yield = 0,
      .num_coroutine_yields = ast->data.AST_LAMBDA.num_yields,
      .num_yield_boundary_xs = ast->data.AST_LAMBDA.num_yield_boundary_crossers,
      .yield_boundary_xs = ast->data.AST_LAMBDA.yield_boundary_crossers,
      .name = ast->data.AST_LAMBDA.fn_name.chars};
  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)
  fn_ctx.coro_ctx = &coro_ctx;

  const char *name;
  int name_len;
  if (ast->data.AST_LAMBDA.fn_name.chars) {
    name = ast->data.AST_LAMBDA.fn_name.chars;
  } else {
    name = "anon_coro";
  }
  char *init_func_name = calloc(strlen(name) + 5 + 1, sizeof(char));
  sprintf(init_func_name, "%s.init", name);

  LLVMTypeRef coro_init_type = get_coro_init_type(ast, &fn_ctx, module);
  coro_ctx.state_layout = get_coro_state_layout(ast, &fn_ctx, module);

  LLVMValueRef init_fn =
      LLVMAddFunction(module, init_func_name, coro_init_type);
  LLVMSetLinkage(init_fn, LLVMExternalLinkage);

  LLVMTypeRef coro_type = PTR_ID_FUNC_TYPE(coro_obj_type);

  LLVMValueRef coro_fn = LLVMAddFunction(module, name, coro_type);
  coro_ctx.func = coro_fn;

  LLVMSetLinkage(coro_fn, LLVMExternalLinkage);
  LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(coro_fn, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry_block);
  LLVMValueRef coro = LLVMGetParam(coro_fn, 0);

  LLVMBasicBlockRef branches[coro_ctx.num_coroutine_yields + 1];
  for (int i = 0; i < coro_ctx.num_coroutine_yields + 1; i++) {
    char branch_name[19];
    if (i == coro_ctx.num_coroutine_yields) {
      sprintf(branch_name, "yield.default");
    } else {
      sprintf(branch_name, "yield.%d", i);
    }
    branches[i] = LLVMAppendBasicBlock(coro_fn, branch_name);
  }

  LLVMBasicBlockRef var_setup_block =
      LLVMAppendBasicBlock(coro_fn, "var_setup_block");
  LLVMBasicBlockRef switch_block =
      LLVMAppendBasicBlock(coro_fn, "switch_block");

  coro_ctx.branches = branches;
  coro_ctx.switch_default = branches[coro_ctx.num_coroutine_yields];

  LLVMValueRef is_finished = coro_is_finished(coro, &coro_ctx, builder);
  LLVMBuildCondBr(builder, is_finished, coro_ctx.switch_default,
                  var_setup_block);

  LLVMPositionBuilderAtEnd(builder, var_setup_block);

  if (coro_ctx.state_layout) {
    LLVMValueRef state = LLVMBuildBitCast(
        builder, coro_state(coro, coro_obj_type, builder),
        LLVMPointerType(coro_ctx.state_layout, 0), "bitcast_generic_state_ptr");
    int i = 0;
    Type *ftype = coro_ctx.cons_type;

    for (AstList *arglist = ast->data.AST_LAMBDA.params; arglist != NULL;
         arglist = arglist->next) {

      Ast *arg = arglist->ast;
      if (arg->tag == AST_VOID) {
        break;
      }
      LLVMValueRef state_storage =

          LLVMBuildStructGEP2(builder, coro_ctx.state_layout, state, i, "");

      LLVMValueRef state_val = LLVMBuildLoad2(
          builder, type_to_llvm_type(ftype->data.T_FN.from, &fn_ctx, module),
          state_storage, "");

      codegen_pattern_binding(arg, state_val, ftype->data.T_FN.from, ctx,
                              module, builder);
      i++;
      ftype = ftype->data.T_FN.to;
    }

    if (ast->data.AST_LAMBDA.num_yield_boundary_crossers > 0) {
      AstList *bxs = ast->data.AST_LAMBDA.yield_boundary_crossers;

      for (int j = 0; j < ast->data.AST_LAMBDA.num_yield_boundary_crossers;
           j++) {
        Ast *bx = bxs->ast;
        Type *bxt = bx->type;

        if (is_generic(bxt)) {
          bxt = resolve_type_in_env(bxt, ctx->env);
        }

        LLVMValueRef state_storage =
            LLVMBuildStructGEP2(builder, coro_ctx.state_layout, state, i, "");

        LLVMTypeRef item_type = type_to_llvm_type(bxt, &fn_ctx, module);
        // LLVMBuildStore(builder, LLVMConstNull(item_type), state_storage);

        JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, bxt, NULL, item_type);
        sym->storage = state_storage;
        const char *chars = bx->data.AST_IDENTIFIER.value;
        int chars_len = bx->data.AST_IDENTIFIER.length;
        ht_set_hash(fn_ctx.frame->table, chars, hash_string(chars, chars_len),
                    sym);

        bxs = bxs->next;
        i++;
      }
    }
  }

  if (ast->data.AST_LAMBDA.fn_name.chars) {
    add_recursive_coroutine_ref(ast, init_fn, coro_init_type, &fn_ctx);
  }

  LLVMBuildBr(builder, switch_block);

  LLVMPositionBuilderAtEnd(builder, switch_block);
  LLVMValueRef switch_ref =
      LLVMBuildSwitch(builder, coro_counter(coro, coro_obj_type, builder),
                      coro_ctx.switch_default, coro_ctx.num_coroutine_yields);

  for (int i = 0; i < coro_ctx.num_coroutine_yields; i++) {
    LLVMAddCase(switch_ref, LLVMConstInt(LLVMInt32Type(), i, 0), branches[i]);
  }
  coro_ctx.switch_ref = switch_ref;

  coro_end_block(coro, &coro_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, branches[0]);
  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);

  LLVMValueRef init_func =
      compile_coroutine_init(name, coro_fn, init_fn, &fn_ctx, module, builder);

  return init_func;
}

static LLVMValueRef coro_create_from_generic(JITSymbol *sym,
                                             Type *expected_fn_type, Ast *ast,
                                             JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  // TODO: figure out coroutines that accept other coroutines as args, eg:
  //
  // let cor_zip = fn c1 c2 ->
  //   let x1 = cor_unwrap_or_end @@ c1 ();
  //   let x2 = cor_unwrap_or_end @@ c2 ();
  //   yield (x1, x2);
  //   yield cor_zip c1 c2
  // ;;
  // let c1 = fn () -> yield 1; yield 2;;
  // let c2 = fn () -> yield 2; yield 1;;
  //
  // let x = cor_zip (c1 ()) (c2 ());
  //
  // -- or --
  //
  // let get_head_opt = fn x ->
  //   match x with
  //   | x::rest -> Some (x, rest)
  //   | [] -> None
  // ;;
  //
  // let seq = fn cors ->
  //   let (h, rest) = cor_unwrap_or_end @@ get_head_opt cors;
  //   yield h;
  //   yield combine rest
  // ;;
  //
  // let x = seq [iter_of_list [1,2,3], iter_of_list [3,2,1]];

  LLVMValueRef func = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type);

  if (!func) {

    JITLangCtx compilation_ctx = *ctx;

    Type *generic_type = sym->symbol_type;
    generic_type = generic_type->data.T_CONS.args[0];

    compilation_ctx.stack_ptr =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

    compilation_ctx.env = create_env_for_generic_fn(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env, generic_type,
        expected_fn_type);

    Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    Type exp = TCONS(TYPE_NAME_COROUTINE_CONSTRUCTOR, 1, expected_fn_type);
    fn_ast.type = &exp;

    LLVMValueRef specific_fn =
        compile_coroutine(&fn_ast, &compilation_ctx, module, builder);

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns = specific_fns_extend(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type,
        specific_fn);

    func = specific_fn;
  }

  return func;
}

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *ast,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMValueRef callable;
  if (sym->type == STYPE_GENERIC_FUNCTION) {
    callable = coro_create_from_generic(sym, expected_fn_type, ast, ctx, module,
                                        builder);
  } else {
    callable = sym->val;
  }

  // expected_fn_type = expected_fn_type->data.T_CONS.args[0];

  Type *ctype = deep_copy_type(expected_fn_type);

  Type *c = ctype;
  int i = 0;

  while (!is_coroutine_type(c->data.T_FN.to)) {
    c = c->data.T_FN.to;
    i++;
  }

  c->data.T_FN.to = &t_ptr;
  LLVMValueRef v = call_callable(ast, ctype, callable, ctx, module, builder);
  return v;
}

LLVMValueRef coro_fn_ptr(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                         LLVMBuilderRef builder) {

  LLVMValueRef fn_ptr_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                                CORO_FN_PTR_SLOT, "fn_ptr_gep");
  LLVMValueRef fn_ptr =
      LLVMBuildLoad2(builder, GENERIC_PTR, fn_ptr_gep, "fn_ptr");
  LLVMTypeRef fn_param_types[] = {LLVMPointerType(coro_obj_type, 0)};

  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMPointerType(coro_obj_type, 0), fn_param_types, 1, 0);

  LLVMValueRef typed_fn_ptr = LLVMBuildBitCast(
      builder, fn_ptr, LLVMPointerType(fn_type, 0), "typed_fn_ptr");

  return typed_fn_ptr;
}
LLVMValueRef coro_promise_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {

  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_PROMISE_SLOT,
                             "promise_gep");
}

LLVMValueRef coro_promise(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMTypeRef promise_type, LLVMBuilderRef builder) {

  LLVMValueRef promise_gep = coro_promise_gep(coro, coro_obj_type, builder);
  LLVMValueRef promise =
      LLVMBuildLoad2(builder, promise_type, promise_gep, "promise");
  return promise;
}

LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                        LLVMBuilderRef builder) {

  LLVMValueRef state_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                               CORO_STATE_SLOT, "state_gep");
  LLVMValueRef state = LLVMBuildLoad2(builder, GENERIC_PTR, state_gep, "state");
  return state;
}

LLVMValueRef coro_state_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                            LLVMBuilderRef builder) {

  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_STATE_SLOT,
                             "state_gep");
}

LLVMValueRef coro_promise_set(LLVMValueRef coro, LLVMValueRef val,
                              LLVMTypeRef coro_obj_type,
                              LLVMTypeRef promise_type,
                              LLVMBuilderRef builder) {

  LLVMValueRef promise_struct = LLVMGetUndef(promise_type);

  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(OPTION_TAG_TYPE, 0, 0), 0,
                                        "coro.promise.tag");

  promise_struct = LLVMBuildInsertValue(builder, promise_struct, val, 1,
                                        "coro.promise.value");

  LLVMValueRef promise_gep = coro_promise_gep(coro, coro_obj_type, builder);
  LLVMBuildStore(builder, promise_struct, promise_gep);
  return coro;
}

LLVMValueRef coro_promise_set_none(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                   LLVMTypeRef promise_type,
                                   LLVMBuilderRef builder) {

  LLVMValueRef promise_struct = LLVMGetUndef(promise_type);

  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(OPTION_TAG_TYPE, 1, 0), 0,
                                        "coro.promise.tag");

  LLVMValueRef promise_gep = coro_promise_gep(coro, coro_obj_type, builder);
  LLVMBuildStore(builder, promise_struct, promise_gep);
  return coro;
}

LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                           LLVMTypeRef coro_obj_type, LLVMBuilderRef builder) {
  LLVMValueRef next_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                              CORO_NEXT_SLOT, "coro.next.gep");
  LLVMBuildStore(builder, next, next_gep);
  return coro;
}

LLVMValueRef coro_stack_push(LLVMValueRef coro, LLVMValueRef next,
                             LLVMTypeRef coro_obj_type,
                             LLVMBuilderRef builder) {

  LLVMValueRef stack_head = coro_next(coro, coro_obj_type, builder);
  coro_next_set(next, stack_head, coro_obj_type, builder);

  LLVMValueRef next_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                              CORO_NEXT_SLOT, "coro.next.gep");
  LLVMBuildStore(builder, next, next_gep);
  return coro;
}

LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                       LLVMBuilderRef builder) {
  LLVMValueRef next_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                              CORO_NEXT_SLOT, "coro.next.gep");
  LLVMValueRef next = LLVMBuildLoad2(builder, LLVMPointerType(coro_obj_type, 0),
                                     next_gep, "coro.next");
  return next;
}

LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMBuilderRef builder) {

  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_obj_type, coro, CORO_COUNTER_SLOT, "counter_gep");
  LLVMValueRef counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "coro.counter");
  return counter;
}

LLVMValueRef coro_counter_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_COUNTER_SLOT,
                             "coro.counter.gep");
}

LLVMValueRef coro_end_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_obj_type, coro, CORO_COUNTER_SLOT, "coro.counter.gep");
  return LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), -1, 1),
                        counter_gep);
}

LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {
  LLVMValueRef fn_ptr = coro_fn_ptr(coro, coro_ctx->coro_obj_type, builder);

  LLVMValueRef args[] = {coro};
  LLVMValueRef new_cor = LLVMBuildCall2(
      builder, PTR_ID_FUNC_TYPE(coro_ctx->coro_obj_type), fn_ptr, args, 1, "");

  return new_cor;
}

LLVMValueRef coro_is_finished(LLVMValueRef coro, CoroutineCtx *ctx,
                              LLVMBuilderRef builder) {

  LLVMValueRef counter = coro_counter(coro, ctx->coro_obj_type, builder);
  LLVMValueRef is_finished =
      LLVMBuildICmp(builder, LLVMIntEQ, counter,
                    LLVMConstInt(LLVMInt32Type(), -1, 1), "coro.is_finished");
  return is_finished;
}

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMValueRef coro = sym->val;

  Type *coro_type = sym->symbol_type;
  Type *ret_inst_type = coro_type->data.T_CONS.args[0];

  // Unwrap nested coroutine types (Coroutine<Coroutine<T>> -> T)
  while (is_coroutine_type(ret_inst_type)) {
    ret_inst_type = ret_inst_type->data.T_CONS.args[0];
  }

  Type *ret_opt_type = create_option_type(ret_inst_type);

  LLVMTypeRef promise_type = type_to_llvm_type(ret_opt_type, ctx, module);
  CoroutineCtx tmp_ctx = {.promise_type = promise_type,
                          .coro_obj_type = CORO_OBJ_TYPE(promise_type)};

  LLVMValueRef _counter = coro_counter(coro, tmp_ctx.coro_obj_type, builder);
  LLVMValueRef is_finished = coro_is_finished(coro, &tmp_ctx, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_block);

  LLVMBasicBlockRef then_block =
      LLVMAppendBasicBlock(func, "coro.is_finished_block");
  LLVMBasicBlockRef else_block =
      LLVMAppendBasicBlock(func, "coro.resume_block");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "merge");

  LLVMBuildCondBr(builder, is_finished, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);

  LLVMValueRef none_promise = LLVMGetUndef(tmp_ctx.promise_type);
  none_promise = LLVMBuildInsertValue(builder, none_promise,
                                      LLVMConstInt(OPTION_TAG_TYPE, 1, 0), 0,
                                      "insert_none_tag");

  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef new_coro = coro_advance(coro, &tmp_ctx, builder);
  LLVMValueRef ret_promise = coro_promise(new_coro, tmp_ctx.coro_obj_type,
                                          tmp_ctx.promise_type, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef result_phi = LLVMBuildPhi(builder, promise_type, "result");

  LLVMAddIncoming(result_phi, (LLVMValueRef[]){none_promise, ret_promise},
                  (LLVMBasicBlockRef[]){then_end_block, else_end_block}, 2);

  return result_phi;
}

LLVMValueRef coro_incr(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                       LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      coro_counter_gep(coro, coro_ctx->coro_obj_type, builder);

  LLVMValueRef current_counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "coro.counter");

  LLVMValueRef is_finished = coro_is_finished(coro, coro_ctx, builder);

  LLVMValueRef incremented_counter = LLVMBuildSelect(
      builder, is_finished, LLVMConstInt(LLVMInt32Type(), -1, 1),
      LLVMBuildAdd(builder, current_counter,
                   LLVMConstInt(LLVMInt32Type(), 1, 1), "coro.counter.incr"),
      "");

  LLVMBuildStore(builder, incremented_counter, counter_gep);
  return NULL;
}

LLVMValueRef recursive_coro_yield(LLVMValueRef coro, Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  LLVMValueRef func = coro_ctx->func;
  LLVMValueRef counter_gep =
      LLVMBuildStructGEP2(builder, coro_ctx->coro_obj_type, coro,
                          CORO_COUNTER_SLOT, "coro.counter.gep");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);
  if (coro_ctx->state_layout && ast->data.AST_APPLICATION.len > 0) {
    LLVMValueRef state = coro_state(coro, coro_ctx->coro_obj_type, builder);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Ast *arg = ast->data.AST_APPLICATION.args + i;
      LLVMValueRef v = codegen(arg, ctx, module, builder);
      LLVMBuildStore(
          builder, v,
          LLVMBuildStructGEP2(builder, coro_ctx->state_layout, state, i, ""));
    }
  }

  LLVMBuildRet(
      builder,
      LLVMBuildCall2(builder, PTR_ID_FUNC_TYPE(coro_ctx->coro_obj_type), func,
                     (LLVMValueRef[]){coro}, 1, "coro.recursive_call"));

  return NULL;
}

LLVMValueRef coro_replace(LLVMValueRef dest, LLVMValueRef src,
                          CoroutineCtx *coro_ctx, LLVMBuilderRef builder) {

  LLVMValueRef struct_size = LLVMSizeOf(coro_ctx->coro_obj_type);

  LLVMBuildMemCpy(builder, dest, 8, src, 8, struct_size);

  return dest;
}

LLVMValueRef tail_call_coro_yield(LLVMValueRef coro, LLVMValueRef new_cor,
                                  CoroutineCtx *coro_ctx,
                                  LLVMBuilderRef builder) {

  LLVMValueRef struct_size = LLVMSizeOf(coro_ctx->coro_obj_type);

  LLVMBuildFree(builder, coro_state(coro, coro_ctx->coro_obj_type, builder));

  LLVMBuildMemCpy(builder, coro, LLVMGetAlignment(coro), new_cor,
                  LLVMGetAlignment(new_cor), struct_size);
  LLVMValueRef fn_ptr = coro_fn_ptr(coro, coro_ctx->coro_obj_type, builder);

  coro = LLVMBuildCall2(builder, PTR_ID_FUNC_TYPE(coro_ctx->coro_obj_type),
                        fn_ptr, (LLVMValueRef[]){coro}, 1, "nested resume");

  LLVMBuildFree(builder, new_cor);
  LLVMBuildRet(builder, coro);
  return NULL;
}

typedef struct {
  int32_t counter;
  void *fn_ptr;
  void *state;
  void *next;
} coro_simple;

void *coro_next_tail(void *_cor) {
  if (_cor == NULL) {
    return _cor;
  }
  coro_simple *c = _cor;
  while (c->next != NULL) {
    c = c->next;
  }
  return c;
}

// handle situations like the following:
// arbitrarily nested coroutines
//
//
//
// let cor1 = fn () ->
//   yield 1
// ;;
//
// let cor2 = fn () -> yield cor1 (); yield 2 ;;
//
// let cor3 = fn () -> yield cor2 (); yield 3 ;;
//
// let cor = fn () -> yield cor3 (); yield 4 ;;
//
// let x = cor ();
//
//
// expected yielded values:
// 1 -> 2 -> 3 -> 4
LLVMValueRef chain_new_coro_yield(LLVMValueRef coro, LLVMValueRef new_cor,
                                  CoroutineCtx *coro_ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  // let B = fn () ->
  //   yield val_b_1
  // ;;
  //
  // let A = fn () ->
  //    yield B ();
  //    yield val_a_1;
  //    yield val_a_2;
  // ;
  //

  LLVMValueRef A = LLVMBuildMalloc(builder, coro_ctx->coro_obj_type,
                                   "copy_calling_coroutine");

  coro_replace(A, coro, coro_ctx, builder);

  LLVMValueRef B = coro_advance(new_cor, coro_ctx, builder);

  coro_replace(coro, B, coro_ctx, builder);

  LLVMTypeRef get_tail_fn_type =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);

  LLVMValueRef get_tail_fn = get_extern_fn(
      "coro_next_tail",
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0),
      module);
  LLVMValueRef Btail = LLVMBuildCall2(builder, get_tail_fn_type, get_tail_fn,
                                      (LLVMValueRef[]){coro}, 1, "");

  coro_next_set(Btail, A, coro_ctx->coro_obj_type, builder);

  LLVMBuildRet(builder, coro);
  return NULL;
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  if (!coro_ctx) {
    fprintf(stderr, "Error - yield must only be used in a coroutine context\n");
    return NULL;
  }
  LLVMValueRef coro = LLVMGetParam(coro_ctx->func, 0);

  Ast *expr = ast->data.AST_YIELD.expr;
  Type *expr_type = resolve_type_in_env(deep_copy_type(expr->type), ctx->env);

  if (ast->data.AST_YIELD.expr->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    // RECURSIVE COROUTINE YIELD - REPLACE COROUTINE WITH ITSELF
    if (sym && is_coroutine_constructor_type(sym->symbol_type) &&
        sym->symbol_data.STYPE_FUNCTION.recursive_ref) {
      return recursive_coro_yield(coro, expr, ctx, module, builder);
    }

    if (is_coroutine_type(expr_type)) {
      // COROUTINE DEFERS TO A NEW ONE AND CAN FORGET ABOUT ITSELF SINCE YIELD
      // IS IN TAIL POSITION
      if (coro_ctx->current_yield == coro_ctx->num_coroutine_yields - 1) {
        LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
        return tail_call_coro_yield(coro, new_cor, coro_ctx, builder);
      }
      // COROUTINE DEFERS TO A NEW ONE BUT MUST RETURN BACK TO ITSELF
      LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
      LLVMValueRef n =
          chain_new_coro_yield(coro, new_cor, coro_ctx, module, builder);
      coro_ctx->current_yield++;
      LLVMPositionBuilderAtEnd(builder,
                               coro_ctx->branches[coro_ctx->current_yield]);
      return n;
    }
  }

  if (is_coroutine_type(expr_type)) {
    if (coro_ctx->current_yield == coro_ctx->num_coroutine_yields - 1) {
      LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
      return tail_call_coro_yield(coro, new_cor, coro_ctx, builder);
    }

    // COROUTINE DEFERS TO A NEW ONE BUT MUST RETURN BACK TO ITSELF
    //
    LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
    LLVMValueRef n =
        chain_new_coro_yield(coro, new_cor, coro_ctx, module, builder);

    coro_ctx->current_yield++;
    LLVMPositionBuilderAtEnd(builder,
                             coro_ctx->branches[coro_ctx->current_yield]);
    return n;
  }

  int branch_idx = coro_ctx->current_yield;

  LLVMValueRef yield_val = codegen(expr, ctx, module, builder);

  coro_incr(coro, coro_ctx, builder);
  coro_promise_set(coro, yield_val, coro_ctx->coro_obj_type,
                   coro_ctx->promise_type, builder);

  LLVMBuildRet(builder, coro);
  coro_ctx->current_yield++;
  LLVMPositionBuilderAtEnd(builder,
                           coro_ctx->branches[coro_ctx->current_yield]);
  return yield_val;
}

LLVMTypeRef get_coro_state_layout(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module) {
  int state_len = 0;
  int args_len = ast->data.AST_LAMBDA.len;
  if (ast->data.AST_LAMBDA.len == 0 ||
      (ast->data.AST_LAMBDA.len == 1 &&
       ast->data.AST_LAMBDA.params->ast->tag == AST_VOID)) {
    state_len = 0;
    args_len = 0;
  }

  state_len += args_len;
  state_len += ast->data.AST_LAMBDA.num_yield_boundary_crossers;

  if (state_len == 0) {
    return NULL;
  }

  LLVMTypeRef t[state_len];

  if (args_len > 0) {

    Type *coro_cons_type = ast->type;
    Type *ftype = coro_cons_type->data.T_CONS.args[0];

    for (int i = 0; i < args_len; i++) {
      Type *from = ftype->data.T_FN.from;
      t[i] = type_to_llvm_type(ftype->data.T_FN.from, ctx, module);
      // printf("%d: ", i);
      // print_type(ftype->data.T_FN.from);
      // LLVMDumpType(t[i]);
      // printf("\n");
      ftype = ftype->data.T_FN.to;
    }
  }

  if (ast->data.AST_LAMBDA.num_yield_boundary_crossers > 0) {
    AstList *bxs = ast->data.AST_LAMBDA.yield_boundary_crossers;
    for (int j = args_len; j < state_len; j++) {
      Ast *bx = bxs->ast;

      // printf("boundary crosser\n");
      // print_ast(bx);

      Type *bxt = bx->type;
      if (is_generic(bxt)) {
        bxt = resolve_type_in_env(bxt, ctx->env);
      }

      t[j] = type_to_llvm_type(bxt, ctx, module);
      bxs = bxs->next;
    }
  }

  LLVMTypeRef state_layout = LLVMStructType(t, state_len, 0);
  return state_layout;
}
