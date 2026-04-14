#include "../../engine/ctx.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/parse.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./audio_jit.h"
#include "./compile_synth.h"
#include "application.h"
#include "function.h"
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

#include "./common.h"
#include "./dsp_build_expr.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

Ast *get_collection_proc_func(Ast *fn_ast) {
  if (fn_ast->tag == AST_RECORD_ACCESS) {
    return get_collection_proc_func(fn_ast->data.AST_RECORD_ACCESS.member);
  }

  if (fn_ast->tag == AST_IDENTIFIER) {
    if (is_ident(fn_ast, "fold") || is_ident(fn_ast, "foldi") ||
        is_ident(fn_ast, "map") || is_ident(fn_ast, "mapi") ||
        is_ident(fn_ast, "sum")) {
      return fn_ast;
    }
    return NULL;
  }

  return NULL;
}

LLVMValueRef dsp_array_sum(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *array_ast = ast->data.AST_APPLICATION.args;

  Type *arr_type = array_ast->type;
  Type *_el_type = arr_type->data.T_CONS.args[0];
  if (_el_type->kind == T_VAR) {
    _el_type = &t_num;
  }
  LLVMTypeRef el_type = type_to_llvm_type(_el_type, ctx, module);
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef idx_type = LLVMInt32Type();

  DspBuildCtx _dsp_ctx = *dsp_ctx;
  LLVMValueRef array_value =
      dsp_build_expr(array_ast, &_dsp_ctx, ctx, module, builder).scalar;
  dsp_ctx->state_offset = _dsp_ctx.state_offset;

  LLVMValueRef runtime_len =
      codegen_get_array_size(builder, array_value, el_type);

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMValueRef fn__ = LLVMGetBasicBlockParent(entry_block);

  LLVMBasicBlockRef cond_block = LLVMAppendBasicBlock(fn__, "sum.cond");
  LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(fn__, "sum.body");
  LLVMBasicBlockRef inc_block = LLVMAppendBasicBlock(fn__, "sum.inc");
  LLVMBasicBlockRef after_block = LLVMAppendBasicBlock(fn__, "sum.after");

  LLVMBuilderRef alloca_builder = LLVMCreateBuilder();
  LLVMValueRef first_inst = LLVMGetFirstInstruction(entry_block);
  if (first_inst) {
    LLVMPositionBuilderBefore(alloca_builder, first_inst);
  } else {
    LLVMPositionBuilderAtEnd(alloca_builder, entry_block);
  }
  LLVMValueRef idx_alloca =
      LLVMBuildAlloca(alloca_builder, idx_type, "sum.idx");
  LLVMValueRef acc_alloca = LLVMBuildAlloca(alloca_builder, f64_ty, "sum.acc");
  LLVMDisposeBuilder(alloca_builder);

  LLVMBuildStore(builder, LLVMConstInt(idx_type, 0, 0), idx_alloca);
  LLVMBuildStore(builder, LLVMConstReal(f64_ty, 0.0), acc_alloca);
  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);
  LLVMValueRef idx_val =
      LLVMBuildLoad2(builder, idx_type, idx_alloca, "sum.idx.val");
  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntSLT, idx_val, runtime_len, "sum.cond");
  LLVMBuildCondBr(builder, cond, body_block, after_block);

  LLVMPositionBuilderAtEnd(builder, body_block);
  LLVMValueRef idx_val_body =
      LLVMBuildLoad2(builder, idx_type, idx_alloca, "sum.idx.body");
  LLVMValueRef elem_val =
      get_array_element(builder, array_value, idx_val_body, el_type);
  elem_val = ensure_float(_el_type, elem_val, builder);
  LLVMValueRef acc_val =
      LLVMBuildLoad2(builder, f64_ty, acc_alloca, "sum.acc.val");
  LLVMValueRef next_acc =
      LLVMBuildFAdd(builder, acc_val, elem_val, "sum.next_acc");
  LLVMBuildStore(builder, next_acc, acc_alloca);
  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);
  LLVMValueRef idx_next = LLVMBuildAdd(
      builder, LLVMBuildLoad2(builder, idx_type, idx_alloca, "sum.idx.cur"),
      LLVMConstInt(idx_type, 1, 0), "sum.idx.next");
  LLVMBuildStore(builder, idx_next, idx_alloca);
  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);
  return LLVMBuildLoad2(builder, f64_ty, acc_alloca, "sum.result");
}

// ---------------------------------------------------------------------------
// Shared helpers for dsp_array_map / dsp_array_fold / dsp_delay_proc
// ---------------------------------------------------------------------------

// Build captured values from a lambda's closed_vals list.
// Returns num_captured; *out_vals is malloc'd (caller frees), NULL if 0.
// Derive param types via LLVMTypeOf rather than type_to_llvm_type — the latter
// returns garbage for complex DSP types (e.g. Array((delay_line, double))).
static int collect_captured_vals(Ast *anon_func, DspBuildCtx *dsp_ctx,
                                 JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder,
                                 LLVMValueRef **out_vals) {
  int n = 0;
  if (anon_func->tag == AST_LAMBDA && anon_func->type &&
      anon_func->type->closure_meta)
    n = anon_func->type->closure_meta->data.T_CONS.num_args;
  *out_vals = NULL;
  if (n > 0) {
    *out_vals = malloc(sizeof(LLVMValueRef) * (size_t)n);
    int i = 0;
    for (AstList *cv = anon_func->data.AST_LAMBDA.closed_vals; cv;
         cv = cv->next)
      (*out_vals)[i++] =
          dsp_build_expr(cv->ast, dsp_ctx, ctx, module, builder).scalar;
  }
  return n;
}

// Allocate per-element lambda state in the synth state region and emit per-slot
// init calls. Updates dsp_ctx->state_offset. Returns iter_region_base
// (NULL if iter_region_bytes == 0).
static LLVMValueRef
alloc_iter_lambda_state(DspBuildCtx *dsp_ctx, LLVMBuilderRef builder,
                        const SynthRecord *s, int comptime_size,
                        int iter_state_stride, int iter_region_bytes,
                        const char *prefix) {
  if (iter_region_bytes <= 0)
    return NULL;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();

  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + iter_region_bytes;

  char frame_name[64], init_name[64], slot_name[64];
  snprintf(frame_name, sizeof(frame_name), "%s.iter_region_base", prefix);
  snprintf(init_name, sizeof(init_name), "%s.iter_init_region_base", prefix);
  snprintf(slot_name, sizeof(slot_name), "%s.iter_init_state", prefix);

  LLVMValueRef base = dsp_consume_frame_state(dsp_ctx, builder,
                                              iter_region_bytes, 8, frame_name);
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, iter_region_bytes, 8, init_name);
    if (init_base && s->init_fn) {
      LLVMTypeRef init_fn_ty = LLVMGlobalGetValueType(s->init_fn);
      for (int i = 0; i < comptime_size; i++) {
        LLVMValueRef off_i64 =
            LLVMConstInt(i64_ty, (uint64_t)(i * iter_state_stride), 0);
        LLVMValueRef slot = LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty,
                                          init_base, &off_i64, 1, slot_name);
        LLVMBuildCall2(dsp_ctx->init_builder, init_fn_ty, s->init_fn,
                       (LLVMValueRef[]){slot}, 1, "iter_init_call");
      }
    }
  }
  return base;
}

// Per-iteration state pointer for the current loop index.
// Returns null ptr constant when iter_region_base is NULL (no per-element
// state).
static LLVMValueRef get_loop_iter_state_arg(LLVMBuilderRef builder,
                                            LLVMValueRef iter_region_base,
                                            LLVMValueRef iter_state_ptr_alloca,
                                            LLVMValueRef idx_i64,
                                            int iter_state_stride,
                                            const char *prefix) {
  if (!iter_region_base)
    return LLVMConstNull(GENERIC_PTR);

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  char byte_off_name[64], ptr_name[64], arg_name[64];
  snprintf(byte_off_name, sizeof(byte_off_name), "%s.iter_state.byte_off",
           prefix);
  snprintf(ptr_name, sizeof(ptr_name), "%s.iter_state_ptr", prefix);
  snprintf(arg_name, sizeof(arg_name), "%s.iter_state_arg", prefix);

  LLVMValueRef stride = LLVMConstInt(i64_ty, (uint64_t)iter_state_stride, 0);
  LLVMValueRef byte_off = LLVMBuildMul(builder, idx_i64, stride, byte_off_name);
  LLVMValueRef ptr =
      LLVMBuildGEP2(builder, i8_ty, iter_region_base, &byte_off, 1, ptr_name);
  LLVMBuildStore(builder, ptr, iter_state_ptr_alloca);
  return LLVMBuildLoad2(builder, GENERIC_PTR, iter_state_ptr_alloca, arg_name);
}

// Loop block + alloca scaffolding shared by map and fold.
// extra_type: non-NULL for fold (accumulator alloca); NULL for map.
typedef struct {
  LLVMBasicBlockRef cond, body, inc, after;
  LLVMValueRef idx_alloca;
  LLVMValueRef iter_state_ptr_alloca; // NULL if iter_region_bytes == 0
  LLVMValueRef extra_alloca;          // NULL unless extra_type provided
} ArrayLoopBlocks;

static ArrayLoopBlocks setup_array_loop(LLVMBuilderRef builder,
                                        LLVMTypeRef idx_type,
                                        int iter_region_bytes,
                                        LLVMTypeRef extra_type,
                                        const char *prefix) {
  LLVMBasicBlockRef entry = LLVMGetInsertBlock(builder);
  LLVMValueRef fn = LLVMGetBasicBlockParent(entry);

  char cond_n[32], body_n[32], inc_n[32], after_n[32];
  snprintf(cond_n, sizeof(cond_n), "%s.cond", prefix);
  snprintf(body_n, sizeof(body_n), "%s.body", prefix);
  snprintf(inc_n, sizeof(inc_n), "%s.inc", prefix);
  snprintf(after_n, sizeof(after_n), "%s.after", prefix);

  ArrayLoopBlocks r = {
      .cond = LLVMAppendBasicBlock(fn, cond_n),
      .body = LLVMAppendBasicBlock(fn, body_n),
      .inc = LLVMAppendBasicBlock(fn, inc_n),
      .after = LLVMAppendBasicBlock(fn, after_n),
  };

  LLVMBuilderRef ab = LLVMCreateBuilder();
  LLVMValueRef first = LLVMGetFirstInstruction(entry);
  if (first)
    LLVMPositionBuilderBefore(ab, first);
  else
    LLVMPositionBuilderAtEnd(ab, entry);

  r.idx_alloca = LLVMBuildAlloca(ab, idx_type, "loop.idx");
  if (iter_region_bytes > 0)
    r.iter_state_ptr_alloca =
        LLVMBuildAlloca(ab, GENERIC_PTR, "loop.iter_state_ptr");
  if (extra_type)
    r.extra_alloca = LLVMBuildAlloca(ab, extra_type, "loop.extra");
  LLVMDisposeBuilder(ab);
  return r;
}

// ---------------------------------------------------------------------------

DspValue dsp_array_map(bool with_index, Ast *ast, DspBuildCtx *dsp_ctx,
                       JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder) {
  Ast *anon_func = ast->data.AST_APPLICATION.args;
  Ast *array_ast = ast->data.AST_APPLICATION.args + 1;

  Type *in_arr_type = array_ast->type;
  Type *_in_el_type = in_arr_type->data.T_CONS.args[0];
  if (_in_el_type->kind == T_VAR) {
    _in_el_type = &t_num;
  }
  LLVMTypeRef in_el_type = type_to_llvm_type(_in_el_type, ctx, module);

  Type *_out_el_type = (ast->type && ast->type->kind == T_CONS &&
                        ast->type->data.T_CONS.num_args > 0)
                           ? ast->type->data.T_CONS.args[0]
                           : _in_el_type;
  if (!_out_el_type || _out_el_type->kind == T_VAR) {
    _out_el_type = _in_el_type;
  }
  LLVMTypeRef out_el_type = type_to_llvm_type(_out_el_type, ctx, module);
  if (!out_el_type) {
    out_el_type = in_el_type;
  }

  LLVMTypeRef idx_type = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();

  // Build captured vals first — derive their frame param types from LLVMTypeOf
  // rather than type_to_llvm_type, which can return garbage for complex DSP
  // types (e.g. Array((delay_line, double))) that lack clean type-system reps.
  LLVMValueRef *captured_vals;
  int num_captured = collect_captured_vals(anon_func, dsp_ctx, ctx, module,
                                           builder, &captured_vals);

  // frame_ty: (state_ptr, node_ptr, out_ptr, [idx,] element, ...captured) ->
  // void
  int max_params = 5 + num_captured;
  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)max_params);
  int frame_nparams = 0;
  frame_param_tys[frame_nparams++] = GENERIC_PTR;
  frame_param_tys[frame_nparams++] = GENERIC_PTR;
  frame_param_tys[frame_nparams++] = LLVMPointerType(out_el_type, 0);
  if (with_index) {
    frame_param_tys[frame_nparams++] = idx_type;
  }
  frame_param_tys[frame_nparams++] = in_el_type;
  for (int i = 0; i < num_captured; i++) {
    frame_param_tys[frame_nparams++] = LLVMTypeOf(captured_vals[i]);
  }
  LLVMTypeRef frame_ty =
      LLVMFunctionType(LLVMVoidType(), frame_param_tys, frame_nparams, 0);
  free(frame_param_tys);

  SynthRecord s = compile_lambda_to_synth_record(anon_func, "anon", frame_ty,
                                                 ctx, module, builder);

  DspBuildCtx _dsp_ctx = *dsp_ctx;
  LLVMValueRef array_value =
      dsp_build_expr(array_ast, &_dsp_ctx, ctx, module, builder).scalar;
  DspArrayAttributes array_attrs = _dsp_ctx.array_attrs;
  dsp_ctx->state_offset = _dsp_ctx.state_offset;

  if (s.state_bytes && array_attrs.comptime_size == 0) {
    fprintf(stderr,
            "Error - map function makes synth-lifetime allocations but array "
            "mapped over has size which is unknown at comptime\n");
    print_ast_err(ast);
    return DSP_NULL;
  }

  int iter_state_stride = s.state_bytes ? (s.state_bytes + 7) & ~7 : 0;
  int iter_region_bytes = array_attrs.comptime_size * iter_state_stride;

  // Stack-allocate output data before the loop
  LLVMValueRef runtime_len =
      codegen_get_array_size(builder, array_value, in_el_type);
  LLVMValueRef *out_data_ptrs =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)s.output_lanes, 8);
  if (!out_data_ptrs) {
    free(captured_vals);
    return DSP_NULL;
  }
  for (int lane = 0; lane < s.output_lanes; lane++) {
    out_data_ptrs[lane] =
        LLVMBuildArrayAlloca(builder, out_el_type, runtime_len, "map.out.data");
  }

  LLVMValueRef iter_region_base =
      alloc_iter_lambda_state(dsp_ctx, builder, &s, array_attrs.comptime_size,
                              iter_state_stride, iter_region_bytes, "map");

  ArrayLoopBlocks loop =
      setup_array_loop(builder, idx_type, iter_region_bytes, NULL, "map");

  LLVMBuildStore(builder, LLVMConstInt(idx_type, 0, 0), loop.idx_alloca);
  LLVMBuildBr(builder, loop.cond);

  LLVMPositionBuilderAtEnd(builder, loop.cond);
  LLVMValueRef idx_val =
      LLVMBuildLoad2(builder, idx_type, loop.idx_alloca, "map.idx.val");
  LLVMBuildCondBr(
      builder,
      LLVMBuildICmp(builder, LLVMIntSLT, idx_val, runtime_len, "map.cond"),
      loop.body, loop.after);

  LLVMPositionBuilderAtEnd(builder, loop.body);
  LLVMValueRef idx_val_body =
      LLVMBuildLoad2(builder, idx_type, loop.idx_alloca, "map.idx.body");
  LLVMValueRef idx_i64 =
      LLVMBuildSExt(builder, idx_val_body, i64_ty, "map.idx.i64");

  LLVMValueRef state_arg = get_loop_iter_state_arg(
      builder, iter_region_base, loop.iter_state_ptr_alloca, idx_i64,
      iter_state_stride, "map");

  LLVMValueRef elem_val =
      get_array_element(builder, array_value, idx_val_body, in_el_type);
  elem_val = ensure_float(_in_el_type, elem_val, builder);

  LLVMValueRef frame_fn = s.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);
  unsigned frame_total_args = LLVMCountParamTypes(frame_fn_ty);
  LLVMValueRef *frame_args =
      malloc(sizeof(LLVMValueRef) * (size_t)frame_total_args);
  unsigned nargs = 0;
  frame_args[nargs++] = state_arg;
  frame_args[nargs++] = LLVMConstNull(GENERIC_PTR);
  LLVMValueRef mapped_out_ptr = LLVMBuildArrayAlloca(
      builder, out_el_type,
      LLVMConstInt(LLVMInt32Type(), (uint64_t)s.output_lanes, 0),
      "map.out.tmp");
  frame_args[nargs++] = mapped_out_ptr;
  if (with_index)
    frame_args[nargs++] = idx_val_body;
  frame_args[nargs++] = elem_val;
  for (int i = 0; i < num_captured; i++)
    frame_args[nargs++] = captured_vals[i];

  LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args, nargs,
                 "map.out_elem");
  free(frame_args);
  free(captured_vals);
  for (int lane = 0; lane < s.output_lanes; lane++) {
    LLVMValueRef lane_idx = LLVMConstInt(i64_ty, (uint64_t)lane, 0);
    LLVMValueRef lane_ptr = LLVMBuildGEP2(builder, out_el_type, mapped_out_ptr,
                                          &lane_idx, 1, "map.out.val.ptr");
    LLVMValueRef mapped_val =
        LLVMBuildLoad2(builder, out_el_type, lane_ptr, "map.out.val");
    LLVMValueRef out_elem_ptr =
        LLVMBuildGEP2(builder, out_el_type, out_data_ptrs[lane],
                      (LLVMValueRef[]){idx_i64}, 1, "map.out.elem_ptr");
    LLVMBuildStore(builder, mapped_val, out_elem_ptr);
  }
  LLVMBuildBr(builder, loop.inc);

  LLVMPositionBuilderAtEnd(builder, loop.inc);
  LLVMValueRef idx_next = LLVMBuildAdd(
      builder,
      LLVMBuildLoad2(builder, idx_type, loop.idx_alloca, "map.idx.cur"),
      LLVMConstInt(idx_type, 1, 0), "map.idx.next");
  LLVMBuildStore(builder, idx_next, loop.idx_alloca);
  LLVMBuildBr(builder, loop.cond);

  LLVMPositionBuilderAtEnd(builder, loop.after);

  dsp_ctx->array_attrs =
      (DspArrayAttributes){.comptime_size = array_attrs.comptime_size};
  LLVMTypeRef out_arr_ty = codegen_array_type(out_el_type);
  if (s.output_lanes <= 1) {
    LLVMValueRef out_arr = LLVMGetUndef(out_arr_ty);
    out_arr =
        LLVMBuildInsertValue(builder, out_arr, runtime_len, 0, "map.out.size");
    out_arr = LLVMBuildInsertValue(builder, out_arr, out_data_ptrs[0], 1,
                                   "map.out.data_ptr");
    return DSP_SCALAR(out_arr);
  }

  LLVMValueRef *lane_arrays =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)s.output_lanes, 8);
  if (!lane_arrays) {
    return DSP_NULL;
  }
  for (int lane = 0; lane < s.output_lanes; lane++) {
    LLVMValueRef out_arr = LLVMGetUndef(out_arr_ty);
    out_arr =
        LLVMBuildInsertValue(builder, out_arr, runtime_len, 0, "map.out.size");
    out_arr = LLVMBuildInsertValue(builder, out_arr, out_data_ptrs[lane], 1,
                                   "map.out.data_ptr");
    lane_arrays[lane] = out_arr;
  }
  return DSP_MULTI(s.output_lanes, lane_arrays);
}

DspValue dsp_delay_proc(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  // Args: f (anon_func), dl (delay buffer array), inp (input signal)
  Ast *anon_func = ast->data.AST_APPLICATION.args;
  // print_ast(anon_func);
  // print_type(anon_func->type);
  Ast *dl_ast = ast->data.AST_APPLICATION.args + 1;
  Ast *inp_ast = ast->data.AST_APPLICATION.args + 2;

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ptr_ty = LLVMPointerType(i32_ty, 0);

  // 1. Closure captures — build values first, derive types from LLVMTypeOf
  LLVMValueRef *captured_vals = NULL;
  int num_captured = collect_captured_vals(anon_func, dsp_ctx, ctx, module,
                                           builder, &captured_vals);

  // 2. Build dl and inp so state_offset advances before compile_lambda
  DspBuildCtx _dsp_ctx = *dsp_ctx;
  LLVMValueRef dl_val =
      dsp_build_expr(dl_ast, &_dsp_ctx, ctx, module, builder).scalar;
  dsp_ctx->state_offset = _dsp_ctx.state_offset;
  LLVMValueRef inp_val =
      dsp_build_expr(inp_ast, dsp_ctx, ctx, module, builder).scalar;
  inp_val = ensure_float(inp_ast->type, inp_val, builder);

  // 3. frame_ty: (state*, node*, out_ptr, i32 w, dl_struct, f64 inp, ...captured) -> void
  LLVMTypeRef dl_llvm_ty = LLVMTypeOf(dl_val);
  int frame_nparams = 6 + num_captured;
  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)frame_nparams);
  int pi = 0;
  frame_param_tys[pi++] = GENERIC_PTR;
  frame_param_tys[pi++] = GENERIC_PTR;
  frame_param_tys[pi++] = LLVMPointerType(f64_ty, 0);
  frame_param_tys[pi++] = i32_ty;
  frame_param_tys[pi++] = dl_llvm_ty;
  frame_param_tys[pi++] = f64_ty;
  for (int i = 0; i < num_captured; i++)
    frame_param_tys[pi++] = LLVMTypeOf(captured_vals[i]);
  LLVMTypeRef frame_ty = LLVMFunctionType(LLVMVoidType(), frame_param_tys, pi, 0);
  free(frame_param_tys);

  SynthRecord s = compile_lambda_to_synth_record(
      anon_func, "delay_proc", frame_ty, ctx, module, builder);

  // 4. Frame state: 4-byte write pointer (zeroed by ctor) + lambda state
  int wp_off = (dsp_ctx->state_offset + 3) & ~3;
  dsp_ctx->state_offset = wp_off + 4;
  LLVMValueRef wp_base =
      dsp_consume_frame_state(dsp_ctx, builder, 4, 4, "delay_proc.wp");
  LLVMValueRef wp_ptr =
      LLVMBuildBitCast(builder, wp_base, i32_ptr_ty, "delay_proc.wp_ptr");

  LLVMValueRef fn_state = LLVMConstNull(GENERIC_PTR);
  if (s.state_bytes > 0) {
    int fn_state_off = (dsp_ctx->state_offset + 7) & ~7;
    dsp_ctx->state_offset = fn_state_off + s.state_bytes;
    fn_state = dsp_consume_frame_state(dsp_ctx, builder, s.state_bytes, 8,
                                       "delay_proc.fn_state");
    if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
      (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 4, 4,
                                   "delay_proc.init.wp");
      LLVMValueRef init_fn_state =
          dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, s.state_bytes,
                                 8, "delay_proc.init.fn_state");
      if (s.init_fn && init_fn_state) {
        LLVMTypeRef init_fn_ty = LLVMGlobalGetValueType(s.init_fn);
        LLVMBuildCall2(dsp_ctx->init_builder, init_fn_ty, s.init_fn,
                       (LLVMValueRef[]){init_fn_state}, 1,
                       "delay_proc.init_call");
      }
    }
  } else if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 4, 4,
                                 "delay_proc.init.wp");
  }

  // 5. Load write pointer
  LLVMValueRef w = LLVMBuildLoad2(builder, i32_ty, wp_ptr, "delay_proc.w");

  if (s.output_lanes != 1) {
    fprintf(stderr, "audio_jit: delay_proc callback must return a scalar\n");
    if (captured_vals)
      free(captured_vals);
    return DSP_SCALAR(LLVMConstReal(f64_ty, 0.0));
  }

  // 6. Call frame_fn(fn_state, NULL, out_ptr, w, dl, inp, ...captured)
  int total_args = 6 + num_captured;
  LLVMValueRef *frame_args = malloc(sizeof(LLVMValueRef) * (size_t)total_args);
  int ai = 0;
  frame_args[ai++] = fn_state;
  frame_args[ai++] = LLVMConstNull(GENERIC_PTR);
  LLVMValueRef out_ptr =
      LLVMBuildArrayAlloca(builder, f64_ty, LLVMConstInt(i32_ty, 1, 0),
                           "delay_proc.out_ptr");
  frame_args[ai++] = out_ptr;
  frame_args[ai++] = w;
  frame_args[ai++] = dl_val;
  frame_args[ai++] = inp_val;
  for (int i = 0; i < num_captured; i++)
    frame_args[ai++] = captured_vals[i];
  LLVMValueRef frame_fn = s.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);
  LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args, (unsigned)ai,
                 "delay_proc.out");
  free(frame_args);
  if (captured_vals)
    free(captured_vals);
  LLVMValueRef out_val =
      LLVMBuildLoad2(builder, f64_ty, out_ptr, "delay_proc.out_val");

  // 7. dl[w] = out_val  (safe — dl lives in synth state via EA_ATTR_MUTABLE)
  set_array_element(builder, dl_val, w, out_val, f64_ty);

  // 8. *wp_ptr = (w + 1) % max_size
  LLVMValueRef max_size = codegen_get_array_size(builder, dl_val, f64_ty);
  LLVMValueRef w1 =
      LLVMBuildAdd(builder, w, LLVMConstInt(i32_ty, 1, 0), "delay_proc.w1");
  LLVMValueRef w_next =
      LLVMBuildSRem(builder, w1, max_size, "delay_proc.w_next");
  LLVMBuildStore(builder, w_next, wp_ptr);

  return DSP_SCALAR(out_val);
}

static DspValue dsp_array_fold(bool with_index, Ast *ast, DspBuildCtx *dsp_ctx,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  Ast *anon_func = ast->data.AST_APPLICATION.args;
  Ast *init_ast = ast->data.AST_APPLICATION.args + 1;
  Ast *array_ast = ast->data.AST_APPLICATION.args + 2;

  Type *arr_type = array_ast->type;
  Type *_el_type = arr_type->data.T_CONS.args[0];
  if (_el_type->kind == T_VAR) {
    _el_type = &t_num;
  }
  LLVMTypeRef el_type = type_to_llvm_type(_el_type, ctx, module);
  LLVMTypeRef idx_type = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();

  Type *_fold_type = init_ast->type;
  if (!_fold_type || _fold_type->kind == T_VAR) {
    _fold_type = &t_num;
  }
  LLVMTypeRef fold_type = type_to_llvm_type(_fold_type, ctx, module);
  if (!fold_type) {
    fold_type = LLVMDoubleType();
  }

  LLVMValueRef *captured_vals = NULL;
  int num_captured = collect_captured_vals(anon_func, dsp_ctx, ctx, module,
                                           builder, &captured_vals);

  // frame_ty: (state_ptr, node_ptr, out_ptr, [idx,] acc, element, ...captured)
  // -> void. Use LLVMTypeOf on already-built captured vals so complex DSP
  // types (e.g. arrays of tuples) get correct struct param types.
  int max_params = 6 + num_captured;
  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)max_params);
  int frame_nparams = 0;
  frame_param_tys[frame_nparams++] = GENERIC_PTR;                   // state ptr
  frame_param_tys[frame_nparams++] = GENERIC_PTR;                   // node ptr
  frame_param_tys[frame_nparams++] = LLVMPointerType(fold_type, 0); // out ptr
  if (with_index) {
    frame_param_tys[frame_nparams++] = idx_type; // i32 index
  }
  frame_param_tys[frame_nparams++] = fold_type; // accumulator
  frame_param_tys[frame_nparams++] = el_type;   // element
  for (int i = 0; i < num_captured; i++) {
    frame_param_tys[frame_nparams++] = LLVMTypeOf(captured_vals[i]);
  }
  LLVMTypeRef frame_ty =
      LLVMFunctionType(LLVMVoidType(), frame_param_tys, frame_nparams, 0);
  free(frame_param_tys);

  SynthRecord s = compile_lambda_to_synth_record(anon_func, "anon", frame_ty,
                                                 ctx, module, builder);

  // Build array via a copy of dsp_ctx to capture array_attrs, then sync
  // state_offset back so subsequent allocations are correctly sequenced.
  DspBuildCtx _dsp_ctx = *dsp_ctx;
  LLVMValueRef array_value =
      dsp_build_expr(array_ast, &_dsp_ctx, ctx, module, builder).scalar;
  DspArrayAttributes array_attrs = _dsp_ctx.array_attrs;
  dsp_ctx->state_offset = _dsp_ctx.state_offset;

  if (s.state_bytes && array_attrs.comptime_size == 0) {
    fprintf(stderr,
            "Error - fold function makes synth-lifetime allocations but array "
            "folded over has size which is unknown at comptime\n");
    print_ast_err(ast);
    return DSP_NULL;
  }

  int iter_state_stride = s.state_bytes ? (s.state_bytes + 7) & ~7 : 0;
  int iter_region_bytes = array_attrs.comptime_size * iter_state_stride;

  LLVMValueRef init_val =
      dsp_build_expr(init_ast, dsp_ctx, ctx, module, builder).scalar;
  if (LLVMGetTypeKind(fold_type) == LLVMDoubleTypeKind) {
    init_val = ensure_float(init_ast->type, init_val, builder);
  }

  ArrayLoopBlocks lb =
      setup_array_loop(builder, idx_type, iter_region_bytes, fold_type, "fold");
  LLVMBasicBlockRef cond_block = lb.cond;
  LLVMBasicBlockRef body_block = lb.body;
  LLVMBasicBlockRef inc_block = lb.inc;
  LLVMBasicBlockRef after_block = lb.after;
  LLVMValueRef idx_alloca = lb.idx_alloca;
  LLVMValueRef acc_alloca = lb.extra_alloca;
  LLVMValueRef iter_state_ptr_alloca = lb.iter_state_ptr_alloca;

  LLVMValueRef iter_region_base =
      alloc_iter_lambda_state(dsp_ctx, builder, &s, array_attrs.comptime_size,
                              iter_state_stride, iter_region_bytes, "fold");

  // Runtime loop length extracted from the array value
  LLVMValueRef loop_length_val =
      codegen_get_array_size(builder, array_value, el_type);

  LLVMBuildStore(builder, LLVMConstInt(idx_type, 0, 0), idx_alloca);
  LLVMBuildStore(builder, init_val, acc_alloca);
  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);
  LLVMValueRef idx_val =
      LLVMBuildLoad2(builder, idx_type, idx_alloca, "fold.idx.val");
  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntSLT, idx_val, loop_length_val, "fold.cond");
  LLVMBuildCondBr(builder, cond, body_block, after_block);

  LLVMPositionBuilderAtEnd(builder, body_block);

  LLVMValueRef idx_val_body =
      LLVMBuildLoad2(builder, idx_type, idx_alloca, "fold.idx.body");
  LLVMValueRef idx_i64 =
      LLVMBuildSExt(builder, idx_val_body, i64_ty, "fold.idx.i64");

  LLVMValueRef state_arg =
      get_loop_iter_state_arg(builder, iter_region_base, iter_state_ptr_alloca,
                              idx_i64, iter_state_stride, "fold");

  LLVMValueRef elem_val =
      get_array_element(builder, array_value, idx_val_body, el_type);
  elem_val = ensure_float(_el_type, elem_val, builder);

  LLVMValueRef acc_val =
      LLVMBuildLoad2(builder, fold_type, acc_alloca, "fold.acc.val");

  if (s.output_lanes != 1) {
    fprintf(stderr, "audio_jit: array fold callback must return a scalar\n");
    free(captured_vals);
    return DSP_SCALAR(LLVMConstNull(fold_type));
  }

  LLVMValueRef frame_fn = s.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);
  unsigned frame_total_args = LLVMCountParamTypes(frame_fn_ty);
  LLVMValueRef *frame_args =
      malloc(sizeof(LLVMValueRef) * (size_t)frame_total_args);
  unsigned nargs = 0;
  frame_args[nargs++] = state_arg;
  frame_args[nargs++] = LLVMConstNull(GENERIC_PTR);
  LLVMValueRef next_acc_ptr =
      LLVMBuildAlloca(builder, fold_type, "fold.next_acc_ptr");
  frame_args[nargs++] = next_acc_ptr;
  if (with_index) {
    frame_args[nargs++] = idx_val_body; // foldi: f(idx, acc, element)
  }
  frame_args[nargs++] = acc_val;
  frame_args[nargs++] = elem_val;
  for (int i = 0; i < num_captured; i++) {
    frame_args[nargs++] = captured_vals[i];
  }

  LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args, nargs,
                 "fold.next_acc");
  free(frame_args);
  free(captured_vals);
  LLVMValueRef next_acc =
      LLVMBuildLoad2(builder, fold_type, next_acc_ptr, "fold.next_acc.val");

  LLVMBuildStore(builder, next_acc, acc_alloca);
  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);
  LLVMValueRef idx_next = LLVMBuildAdd(
      builder, LLVMBuildLoad2(builder, idx_type, idx_alloca, "fold.idx.cur"),
      LLVMConstInt(idx_type, 1, 0), "fold.idx.next");
  LLVMBuildStore(builder, idx_next, idx_alloca);
  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);
  return DSP_SCALAR(
      LLVMBuildLoad2(builder, fold_type, acc_alloca, "fold.result"));
}

DspValue call_dsp_list_proc(Ast *fn_ast, Ast *ast, DspBuildCtx *dsp_ctx,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  if (is_array_type(ast->data.AST_APPLICATION.args[0].type) &&
      is_ident(fn_ast, "sum")) {

    return DSP_SCALAR(dsp_array_sum(ast, dsp_ctx, ctx, module, builder));
  }
  if (is_array_type(ast->data.AST_APPLICATION.args[1].type)) {
    if (is_ident(fn_ast, "mapi")) {
      return dsp_array_map(true, ast, dsp_ctx, ctx, module, builder);
    }
    if (is_ident(fn_ast, "map")) {
      return dsp_array_map(false, ast, dsp_ctx, ctx, module, builder);
    }
  }

  if (is_array_type(ast->data.AST_APPLICATION.args[2].type)) {
    if (is_ident(fn_ast, "foldi")) {
      return dsp_array_fold(true, ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(fn_ast, "fold")) {
      return dsp_array_fold(false, ast, dsp_ctx, ctx, module, builder);
    }

  } else if (is_list_type(ast->data.AST_APPLICATION.args[2].type)) {
    if (is_ident(fn_ast, "foldi")) {
      return dsp_array_fold(true, ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(fn_ast, "fold")) {
      return dsp_array_fold(false, ast, dsp_ctx, ctx, module, builder);
    }
  }
  return DSP_NULL;
}
