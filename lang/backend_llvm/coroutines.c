#include "./coroutines.h"
#include "function.h"
#include "symbols.h"
#include "types.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  Type *cons_type;
  LLVMTypeRef coro_obj_type;
  LLVMTypeRef promise_type;
  int num_coroutine_yields;
  int current_yield;
  AstList *yield_boundary_xs;
  int num_yield_boundary_xs;
  LLVMBasicBlockRef *branches;
  LLVMBasicBlockRef switch_default;
  LLVMValueRef switch_ref;
  LLVMValueRef func;
} CoroutineCtx;

#define PTR_ID_FUNC_TYPE(obj)                                                  \
  LLVMFunctionType(LLVMPointerType(obj, 0),                                    \
                   (LLVMTypeRef[]){LLVMPointerType(obj, 0)}, 1, 0)

static LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                 LLVMBuilderRef builder);

static LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                               LLVMBuilderRef builder);

static LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                                  LLVMTypeRef coro_obj_type,
                                  LLVMBuilderRef builder);

static LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder);

static LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                                 LLVMBuilderRef builder);

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
  LLVMValueRef alloc = LLVMBuildMalloc(builder, cor_obj_type, "coro_alloc");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0),
                 LLVMBuildStructGEP2(builder, cor_obj_type, alloc, 0,
                                     "insert_coro_counter"));

  LLVMBuildStore(builder, coro_fn,
                 LLVMBuildStructGEP2(builder, cor_obj_type, alloc, 1,
                                     "insert_coro_fn_ptr"));

  LLVMValueRef promise_struct = LLVMGetUndef(promise_type);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt32Type(), 1, 0), 0,
                                        "insert_promise_field_0");
  LLVMBuildStore(builder, promise_struct,
                 LLVMBuildStructGEP2(builder, cor_obj_type, alloc, 3,
                                     "insert_promise_struct"));

  LLVMBuildRet(builder, alloc);
  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}
static void add_recursive_coroutine_ref(Ast *ast, LLVMValueRef init_fn,
                                        LLVMTypeRef coro_init_type,
                                        JITLangCtx *fn_ctx) {
  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  JITSymbol *sym = new_symbol(STYPE_FUNCTION, ast->md, init_fn, coro_init_type);
  sym->symbol_data.STYPE_FUNCTION.fn_type = ast->md;
  sym->symbol_data.STYPE_FUNCTION.recursive_ref = true;

  ht *scope = fn_ctx->frame->table;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
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

  Type *t = ast->md;
  Type *return_opt_type = fn_return_type(t);
  LLVMTypeRef promise_type = type_to_llvm_type(return_opt_type, ctx, module);

  LLVMTypeRef coro_obj_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(), // counter
          GENERIC_PTR,     // fn ptr
          GENERIC_PTR,     // state
          promise_type,    // result or 'promise'
          GENERIC_PTR      // next
      },
      5, 0);

  CoroutineCtx coro_ctx = {
      .cons_type = t,
      .coro_obj_type = coro_obj_type,
      .promise_type = promise_type,
      .current_yield = 0,
      .num_coroutine_yields = ast->data.AST_LAMBDA.num_yields,
      .num_yield_boundary_xs = ast->data.AST_LAMBDA.num_yield_boundary_crossers,
      .yield_boundary_xs = ast->data.AST_LAMBDA.yield_boundary_crossers,
  };
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

  LLVMTypeRef coro_init_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
  LLVMValueRef init_fn =
      LLVMAddFunction(module, init_func_name, coro_init_type);
  LLVMSetLinkage(init_fn, LLVMExternalLinkage);

  LLVMTypeRef coro_type = PTR_ID_FUNC_TYPE(coro_obj_type);

  LLVMValueRef coro_fn = LLVMAddFunction(module, name, coro_type);
  LLVMSetLinkage(coro_fn, LLVMExternalLinkage);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(coro_fn, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, block);
  LLVMValueRef coro = LLVMGetParam(coro_fn, 0);

  if (ast->data.AST_LAMBDA.fn_name.chars) {
    add_recursive_coroutine_ref(ast, init_fn, coro_init_type, &fn_ctx);
  }

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
  coro_ctx.branches = branches;
  coro_ctx.switch_default = branches[coro_ctx.num_coroutine_yields];
  LLVMPositionBuilderAtEnd(builder, coro_ctx.switch_default);
  LLVMValueRef promise_struct = LLVMGetUndef(coro_ctx.promise_type);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt32Type(), 1, 0), 0,
                                        "insert_promise_status");

  LLVMValueRef promise_gep =
      LLVMBuildStructGEP2(builder, coro_ctx.coro_obj_type,
                          LLVMGetParam(coro_fn, 0), 3, "promise_gep");
  LLVMBuildStore(builder, promise_struct, promise_gep);

  LLVMValueRef state = coro_state(coro, coro_ctx.coro_obj_type, builder);
  LLVMBuildFree(builder, state);

  LLVMBuildRet(builder, coro);

  LLVMPositionBuilderAtEnd(builder, block);
  LLVMValueRef switch_ref =
      LLVMBuildSwitch(builder, coro_counter(coro, coro_obj_type, builder),
                      coro_ctx.switch_default, coro_ctx.num_coroutine_yields);

  for (int i = 0; i < coro_ctx.num_coroutine_yields; i++) {
    LLVMAddCase(switch_ref, LLVMConstInt(LLVMInt32Type(), i, 0), branches[i]);
  }

  coro_ctx.switch_ref = switch_ref;
  LLVMPositionBuilderAtEnd(builder, branches[0]);
  coro_ctx.func = coro_fn;

  LLVMValueRef body = codegen_lambda_body(ast, &fn_ctx, module, builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  destroy_ctx(&fn_ctx);

  LLVMValueRef init_func =
      compile_coroutine_init(name, coro_fn, init_fn, &fn_ctx, module, builder);

  return init_func;
}

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *args,
                         int args_len, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  return LLVMBuildCall2(builder, LLVMFunctionType(GENERIC_PTR, NULL, 0, 0),
                        sym->val, NULL, 0, "create_coro_inst");
}
static LLVMValueRef coro_fn_ptr(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                LLVMBuilderRef builder) {

  LLVMValueRef fn_ptr_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 1, "fn_ptr_gep");
  LLVMValueRef fn_ptr =
      LLVMBuildLoad2(builder, GENERIC_PTR, fn_ptr_gep, "fn_ptr");
  LLVMTypeRef fn_param_types[] = {LLVMPointerType(coro_obj_type, 0)};

  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMPointerType(coro_obj_type, 0), fn_param_types, 1, 0);

  LLVMValueRef typed_fn_ptr = LLVMBuildBitCast(
      builder, fn_ptr, LLVMPointerType(fn_type, 0), "typed_fn_ptr");

  return typed_fn_ptr;
}

static LLVMValueRef coro_promise(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                 LLVMTypeRef promise_type,
                                 LLVMBuilderRef builder) {

  LLVMValueRef promise_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 3, "promise_gep");
  LLVMValueRef promise =
      LLVMBuildLoad2(builder, promise_type, promise_gep, "promise");
  return promise;
}

static LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                               LLVMBuilderRef builder) {

  LLVMValueRef state_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 2, "state_gep");
  LLVMValueRef state = LLVMBuildLoad2(builder, GENERIC_PTR, state_gep, "state");
  return state;
}

static LLVMValueRef coro_promise_set(LLVMValueRef coro, LLVMValueRef val,
                                     LLVMTypeRef coro_obj_type,
                                     LLVMTypeRef promise_type,
                                     LLVMBuilderRef builder) {

  LLVMValueRef promise_struct = LLVMGetUndef(promise_type);

  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                        "coro.promise.tag");

  promise_struct = LLVMBuildInsertValue(builder, promise_struct, val, 1,
                                        "coro.promise.value");

  LLVMValueRef promise_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 3, "coro.promise.gep");
  LLVMBuildStore(builder, promise_struct, promise_gep);
  return coro;
}

static LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                                  LLVMTypeRef coro_obj_type,
                                  LLVMBuilderRef builder) {
  LLVMValueRef next_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 4, "coro.next.gep");
  LLVMBuildStore(builder, next, next_gep);
  return coro;
}
static LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  LLVMValueRef next_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 4, "coro.next.gep");
  LLVMValueRef next = LLVMBuildLoad2(builder, LLVMPointerType(coro_obj_type, 0),
                                     next_gep, "coro.next");
  return next;
}

static LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                 LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, 0, "counter_gep");
  LLVMValueRef counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "coro.counter");
  return counter;
}

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMValueRef coro = sym->val;
  Type *coro_type = sym->symbol_type;
  Type *ret_opt_type = fn_return_type(coro_type);
  LLVMTypeRef promise_type = type_to_llvm_type(ret_opt_type, ctx, module);

  LLVMTypeRef coro_obj_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(), // counter
          GENERIC_PTR,     // fn ptr
          GENERIC_PTR,     // state
          promise_type,    // result or 'promise'
          GENERIC_PTR      // next
      },
      5, 0);

  LLVMValueRef fn_ptr = coro_fn_ptr(coro, coro_obj_type, builder);

  LLVMValueRef args[] = {coro};
  LLVMValueRef new_cor = LLVMBuildCall2(
      builder, PTR_ID_FUNC_TYPE(coro_obj_type), fn_ptr, args, 1, "");
  sym->val = new_cor;

  return coro_promise(coro, coro_obj_type, promise_type, builder);
}

static LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                                 LLVMBuilderRef builder) {

  LLVMTypeRef promise_type = coro_ctx->promise_type;

  LLVMTypeRef coro_obj_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(), // counter
          GENERIC_PTR,     // fn ptr
          GENERIC_PTR,     // state
          promise_type,    // result or 'promise'
          GENERIC_PTR      // next
      },
      5, 0);

  LLVMValueRef fn_ptr = coro_fn_ptr(coro, coro_obj_type, builder);

  LLVMValueRef args[] = {coro};
  LLVMValueRef new_cor = LLVMBuildCall2(
      builder, PTR_ID_FUNC_TYPE(coro_obj_type), fn_ptr, args, 1, "");

  return new_cor;
}

static LLVMValueRef coro_incr(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                              LLVMBuilderRef builder) {

  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_ctx->coro_obj_type, coro, 0, "coro.counter.gep");
  LLVMValueRef current_counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "coro.counter");
  LLVMValueRef incremented_counter =
      LLVMBuildAdd(builder, current_counter,
                   LLVMConstInt(LLVMInt32Type(), 1, 0), "coro.counter.incr");
  LLVMBuildStore(builder, incremented_counter, counter_gep);
  return NULL;
}
static LLVMValueRef recursive_coro_yield(LLVMValueRef coro,
                                         CoroutineCtx *coro_ctx,
                                         LLVMBuilderRef builder) {

  LLVMValueRef func = coro_ctx->func;
  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_ctx->coro_obj_type, coro, 0, "coro.counter.gep");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);
  LLVMBuildRet(
      builder,
      LLVMBuildCall2(builder, PTR_ID_FUNC_TYPE(coro_ctx->coro_obj_type), func,
                     (LLVMValueRef[]){coro}, 1, "coro.recursive_call"));

  return NULL;
}
static LLVMValueRef tail_call_coro_yield(LLVMValueRef coro,
                                         LLVMValueRef new_cor,
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
static LLVMValueRef chain_new_coro_yield(LLVMValueRef coro,
                                         LLVMValueRef new_cor,
                                         CoroutineCtx *coro_ctx,
                                         LLVMBuilderRef builder) {

  coro_incr(coro, coro_ctx, builder);
  coro_next_set(new_cor, coro, coro_ctx->coro_obj_type, builder);
  LLVMValueRef fn_ptr = coro_fn_ptr(new_cor, coro_ctx->coro_obj_type, builder);

  new_cor =
      LLVMBuildCall2(builder, PTR_ID_FUNC_TYPE(coro_ctx->coro_obj_type), fn_ptr,
                     (LLVMValueRef[]){new_cor}, 1, "nested resume");

  LLVMValueRef promise_gep = LLVMBuildStructGEP2(
      builder, coro_ctx->coro_obj_type, coro, 3, "coro.promise.gep");

  LLVMValueRef new_promise = coro_promise(new_cor, coro_ctx->coro_obj_type,
                                          coro_ctx->promise_type, builder);

  LLVMBuildStore(builder, new_promise, promise_gep);

  LLVMBuildRet(builder, new_cor);
  return NULL;
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  LLVMValueRef coro = LLVMGetParam(coro_ctx->func, 0);

  if (ast->data.AST_YIELD.expr->tag == AST_APPLICATION) {
    Ast *expr = ast->data.AST_YIELD.expr;
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);

    if (sym && is_coroutine_constructor_type(sym->symbol_type) &&
        sym->symbol_data.STYPE_FUNCTION.recursive_ref) {
      return recursive_coro_yield(coro, coro_ctx, builder);
    }
    if (sym && is_coroutine_constructor_type(sym->symbol_type)) {
      if (coro_ctx->current_yield == coro_ctx->num_coroutine_yields - 1) {
        LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
        return tail_call_coro_yield(coro, new_cor, coro_ctx, builder);
      } else {
        LLVMValueRef new_cor = codegen(expr, ctx, module, builder);
        chain_new_coro_yield(coro, new_cor, coro_ctx, builder);
        coro_ctx->current_yield++;
        LLVMPositionBuilderAtEnd(builder,
                                 coro_ctx->branches[coro_ctx->current_yield]);
        return NULL;
      }
    }
  }

  int branch_idx = coro_ctx->current_yield;
  LLVMValueRef yield_val =
      codegen(ast->data.AST_YIELD.expr, ctx, module, builder);

  coro_incr(coro, coro_ctx, builder);
  coro_promise_set(coro, yield_val, coro_ctx->coro_obj_type,
                   coro_ctx->promise_type, builder);

  LLVMBuildRet(builder, coro);
  coro_ctx->current_yield++;
  LLVMPositionBuilderAtEnd(builder,
                           coro_ctx->branches[coro_ctx->current_yield]);
  return yield_val;
}
