#include "./audio_jit.h"

#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/common.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./compile_synth.h"

#include <llvm-c/Core.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SIN_TABSIZE
#define SIN_TABSIZE (1 << 11)
#endif

const double ylc_sin_table[SIN_TABSIZE] = {
#include "../../engine/assets/sin_table.csv"
};

#ifndef SQ_TABSIZE
#define SQ_TABSIZE (1 << 11)
#endif
const double ylc_sq_table[SQ_TABSIZE] = {
#include "../../engine/assets/sq_table.csv"
};

#ifndef SAW_TABSIZE
#define SAW_TABSIZE (1 << 11)
#endif

const double ylc_saw_table[SAW_TABSIZE] = {
#include "../../engine/assets/saw_table.csv"
};

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
int STYPE_AUDIO_JIT_INLINE_LAMBDA;
int STYPE_AUDIO_JIT_SYNTH_INLET;

// let array_of_buf = extern fn Ptr -> Array of Double;
// let bufsize = extern fn Ptr -> Int;
_DoubleArray array_of_buf(NodeRef buf) {
  return (_DoubleArray){.data = buf->output.buf, .size = buf->output.size};
}
int bufsize(NodeRef buf) { return buf->output.size; }

Node *ylc_create_audio_node(perform_func_t perform, int num_inputs,
                            int state_bytes) {
  size_t total =
      sizeof(Node) + (size_t)state_bytes + ((size_t)BUF_SIZE * sizeof(double));
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return NULL;
  }

  node->perform = perform;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
  node->meta = (char *)"audio_jit_synth";
  node->output = (Signal){
      .layout = 1,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->next = NULL;

  return node;
}

Node *ylc_const_inlet(double val) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);
  int saved_idx = node->node_index;

  *node = (Node){
      .perform = NULL,
      .node_index = saved_idx,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = (char *)"jit_const_inlet",
  };

  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.buf[i] = val;
  }

  return node;
}

void dsp_write_output(void *node_raw, int64_t frame, double val) {
  ((Node *)node_raw)->output.buf[frame] = val;
}

double ylc_read_inlet_node(void *node_raw, int64_t frame) {
  return ((Node *)node_raw)->output.buf[frame];
}

#define _EPSILON 0.0001
double exp_decay_multiplier(double T) {
  const double epsilon = _EPSILON;
  double spf = ctx_spf();
  if (T <= 0.0 || spf <= 0.0) {
    return 0.0;
  }
  return pow(epsilon, spf / T);
}

static LLVMValueRef SinOscHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module_ref,
                                  LLVMBuilderRef builder) {

  Ast *sym_id = ast->data.AST_APPLICATION.function;

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

static LLVMValueRef get_table_global_ptr(const char *sym_name,
                                         int32_t table_size,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef arr_ty = LLVMArrayType(f64_ty, table_size);
  LLVMValueRef global = LLVMGetNamedGlobal(module, sym_name);
  if (!global) {
    global = LLVMAddGlobal(module, arr_ty, sym_name);
    LLVMSetLinkage(global, LLVMExternalLinkage);
  }

  LLVMValueRef zero = LLVMConstInt(i32_ty, 0, 0);
  LLVMValueRef idxs[] = {zero, zero};
  return LLVMBuildGEP2(builder, arr_ty, global, idxs, 2, "tab.base");
}

static bool ast_is_const(Ast *ast, JITLangCtx *jit_ctx) {
  TICtx ti = {};
  ti.env = jit_ctx->env;
  return is_constant_expr(ast, &ti);
}

static bool ast_try_eval_const_num(Ast *ast, double *out) {
  if (!ast || !out) {
    return false;
  }

  switch (ast->tag) {
  case AST_INT: {
    *out = (double)ast->data.AST_INT.value;
    return true;
  }
  case AST_DOUBLE: {
    *out = ast->data.AST_DOUBLE.value;
    return true;
  }
  case AST_APPLICATION: {
    Ast *fn = ast->data.AST_APPLICATION.function;
    Ast *args = ast->data.AST_APPLICATION.args;
    if (!fn || fn->tag != AST_IDENTIFIER ||
        ast->data.AST_APPLICATION.len != 2) {
      return false;
    }

    double a, b;
    if (!ast_try_eval_const_num(args, &a) ||
        !ast_try_eval_const_num(args + 1, &b)) {
      return false;
    }

    const char *name = fn->data.AST_IDENTIFIER.value;
    if (strcmp(name, "+") == 0) {
      *out = a + b;
      return true;
    }
    if (strcmp(name, "-") == 0) {
      *out = a - b;
      return true;
    }
    if (strcmp(name, "*") == 0) {
      *out = a * b;
      return true;
    }
    if (strcmp(name, "/") == 0) {
      *out = a / b;
      return true;
    }
    if (strcmp(name, "%") == 0) {
      *out = fmod(a, b);
      return true;
    }
    return false;
  }
  default:
    return false;
  }
}

static bool ast_is_const_zero(Ast *ast, JITLangCtx *jit_ctx) {
  double value = 1.0;
  return ast_is_const(ast, jit_ctx) && ast_try_eval_const_num(ast, &value) &&
         value == 0.0;
}

LLVMValueRef builtin_phasor(LLVMValueRef freq, DspBuildCtx *dsp_ctx,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();

  // ctor allocation is zeroed, so the initial phase is already 0.0.
  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef phase_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &off_val, 1, "phasor.phase_ptr");
  LLVMValueRef phase =
      LLVMBuildLoad2(builder, f64_ty, phase_ptr, "phasor.phase");
  LLVMValueRef spf = dsp_ctx->spf;
  LLVMValueRef step = LLVMBuildFMul(builder, freq, spf, "phasor.step");
  LLVMValueRef advanced =
      LLVMBuildFAdd(builder, phase, step, "phasor.advanced");

  LLVMValueRef zero = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one = LLVMConstReal(f64_ty, 1.0);
  LLVMValueRef ovf =
      LLVMBuildFCmp(builder, LLVMRealOGE, advanced, one, "phasor.ovf");
  LLVMValueRef udf =
      LLVMBuildFCmp(builder, LLVMRealOLT, advanced, zero, "phasor.udf");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, zero, advanced, "phasor.wrap_ovf");
  next = LLVMBuildSelect(builder, udf, one, next, "phasor.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  return phase;
}

LLVMValueRef builtin_phasor_sinc(LLVMValueRef freq, LLVMValueRef trig,
                                 DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();

  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef prev_off_val = LLVMConstInt(i32_ty, (uint64_t)(off + 8), 0);
  if (dsp_ctx->ctor_state_ptr) {
    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->ctor_builder, i8_ty, dsp_ctx->ctor_state_ptr,
                      &prev_off_val, 1, "phasor.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr = LLVMBuildBitCast(
        dsp_ctx->ctor_builder, prev_init_ptr_i8, LLVMPointerType(f64_ty, 0),
        "phasor.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->ctor_builder, LLVMConstReal(f64_ty, 0.0),
                   prev_init_ptr);
  }

  LLVMValueRef phase_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &off_val, 1, "phasor.phase_ptr");
  LLVMValueRef prev_trig_ptr =
      LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr, &prev_off_val, 1,
                    "phasor.prev_trig_ptr");
  LLVMValueRef phase =
      LLVMBuildLoad2(builder, f64_ty, phase_ptr, "phasor.phase");
  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_trig_ptr, "phasor.prev_trig");

  LLVMValueRef spf = dsp_ctx->spf;
  LLVMValueRef half = LLVMConstReal(f64_ty, 0.5);
  LLVMValueRef trig_hi =
      LLVMBuildFCmp(builder, LLVMRealOGE, trig, half, "phasor.trig_hi");
  LLVMValueRef prev_lo =
      LLVMBuildFCmp(builder, LLVMRealOLT, prev_trig, half, "phasor.prev_lo");
  LLVMValueRef rising =
      LLVMBuildAnd(builder, trig_hi, prev_lo, "phasor.trig_rising");

  LLVMValueRef cur_phase = LLVMBuildSelect(
      builder, rising, LLVMConstReal(f64_ty, 0.0), phase, "phasor.cur_phase");
  LLVMValueRef step = LLVMBuildFMul(builder, freq, spf, "phasor.step");
  LLVMValueRef advanced =
      LLVMBuildFAdd(builder, cur_phase, step, "phasor.advanced");

  LLVMValueRef zero = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one = LLVMConstReal(f64_ty, 1.0);
  LLVMValueRef ovf =
      LLVMBuildFCmp(builder, LLVMRealOGE, advanced, one, "phasor.ovf");
  LLVMValueRef udf =
      LLVMBuildFCmp(builder, LLVMRealOLT, advanced, zero, "phasor.udf");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, zero, advanced, "phasor.wrap_ovf");
  next = LLVMBuildSelect(builder, udf, one, next, "phasor.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  LLVMBuildStore(builder, trig, prev_trig_ptr);

  return cur_phase;
}

LLVMValueRef builtin_trig(LLVMValueRef freq, bool freq_is_const_zero,
                          DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {
  (void)ctx;
  (void)module;

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMValueRef zero_f = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one_f = LLVMConstReal(f64_ty, 1.0);

  if (freq_is_const_zero) {
    LLVMValueRef latch_off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
    LLVMValueRef cur_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &latch_off_val, 1, "trig.cur_ptr");
    LLVMValueRef prev_i64 =
        LLVMBuildLoad2(builder, i64_ty, cur_ptr, "trig.prev_i64");
    LLVMValueRef fired =
        LLVMBuildICmp(builder, LLVMIntNE, prev_i64, LLVMConstInt(i64_ty, 0, 0),
                      "trig.has_fired");
    LLVMValueRef cur = LLVMBuildSelect(
        builder, LLVMBuildNot(builder, fired, "trig.first_fire"), one_f, zero_f,
        "trig.cur");
    LLVMBuildStore(builder, LLVMConstInt(i64_ty, 1, 0), cur_ptr);
    return cur;
  }

  LLVMValueRef phase_off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef phase_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &phase_off_val, 1, "trig.phase_ptr");
  LLVMValueRef phase = LLVMBuildLoad2(builder, f64_ty, phase_ptr, "trig.phase");
  LLVMValueRef spf = dsp_ctx->spf;
  LLVMValueRef step = LLVMBuildFMul(builder, freq, spf, "trig.step");
  LLVMValueRef advanced = LLVMBuildFAdd(builder, phase, step, "trig.advanced");

  LLVMValueRef phase_is_zero =
      LLVMBuildFCmp(builder, LLVMRealOEQ, phase, zero_f, "trig.phase_is_zero");

  LLVMValueRef ovf =
      LLVMBuildFCmp(builder, LLVMRealOGE, advanced, one_f, "trig.ovf");
  LLVMValueRef udf =
      LLVMBuildFCmp(builder, LLVMRealOLT, advanced, zero_f, "trig.udf");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, zero_f, advanced, "trig.wrap_ovf");
  next = LLVMBuildSelect(builder, udf, one_f, next, "trig.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  return LLVMBuildSelect(builder, phase_is_zero, one_f, zero_f, "trig.out");
}

static LLVMValueRef pow2_tabread(LLVMValueRef phasor, LLVMValueRef table_ptr,
                                 int32_t table_size, LLVMBuilderRef builder) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMValueRef scaled =
      LLVMBuildFMul(builder, phasor, LLVMConstReal(f64_ty, (double)table_size),
                    "tab.scaled_idx");
  LLVMValueRef index = LLVMBuildFPToSI(builder, scaled, i32_ty, "tab.index");
  LLVMValueRef index_f = LLVMBuildSIToFP(builder, index, f64_ty, "tab.index_f");
  LLVMValueRef frac = LLVMBuildFSub(builder, scaled, index_f, "tab.frac");

  LLVMValueRef mask = LLVMConstInt(i32_ty, table_size - 1, 0);
  LLVMValueRef one_i32 = LLVMConstInt(i32_ty, 1, 0);
  LLVMValueRef idx0 = LLVMBuildAnd(builder, index, mask, "tab.idx0");
  LLVMValueRef idx1 = LLVMBuildAnd(
      builder, LLVMBuildAdd(builder, index, one_i32, "tab.idx1_raw"), mask,
      "tab.idx1");

  LLVMValueRef idx0_i64 = LLVMBuildZExt(builder, idx0, i64_ty, "tab.idx0.i64");
  LLVMValueRef idx1_i64 = LLVMBuildZExt(builder, idx1, i64_ty, "tab.idx1.i64");

  LLVMValueRef a_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx0_i64, 1, "tab.a.ptr");
  LLVMValueRef b_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx1_i64, 1, "tab.b.ptr");
  LLVMValueRef a = LLVMBuildLoad2(builder, f64_ty, a_ptr, "tab.a");
  LLVMValueRef b = LLVMBuildLoad2(builder, f64_ty, b_ptr, "tab.b");

  LLVMValueRef inv_frac =
      LLVMBuildFSub(builder, LLVMConstReal(f64_ty, 1.0), frac, "tab.inv_frac");
  LLVMValueRef a_term = LLVMBuildFMul(builder, inv_frac, a, "tab.a_term");
  LLVMValueRef b_term = LLVMBuildFMul(builder, frac, b, "tab.b_term");
  return LLVMBuildFAdd(builder, a_term, b_term, "tab.sample");
}
LLVMValueRef ensure_float(Type *in_type, LLVMValueRef val,
                          LLVMBuilderRef builder) {

  if (types_equal(in_type, &t_int)) {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "freq.f64");
  }
  return val;
}

LLVMValueRef builtin_tab_osc(const char *tab_sym, int32_t tabsize, Ast *ast,
                             DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *in_type = ast->data.AST_APPLICATION.args->type;

  LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                     ctx, module, builder);
  freq = ensure_float(in_type, freq, builder);
  LLVMValueRef phasor = builtin_phasor(freq, dsp_ctx, ctx, module, builder);
  LLVMValueRef table_ptr =
      get_table_global_ptr(tab_sym, tabsize, module, builder);
  return pow2_tabread(phasor, table_ptr, tabsize, builder);
}

LLVMValueRef build_tabread(LLVMValueRef tab, LLVMValueRef phase,
                           DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);

  LLVMValueRef tab_struct = tab;
  if (LLVMGetTypeKind(LLVMTypeOf(tab)) == LLVMPointerTypeKind) {
    tab_struct = LLVMBuildLoad2(builder, arr_ty, tab, "tabread.arr");
  }

  LLVMValueRef len_i32 =
      LLVMBuildExtractValue(builder, tab_struct, 0, "tabread.len.i32");
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, tab_struct, 1, "tabread.data");
  LLVMValueRef len_i64 =
      LLVMBuildSExt(builder, len_i32, i64_ty, "tabread.len.i64");
  LLVMValueRef len_f =
      LLVMBuildSIToFP(builder, len_i32, f64_ty, "tabread.len.f64");

  LLVMValueRef phasor_f = phase;
  LLVMTypeKind phase_kind = LLVMGetTypeKind(LLVMTypeOf(phase));
  if (phase_kind == LLVMIntegerTypeKind) {
    phasor_f = LLVMBuildSIToFP(builder, phase, f64_ty, "tabread.phasor.f64");
  }

  LLVMValueRef idx_f =
      LLVMBuildFMul(builder, phasor_f, len_f, "tabread.scaled_idx");

  LLVMTypeRef floor_param_tys[] = {f64_ty};
  LLVMTypeRef floor_fn_ty = LLVMFunctionType(f64_ty, floor_param_tys, 1, 0);
  LLVMValueRef floor_fn = LLVMGetNamedFunction(module, "llvm.floor.f64");
  if (!floor_fn) {
    floor_fn = LLVMAddFunction(module, "llvm.floor.f64", floor_fn_ty);
    LLVMSetLinkage(floor_fn, LLVMExternalLinkage);
  }

  LLVMValueRef idx_over_len =
      LLVMBuildFDiv(builder, idx_f, len_f, "tabread.idx_over_len");
  LLVMValueRef wrap_q = LLVMBuildCall2(builder, floor_fn_ty, floor_fn,
                                       &idx_over_len, 1, "tabread.wrap_q");
  LLVMValueRef wrapped_idx = LLVMBuildFSub(
      builder, idx_f, LLVMBuildFMul(builder, len_f, wrap_q, "tabread.wrap_off"),
      "tabread.wrapped_idx");

  LLVMValueRef i0_f = LLVMBuildCall2(builder, floor_fn_ty, floor_fn,
                                     &wrapped_idx, 1, "tabread.i0f");
  LLVMValueRef frac = LLVMBuildFSub(builder, wrapped_idx, i0_f, "tabread.frac");
  LLVMValueRef i0 = LLVMBuildFPToSI(builder, i0_f, i64_ty, "tabread.i0");
  LLVMValueRef i1_raw =
      LLVMBuildAdd(builder, i0, LLVMConstInt(i64_ty, 1, 0), "tabread.i1_raw");

  LLVMValueRef i1_ge_len =
      LLVMBuildICmp(builder, LLVMIntSGE, i1_raw, len_i64, "tabread.i1_ge_len");
  LLVMValueRef i1 = LLVMBuildSelect(
      builder, i1_ge_len, LLVMConstInt(i64_ty, 0, 0), i1_raw, "tabread.i1");

  LLVMValueRef y0_ptr =
      LLVMBuildGEP2(builder, f64_ty, data_ptr, &i0, 1, "tabread.y0_ptr");
  LLVMValueRef y1_ptr =
      LLVMBuildGEP2(builder, f64_ty, data_ptr, &i1, 1, "tabread.y1_ptr");
  LLVMValueRef y0 = LLVMBuildLoad2(builder, f64_ty, y0_ptr, "tabread.y0");
  LLVMValueRef y1 = LLVMBuildLoad2(builder, f64_ty, y1_ptr, "tabread.y1");

  LLVMValueRef dy = LLVMBuildFSub(builder, y1, y0, "tabread.dy");
  return LLVMBuildFAdd(builder, y0,
                       LLVMBuildFMul(builder, frac, dy, "tabread.mix"),
                       "tabread.sample");
}

LLVMValueRef build_exp_decay(LLVMValueRef T, LLVMValueRef trig,
                             DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  (void)ctx;

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef prev_off_val = LLVMConstInt(i32_ty, (uint64_t)(off + 8), 0);
  if (dsp_ctx->ctor_state_ptr) {
    LLVMValueRef val_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->ctor_builder, i8_ty, dsp_ctx->ctor_state_ptr,
                      &off_val, 1, "exp_decay.val_init_ptr");
    LLVMValueRef val_init_ptr =
        LLVMBuildBitCast(dsp_ctx->ctor_builder, val_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.val_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->ctor_builder, LLVMConstReal(f64_ty, 0.0),
                   val_init_ptr);

    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->ctor_builder, i8_ty, dsp_ctx->ctor_state_ptr,
                      &prev_off_val, 1, "exp_decay.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr =
        LLVMBuildBitCast(dsp_ctx->ctor_builder, prev_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->ctor_builder, LLVMConstReal(f64_ty, 0.0),
                   prev_init_ptr);
  }

  LLVMValueRef val_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                          &off_val, 1, "exp_decay.val_ptr_i8");
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, val_ptr_i8, f64_ptr_ty, "exp_decay.val_ptr");
  LLVMValueRef prev_trig_ptr_i8 =
      LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr, &prev_off_val, 1,
                    "exp_decay.prev_trig_ptr_i8");
  LLVMValueRef prev_trig_ptr = LLVMBuildBitCast(
      builder, prev_trig_ptr_i8, f64_ptr_ty, "exp_decay.prev_trig_ptr");

  LLVMValueRef val = LLVMBuildLoad2(builder, f64_ty, val_ptr, "exp_decay.val");
  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_trig_ptr, "exp_decay.prev_trig");

  LLVMValueRef half = LLVMConstReal(f64_ty, 0.5);
  LLVMValueRef trig_hi =
      LLVMBuildFCmp(builder, LLVMRealOGE, trig, half, "exp_decay.trig_hi");
  LLVMValueRef prev_lo =
      LLVMBuildFCmp(builder, LLVMRealOLT, prev_trig, half, "exp_decay.prev_lo");
  LLVMValueRef rising =
      LLVMBuildAnd(builder, trig_hi, prev_lo, "exp_decay.rising");

  LLVMValueRef cur_val = LLVMBuildSelect(
      builder, rising, LLVMConstReal(f64_ty, 1.0), val, "exp_decay.cur");

  LLVMTypeRef mul_fn_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty}, 1, 0);
  LLVMValueRef mul_fn = LLVMGetNamedFunction(module, "exp_decay_multiplier");
  if (!mul_fn) {
    mul_fn = LLVMAddFunction(module, "exp_decay_multiplier", mul_fn_ty);
    LLVMSetLinkage(mul_fn, LLVMExternalLinkage);
  }
  LLVMValueRef mod =
      LLVMBuildCall2(builder, mul_fn_ty, mul_fn, (LLVMValueRef[]){T}, 1,
                     "exp_decay.multiplier");
  LLVMValueRef next = LLVMBuildFMul(builder, cur_val, mod, "exp_decay.next");

  LLVMBuildStore(builder, next, val_ptr);
  LLVMBuildStore(builder, trig, prev_trig_ptr);

  return cur_val;
}
LLVMValueRef call_registered_synth_in_audio_fn(Ast *ast, SynthRecord rec,
                                               DspBuildCtx *dsp_ctx,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,

                                               LLVMBuilderRef builder) {
  if (!rec.frame_fn) {
    fprintf(stderr, "audio_jit: missing frame_fn for registered synth\n");
    return LLVMConstReal(LLVMDoubleType(), 0.0);
  }

  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + rec.state_bytes;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();

  LLVMValueRef off_i32 = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef state_ptr =
      LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr, &off_i32, 1,
                    "reg_synth.state_ptr");

  LLVMValueRef frame_fn = rec.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);
  unsigned formal_count = LLVMCountParamTypes(frame_fn_ty);
  if (formal_count == 0) {
    fprintf(stderr, "audio_jit: malformed frame_fn signature\n");
    return LLVMConstReal(LLVMDoubleType(), 0.0);
  }

  LLVMTypeRef *formal_tys =
      formal_count ? alloca(sizeof(LLVMTypeRef) * formal_count) : NULL;
  LLVMValueRef *frame_args =
      formal_count ? alloca(sizeof(LLVMValueRef) * formal_count) : NULL;
  LLVMGetParamTypes(frame_fn_ty, formal_tys);

  frame_args[0] = state_ptr;

  int arg_count = ast->data.AST_APPLICATION.len;
  for (unsigned i = 1; i < formal_count; i++) {
    int arg_idx = (int)i - 1;
    if (arg_idx < arg_count) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + arg_idx;
      LLVMValueRef arg_val = dsp_build_expr(arg_ast, dsp_ctx, ctx, module, builder);
      if (LLVMGetTypeKind(formal_tys[i]) == LLVMDoubleTypeKind) {
        arg_val = ensure_float(arg_ast->type, arg_val, builder);
      }
      frame_args[i] = arg_val;
    } else {
      frame_args[i] = LLVMConstNull(formal_tys[i]);
    }
  }

  return LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args, formal_count,
                        "reg_synth.frame_call");
}

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    Ast *stmt = l->ast;
                    val = dsp_build_expr(stmt, dsp_ctx, ctx, module, builder);
                  }));
    // return val;
    return val;
  }
  case AST_LET: {
    Ast *binding = ast->data.AST_LET.binding;
    const char *chars = binding->data.AST_IDENTIFIER.value;
    int len = binding->data.AST_IDENTIFIER.length;
    Ast *expr = ast->data.AST_LET.expr;
    Ast *in_expr = ast->data.AST_LET.in_expr;
    JITLangCtx cont_ctx = *ctx;

    if (in_expr) {
      STACK_ALLOC_CTX_PUSH(inner_ctx, ctx)
      cont_ctx = inner_ctx;
    }
    if (expr->tag == AST_LAMBDA) {

      JITSymbol *sym =
          new_symbol(STYPE_AUDIO_JIT_INLINE_LAMBDA, expr->type, NULL, NULL);
      sym->symbol_data._USER_DEFINED_SYMBOL = expr;

      ht_set_hash((&cont_ctx)->frame->table, chars, hash_string(chars, len),
                  sym);

      if (in_expr) {

        LLVMValueRef e_val =
            dsp_build_expr(in_expr, dsp_ctx, &cont_ctx, module, builder);

        destroy_ctx(&cont_ctx);
        return e_val;
      }

      return NULL;
    }

    LLVMValueRef val =
        dsp_build_expr(expr, dsp_ctx, &cont_ctx, module, builder);

    if (!val) {
      fprintf(stderr, "Error: could not compute dsp val for binding\n");
      print_ast_err(ast);
      return NULL;
    }

    JITSymbol *sym =
        new_symbol(STYPE_LOCAL_VAR, expr->type, val, LLVMTypeOf(val));
    ht_set_hash((&cont_ctx)->frame->table, chars, hash_string(chars, len), sym);

    if (in_expr) {

      LLVMValueRef e_val =
          dsp_build_expr(in_expr, dsp_ctx, &cont_ctx, module, builder);

      destroy_ctx(&cont_ctx);
      return e_val;
    }

    destroy_ctx(&cont_ctx);
    return val;
  }
  case AST_IDENTIFIER: {
    JITSymbol *sym = lookup_id_ast(ast, ctx);
    if (sym && sym->type == (symbol_type)STYPE_AUDIO_JIT_SYNTH_INLET) {
      LLVMValueRef inlet_node = sym->val;
      LLVMTypeRef i64_ty = LLVMInt64Type();
      LLVMTypeRef f64_ty = LLVMDoubleType();
      LLVMTypeRef read_param_tys[] = {GENERIC_PTR, i64_ty};
      LLVMTypeRef read_fn_ty = LLVMFunctionType(f64_ty, read_param_tys, 2, 0);
      LLVMValueRef read_fn =
          LLVMGetNamedFunction(module, "ylc_read_inlet_node");
      if (!read_fn) {
        read_fn = LLVMAddFunction(module, "ylc_read_inlet_node", read_fn_ty);
        LLVMSetLinkage(read_fn, LLVMExternalLinkage);
      }

      LLVMValueRef frame_i64 =
          LLVMBuildSExt(builder, dsp_ctx->frame_idx, i64_ty, "frame_idx.i64");
      LLVMValueRef read_args[] = {inlet_node, frame_i64};
      return LLVMBuildCall2(builder, read_fn_ty, read_fn, read_args, 2,
                            "inlet.sample");
    }
    return codegen(ast, ctx, module, builder);
  }
  case AST_DOUBLE: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_INT: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_APPLICATION: {
    Ast *f = ast->data.AST_APPLICATION.function;

    if (strcmp(f->data.AST_IDENTIFIER.value, "sin_osc") == 0) {
      return builtin_tab_osc("ylc_sin_table", SIN_TABSIZE, ast, dsp_ctx, ctx,
                             module, builder);
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "sq_osc") == 0) {
      return builtin_tab_osc("ylc_sq_table", SQ_TABSIZE, ast, dsp_ctx, ctx,
                             module, builder);
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "saw_osc") == 0) {
      return builtin_tab_osc("ylc_saw_table", SAW_TABSIZE, ast, dsp_ctx, ctx,
                             module, builder);
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "phasor") == 0) {
      LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args,
                                         dsp_ctx, ctx, module, builder);
      freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
      return builtin_phasor(freq, dsp_ctx, ctx, module, builder);
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "phasor_sinc") == 0) {
      LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args,
                                         dsp_ctx, ctx, module, builder);
      freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);

      LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                         dsp_ctx, ctx, module, builder);
      return builtin_phasor_sinc(freq, trig, dsp_ctx, ctx, module, builder);
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "trig") == 0) {
      Ast *freq_ast = ast->data.AST_APPLICATION.args;
      LLVMValueRef freq =
          dsp_build_expr(freq_ast, dsp_ctx, ctx, module, builder);
      freq = ensure_float(freq_ast->type, freq, builder);

      bool freq_is_const_zero = ast_is_const_zero(freq_ast, ctx);

      return builtin_trig(freq, freq_is_const_zero, dsp_ctx, ctx, module,
                          builder);
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "+") == 0) {
      LLVMValueRef l =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef r =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);
      return LLVMBuildFAdd(builder, l, r, "signal.add");
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "-") == 0) {

      LLVMValueRef l =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef r =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);

      return LLVMBuildFSub(builder, l, r, "signal.sub");
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "*") == 0) {

      LLVMValueRef l =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef r =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);

      return LLVMBuildFMul(builder, l, r, "signal.mul");
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "/") == 0) {
      LLVMValueRef l =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef r =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);
      return LLVMBuildFDiv(builder, l, r, "signal.div");
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "%") == 0) {
      LLVMValueRef l =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef r =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);
      return LLVMBuildFRem(builder, l, r, "signal.fmod");
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "white") == 0) {

      LLVMTypeRef wn_ty = LLVMFunctionType(
          LLVMDoubleType(), (LLVMTypeRef[]){LLVMDoubleType(), LLVMDoubleType()},
          2, 0);
      LLVMValueRef wn_fn = LLVMGetNamedFunction(module, "rand_double_range");
      if (!wn_fn) {
        wn_fn = LLVMAddFunction(module, "rand_double_range", wn_ty);
        LLVMSetLinkage(wn_fn, LLVMExternalLinkage);
      }

      return LLVMBuildCall2(
          builder, wn_ty, wn_fn,
          (LLVMValueRef[]){LLVMConstReal(LLVMDoubleType(), -1.),
                           LLVMConstReal(LLVMDoubleType(), 1.)},
          2, "white_noise.sample");
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "tabread1") == 0) {
      LLVMValueRef table = dsp_build_expr(ast->data.AST_APPLICATION.args,
                                          dsp_ctx, ctx, module, builder);

      LLVMValueRef phase = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                          dsp_ctx, ctx, module, builder);
      return build_tabread(table, phase, dsp_ctx, ctx, module, builder);
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "array_set") == 0) {
      print_ast(ast);
      LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder);
      LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                          dsp_ctx, ctx, module, builder);
      LLVMValueRef value = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                          dsp_ctx, ctx, module, builder);

      set_array_element(
          builder, arr, index, value,
          type_to_llvm_type((ast->data.AST_APPLICATION.args + 2)->type, ctx,
                            module));
      return arr;
    }

    if (strcmp(f->data.AST_IDENTIFIER.value, "array_at") == 0) {

      LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder);
      LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                          dsp_ctx, ctx, module, builder);
      // print_type(ast->type);

      return get_array_element(builder, arr, index,
                               type_to_llvm_type(ast->type, ctx, module));
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "decay") == 0) {

      LLVMValueRef T = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder);
      T = ensure_float(ast->data.AST_APPLICATION.args->type, T, builder);

      LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                         dsp_ctx, ctx, module, builder);
      return build_exp_decay(T, trig, dsp_ctx, ctx, module, builder);
    }
    if (strcmp(f->data.AST_IDENTIFIER.value, "scale") == 0) {

      LLVMValueRef lo =
          ensure_float(ast->data.AST_APPLICATION.args->type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder),
                       builder);

      LLVMValueRef hi =
          ensure_float(ast->data.AST_APPLICATION.args[1].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder),
                       builder);

      LLVMValueRef v =
          ensure_float(ast->data.AST_APPLICATION.args[2].type,
                       dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                      dsp_ctx, ctx, module, builder),
                       builder);
      LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");
      LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
      return LLVMBuildFAdd(builder, lo, scaled, "scale.out");
    }

    JITSymbol *callable_sym =
        lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

    if (callable_sym && callable_sym->type == STYPE_AUDIO_JIT_SYM) {

      int synth_id = audio_sym_synth_id(callable_sym);
      SynthRecord rec = synth_registry_get(synth_id);
      printf("use pre-compiled audio func id %d %s\n", synth_id, rec.name);

      return call_registered_synth_in_audio_fn(ast, rec, dsp_ctx, ctx, module,
                                               builder);
    }

    if (callable_sym && callable_sym->type == STYPE_AUDIO_JIT_INLINE_LAMBDA) {
      Ast *lambda_ast = callable_sym->symbol_data._USER_DEFINED_SYMBOL;
      STACK_ALLOC_CTX_PUSH(lctx, ctx);
      Type *ltype = callable_sym->symbol_type;

      bool is_void_fn = is_void_func(ltype);

      if (!is_void_fn) {
        int idx = 0;
        for (AstList *p = lambda_ast->data.AST_LAMBDA.params; p;
             p = p->next, idx++) {
          Ast *param_ast = p->ast;
          Type *param_type = ltype->data.T_FN.from;

          LLVMValueRef arg_val =
              dsp_build_expr(ast->data.AST_APPLICATION.args + idx, dsp_ctx,
                             &lctx, module, builder);
          JITSymbol *sym =
              new_symbol(STYPE_LOCAL_VAR, param_type, arg_val,
                         type_to_llvm_type(param_type, &lctx, module));

          const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
          int id_len = param_ast->data.AST_IDENTIFIER.length;
          ht_set_hash(lctx.frame->table, id_chars,
                      hash_string(id_chars, id_len), sym);

          ltype = ltype->data.T_FN.to;
        }
      }

      LLVMValueRef res = dsp_build_expr(lambda_ast->data.AST_LAMBDA.body,
                                        dsp_ctx, &lctx, module, builder);

      destroy_ctx(&lctx);

      return res;
    }

    if (callable_sym) {
      LLVMValueRef callable = callable_sym->val;

      if (callable_sym->type == STYPE_LAZY_EXTERN_FUNCTION) {

        callable =
            instantiate_extern_fn_sym(callable_sym, ctx, module, builder);
      }
      // printf("application??a\n");
      // print_ast(ast);
      // print_type(ast->data.AST_APPLICATION.function->type);
      // LLVMDumpValue(callable);
      int args_len = ast->data.AST_APPLICATION.len;
      LLVMValueRef args[args_len];

      Type *f = callable_sym->symbol_type;
      print_type(f);
      print_ast(ast);

      for (int i = 0; i < args_len; i++) {
        args[i] = dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx,
                                 ctx, module, builder);
        Type *t = f->data.T_FN.from;

        if (types_equal(t, &t_num)) {
          args[i] = ensure_float((ast->data.AST_APPLICATION.args + i)->type,
                                 args[i], builder);
        }
        f = f->data.T_FN.to;
      }
      return LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable), callable,
                            args, args_len, "call.ylc-function");
    }

    return NULL;
    // return NULL;
  }

  case AST_ARRAY: {
    if (ast_is_const(ast, ctx)) {
      int len = ast->data.AST_LIST.len;
      int off = (dsp_ctx->state_offset + 7) & ~7;
      dsp_ctx->state_offset = off + (len * 8);

      LLVMTypeRef i8_ty = LLVMInt8Type();
      LLVMTypeRef i32_ty = LLVMInt32Type();
      LLVMTypeRef i64_ty = LLVMInt64Type();
      LLVMTypeRef f64_ty = LLVMDoubleType();
      LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
      LLVMTypeRef arr_ty =
          LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);

      LLVMValueRef off_i32 = LLVMConstInt(i32_ty, (uint64_t)off, 0);
      LLVMValueRef ctor_base_i8 =
          LLVMBuildGEP2(dsp_ctx->ctor_builder, i8_ty, dsp_ctx->ctor_state_ptr,
                        &off_i32, 1, "array.ctor.base");
      LLVMValueRef ctor_base = LLVMBuildBitCast(
          dsp_ctx->ctor_builder, ctor_base_i8, f64_ptr_ty, "array.ctor.data");

      for (int i = 0; i < len; i++) {
        Ast *item = ast->data.AST_LIST.items + i;
        LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
        LLVMValueRef elem_ptr =
            LLVMBuildGEP2(dsp_ctx->ctor_builder, f64_ty, ctor_base, &idx_i64, 1,
                          "array.init.ptr");
        LLVMValueRef elem = codegen(item, ctx, module, dsp_ctx->ctor_builder);
        elem = ensure_float(item->type, elem, dsp_ctx->ctor_builder);
        LLVMBuildStore(dsp_ctx->ctor_builder, elem, elem_ptr);
      }

      LLVMValueRef run_base_i8 = LLVMBuildGEP2(
          builder, i8_ty, dsp_ctx->state_ptr, &off_i32, 1, "array.base");
      LLVMValueRef run_base =
          LLVMBuildBitCast(builder, run_base_i8, f64_ptr_ty, "array.data");

      LLVMValueRef arr = LLVMGetUndef(arr_ty);
      arr = LLVMBuildInsertValue(builder, arr,
                                 LLVMConstInt(i32_ty, (uint64_t)len, 0), 0,
                                 "array.size");
      arr = LLVMBuildInsertValue(builder, arr, run_base, 1, "array.data");
      return arr;
    }

    return NULL;
  }

  case AST_LIST: {
    if (ast_is_const(ast, ctx)) {
      // allocate list in constructor and initialize it to the specified val
    }

    return NULL;
  }
  default: {
    return NULL;
  }
  }
}
// LLVMValueRef dsp_build_perform_fn(LLVMValueRef samp_fn, DspBuildCtx *dsp_ctx,
//                                   JITLangCtx *ctx, LLVMModuleRef module_ref,
//                                   LLVMBuilderRef builder) {}
// LLVMValueRef dsp_build_cons_fn(const char *name, Ast *lambda,
//                                LLVMValueRef perform_fn, DspBuildCtx *dsp_ctx,
//                                JITLangCtx *ctx, LLVMModuleRef module_ref,
//                                LLVMBuilderRef builder) {}

LLVMValueRef PlayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  LLVMValueRef node =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMTypeRef play_param_tys[] = {GENERIC_PTR};
  LLVMTypeRef play_fn_ty = LLVMFunctionType(GENERIC_PTR, play_param_tys, 1, 0);
  LLVMValueRef play_fn = LLVMGetNamedFunction(module, "play_node");
  if (!play_fn) {
    play_fn = LLVMAddFunction(module, "play_node", play_fn_ty);
    LLVMSetLinkage(play_fn, LLVMExternalLinkage);
  }

  return LLVMBuildCall2(builder, play_fn_ty, play_fn, &node, 1, "play_node");
}

static void register_builtin(ht *stack, const char *name,
                             BuiltinHandler handler) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = handler;
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
}

__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  init_synth_registry();

  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no JIT context at load time\n");
    return;
  }

  STYPE_AUDIO_JIT_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_BUILTIN_HANDLER = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_INLINE_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_INLINE_LAMBDA = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_SYNTH_INLET = REGISTERED_JIT_SYMBOL_TYPE++;

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "compile_audio_fn", CompileAudioFnHandler);
  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");

  register_builtin(stack, "play", PlayHandler);
}
