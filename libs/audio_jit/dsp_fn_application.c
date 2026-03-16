#include "../../engine/ctx.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/parse.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/type_ser.h"
#include "./audio_jit.h"
#include "./compile_synth.h"
#include <llvm-c/Types.h>

#include <string.h>
#define is_ident(f, name) strcmp(f->data.AST_IDENTIFIER.value, name) == 0

#define _EPSILON 0.0001
#define BUILD_ON_TRIG(builder, trig, f64_ty, label_prefix, ...)                \
  do {                                                                         \
    LLVMValueRef trig_hi__ =                                                   \
        LLVMBuildFCmp(builder, LLVMRealOGE, trig, LLVMConstReal(f64_ty, 0.5),  \
                      label_prefix ".trig_hi");                                \
    LLVMBasicBlockRef cur_bb__ = LLVMGetInsertBlock(builder);                  \
    LLVMValueRef fn__ = LLVMGetBasicBlockParent(cur_bb__);                     \
    LLVMBasicBlockRef then_bb__ =                                              \
        LLVMAppendBasicBlock(fn__, label_prefix ".trig");                      \
    LLVMBasicBlockRef else_bb__ =                                              \
        LLVMAppendBasicBlock(fn__, label_prefix ".cont");                      \
    LLVMBasicBlockRef merge_bb__ =                                             \
        LLVMAppendBasicBlock(fn__, label_prefix ".merge");                     \
    LLVMBuildCondBr(builder, trig_hi__, then_bb__, else_bb__);                 \
    LLVMPositionBuilderAtEnd(builder, then_bb__);                              \
    __VA_ARGS__;                                                               \
    LLVMBuildBr(builder, merge_bb__);                                          \
    LLVMPositionBuilderAtEnd(builder, else_bb__);                              \
    LLVMBuildBr(builder, merge_bb__);                                          \
    LLVMPositionBuilderAtEnd(builder, merge_bb__);                             \
  } while (0)

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

  // Ast *sym_id = ast->data.AST_APPLICATION.function;

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

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
    const double *table_data = NULL;
    if (strcmp(sym_name, "ylc_sin_table") == 0 && table_size == SIN_TABSIZE) {
      table_data = ylc_sin_table;
    } else if (strcmp(sym_name, "ylc_sq_table") == 0 &&
               table_size == SQ_TABSIZE) {
      table_data = ylc_sq_table;
    } else if (strcmp(sym_name, "ylc_saw_table") == 0 &&
               table_size == SAW_TABSIZE) {
      table_data = ylc_saw_table;
    }

    if (table_data) {
      LLVMValueRef *elems = malloc(sizeof(LLVMValueRef) * (size_t)table_size);
      if (elems) {
        for (int32_t i = 0; i < table_size; i++) {
          elems[i] = LLVMConstReal(f64_ty, table_data[i]);
        }
        LLVMValueRef init = LLVMConstArray(f64_ty, elems, (unsigned)table_size);
        LLVMSetInitializer(global, init);
        LLVMSetGlobalConstant(global, 1);
        LLVMSetLinkage(global, LLVMPrivateLinkage);
        free(elems);
      } else {
        LLVMSetLinkage(global, LLVMExternalLinkage);
      }
    } else {
      LLVMSetLinkage(global, LLVMExternalLinkage);
    }
  }

  LLVMValueRef zero = LLVMConstInt(i32_ty, 0, 0);
  LLVMValueRef idxs[] = {zero, zero};
  return LLVMBuildGEP2(builder, arr_ty, global, idxs, 2, "tab.base");
}

bool ast_is_const(Ast *ast, JITLangCtx *jit_ctx) {
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
  if (dsp_ctx->init_state_ptr) {
    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                      &prev_off_val, 1, "phasor.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, prev_init_ptr_i8, LLVMPointerType(f64_ty, 0),
        "phasor.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
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
  if (dsp_ctx->init_state_ptr) {
    LLVMValueRef val_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                      &off_val, 1, "exp_decay.val_init_ptr");
    LLVMValueRef val_init_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, val_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.val_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   val_init_ptr);

    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                      &prev_off_val, 1, "exp_decay.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, prev_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
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

LLVMValueRef build_adsr(LLVMValueRef attack, LLVMValueRef decay,
                        LLVMValueRef sustain, LLVMValueRef release,
                        LLVMValueRef trig, DspBuildCtx *dsp_ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 24;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef value_off = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef phase_off = LLVMConstInt(i32_ty, (uint64_t)(off + 8), 0);
  LLVMValueRef prev_off = LLVMConstInt(i32_ty, (uint64_t)(off + 16), 0);

  LLVMValueRef value_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                            &value_off, 1, "adsr.value_ptr_i8");
  LLVMValueRef phase_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                            &phase_off, 1, "adsr.phase_ptr_i8");
  LLVMValueRef prev_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                           &prev_off, 1, "adsr.prev_ptr_i8");

  LLVMValueRef value_ptr =
      LLVMBuildBitCast(builder, value_ptr_i8, f64_ptr_ty, "adsr.value_ptr");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_ptr_i8, f64_ptr_ty, "adsr.phase_ptr");
  LLVMValueRef prev_ptr =
      LLVMBuildBitCast(builder, prev_ptr_i8, f64_ptr_ty, "adsr.prev_ptr");

  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_ptr, "adsr.prev_trig");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){f64_ty,     f64_ty,     f64_ty, f64_ty, f64_ty,
                      f64_ty,     f64_ty,     f64_ptr_ty, f64_ptr_ty,
                      f64_ptr_ty},
      10, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "adsr_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "adsr_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  return LLVMBuildCall2(
      builder, fn_ty, fn,
      (LLVMValueRef[]){trig, prev_trig, attack, decay, sustain, release,
                       dsp_ctx->spf, value_ptr, phase_ptr, prev_ptr},
      10, "adsr.sample");
}

LLVMValueRef build_rect(LLVMValueRef duration, LLVMValueRef trig,
                        DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef rem_off = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef prev_off = LLVMConstInt(i32_ty, (uint64_t)(off + 8), 0);

  LLVMValueRef rem_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                          &rem_off, 1, "rect.rem_ptr_i8");
  LLVMValueRef prev_ptr_i8 = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                           &prev_off, 1, "rect.prev_ptr_i8");
  LLVMValueRef rem_ptr =
      LLVMBuildBitCast(builder, rem_ptr_i8, f64_ptr_ty, "rect.rem_ptr");
  LLVMValueRef prev_ptr =
      LLVMBuildBitCast(builder, prev_ptr_i8, f64_ptr_ty, "rect.prev_ptr");

  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_ptr, "rect.prev_trig");

  LLVMTypeRef fn_ty =
      LLVMFunctionType(f64_ty,
                       (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ty,
                                       f64_ptr_ty, f64_ptr_ty},
                       6, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "rect_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "rect_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  return LLVMBuildCall2(builder, fn_ty, fn,
                        (LLVMValueRef[]){duration, trig, prev_trig,
                                         dsp_ctx->spf, rem_ptr, prev_ptr},
                        6, "rect.sample");
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
  LLVMValueRef state_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &off_i32, 1, "reg_synth.state_ptr");
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr && rec.init_fn) {
    LLVMValueRef init_state_ptr =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                      &off_i32, 1, "reg_synth.init_state_ptr");
    LLVMTypeRef init_fn_ty = LLVMGlobalGetValueType(rec.init_fn);
    LLVMBuildCall2(dsp_ctx->init_builder, init_fn_ty, rec.init_fn,
                   (LLVMValueRef[]){init_state_ptr}, 1, "reg_synth.init_call");
  }

  LLVMValueRef frame_fn = rec.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);

  unsigned formal_count = LLVMCountParamTypes(frame_fn_ty);
  if (formal_count == 0) {
    fprintf(stderr, "audio_jit: malformed frame_fn signature\n");
    return LLVMConstReal(LLVMDoubleType(), 0.0);
  }

  LLVMTypeRef formal_tys[formal_count];
  LLVMValueRef frame_args[formal_count];
  LLVMGetParamTypes(frame_fn_ty, formal_tys);

  frame_args[0] = state_ptr;

  int arg_count = ast->data.AST_APPLICATION.len;
  for (unsigned i = 1; i < formal_count; i++) {
    int arg_idx = (int)i - 1;
    if (arg_idx < arg_count) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + arg_idx;
      LLVMValueRef arg_val =
          dsp_build_expr(arg_ast, dsp_ctx, ctx, module, builder);
      if (LLVMGetTypeKind(formal_tys[i]) == LLVMDoubleTypeKind) {
        arg_val = ensure_float(arg_ast->type, arg_val, builder);
      }
      frame_args[i] = arg_val;
    } else {
      frame_args[i] = LLVMConstNull(formal_tys[i]);
    }
  }

  return LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args,
                        formal_count, "reg_synth.frame_call");
}
LLVMValueRef build_lfnoise_lin(LLVMValueRef freq, LLVMValueRef lo,
                               LLVMValueRef hi, DspBuildCtx *dsp_ctx,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  // builtin_trig uses 8 bytes of state for phasor phase
  LLVMValueRef trig = builtin_trig(freq, false, dsp_ctx, ctx, module, builder);

  // State: cur_val(8) + slope(8) = 16 bytes
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();

  LLVMValueRef val_off = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef slp_off = LLVMConstInt(i32_ty, (uint64_t)(off + 8), 0);

  LLVMValueRef val_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                       &val_off, 1, "lfnoise1.val_ptr");
  LLVMValueRef slp_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                       &slp_off, 1, "lfnoise1.slp_ptr");

  LLVMValueRef cur_val =
      LLVMBuildLoad2(builder, f64_ty, val_ptr, "lfnoise1.cur_val");

  // LLVMValueRef lo = LLVMConstReal(f64_ty, 0.0);
  // LLVMValueRef hi = LLVMConstReal(f64_ty, 1.0);

  // Random function
  LLVMTypeRef rdr_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMValueRef rdr_fn = LLVMGetNamedFunction(module, "rand_double_range");
  if (!rdr_fn) {
    rdr_fn = LLVMAddFunction(module, "rand_double_range", rdr_ty);
    LLVMSetLinkage(rdr_fn, LLVMExternalLinkage);
  }
  BUILD_ON_TRIG(
      builder, trig, f64_ty, "lfnoise1",
      LLVMValueRef new_target =
          LLVMBuildCall2(builder, rdr_ty, rdr_fn, (LLVMValueRef[]){lo, hi}, 2,
                         "lfnoise1.new_target");
      // new_slope = (new_target - cur_val) * freq * spf
      // = distance to travel, normalized to per-sample steps over one period
      LLVMValueRef dist =
          LLVMBuildFSub(builder, new_target, cur_val, "lfnoise1.dist");
      LLVMValueRef freq_spf =
          LLVMBuildFMul(builder, freq, dsp_ctx->spf, "lfnoise1.freq_spf");
      LLVMValueRef new_slope =
          LLVMBuildFMul(builder, dist, freq_spf, "lfnoise1.new_slope");
      LLVMBuildStore(builder, new_slope, slp_ptr););

  LLVMValueRef slope =
      LLVMBuildLoad2(builder, f64_ty, slp_ptr, "lfnoise1.slope");

  // Advance cur_val by slope, store for next sample
  LLVMValueRef next_val =
      LLVMBuildFAdd(builder, cur_val, slope, "lfnoise1.next_val");
  LLVMBuildStore(builder, next_val, val_ptr);

  return cur_val;
}

LLVMValueRef build_lfnoise_step(LLVMValueRef freq, LLVMValueRef lo,
                                LLVMValueRef hi, DspBuildCtx *dsp_ctx,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  LLVMValueRef trig = builtin_trig(freq, false, dsp_ctx, ctx, module, builder);

  // State: cur_val(8)
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();

  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef val_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                       &off_val, 1, "lfnoise0.val_ptr");

  LLVMTypeRef rdr_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMValueRef rdr_fn = LLVMGetNamedFunction(module, "rand_double_range");
  if (!rdr_fn) {
    rdr_fn = LLVMAddFunction(module, "rand_double_range", rdr_ty);
    LLVMSetLinkage(rdr_fn, LLVMExternalLinkage);
  }
  BUILD_ON_TRIG(builder, trig, f64_ty, "lfnoise0",
                LLVMValueRef new_rand = LLVMBuildCall2(builder, rdr_ty, rdr_fn,
                                                       (LLVMValueRef[]){lo, hi},
                                                       2, "lfnoise0.new_rand");
                LLVMBuildStore(builder, new_rand, val_ptr););

  return LLVMBuildLoad2(builder, f64_ty, val_ptr, "lfnoise0.next_val");
}

LLVMValueRef build_kill_on_end(LLVMValueRef signal, DspBuildCtx *dsp_ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef void_ty = LLVMVoidType();

  // State: prev_val(8)
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef prev_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                        &off_val, 1, "kill_on_end.prev_ptr");
  LLVMValueRef prev_val =
      LLVMBuildLoad2(builder, f64_ty, prev_ptr, "kill_on_end.prev_val");

  LLVMValueRef eps = LLVMConstReal(f64_ty, _EPSILON);
  LLVMValueRef prev_above_eps =
      LLVMBuildFCmp(builder, LLVMRealOGT, prev_val, eps, "kill_on_end.prev_gt");
  LLVMValueRef cur_below_eps =
      LLVMBuildFCmp(builder, LLVMRealOLE, signal, eps, "kill_on_end.cur_le");
  LLVMValueRef node_nonnull =
      LLVMBuildICmp(builder, LLVMIntNE, dsp_ctx->node_ptr,
                    LLVMConstNull(GENERIC_PTR), "kill_on_end.node_nonnull");
  LLVMValueRef crossed =
      LLVMBuildAnd(builder, prev_above_eps, cur_below_eps, "kill_on_end.crossed");
  LLVMValueRef should_kill =
      LLVMBuildAnd(builder, crossed, node_nonnull, "kill_on_end.cond");

  LLVMTypeRef kill_fn_ty =
      LLVMFunctionType(void_ty, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
  LLVMValueRef kill_fn = LLVMGetNamedFunction(module, "ylc_kill_node");
  if (!kill_fn) {
    kill_fn = LLVMAddFunction(module, "ylc_kill_node", kill_fn_ty);
    LLVMSetLinkage(kill_fn, LLVMExternalLinkage);
  }

  LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef fn = LLVMGetBasicBlockParent(cur_bb);
  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlock(fn, "kill_on_end.then");
  LLVMBasicBlockRef else_bb = LLVMAppendBasicBlock(fn, "kill_on_end.else");
  LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(fn, "kill_on_end.merge");

  LLVMBuildCondBr(builder, should_kill, then_bb, else_bb);

  LLVMPositionBuilderAtEnd(builder, then_bb);
  LLVMBuildCall2(builder, kill_fn_ty, kill_fn,
                 (LLVMValueRef[]){dsp_ctx->node_ptr}, 1, "kill_on_end.call");
  LLVMBuildBr(builder, merge_bb);

  LLVMPositionBuilderAtEnd(builder, else_bb);
  LLVMBuildBr(builder, merge_bb);

  LLVMPositionBuilderAtEnd(builder, merge_bb);
  LLVMBuildStore(builder, signal, prev_ptr);
  return signal;
}
LLVMValueRef dsp_fn_application(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *f = ast->data.AST_APPLICATION.function;

  fprintf(stderr, "dsp_app: %s (scope=%d)\n", f->data.AST_IDENTIFIER.value,
          ctx->stack_ptr);

  if (strcmp(f->data.AST_IDENTIFIER.value, "spf") == 0) {
    return dsp_ctx->spf;
  }

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
    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder);
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
    return builtin_phasor(freq, dsp_ctx, ctx, module, builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "phasor_sinc") == 0) {
    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder);
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);

    LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                       dsp_ctx, ctx, module, builder);
    return builtin_phasor_sinc(freq, trig, dsp_ctx, ctx, module, builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "trig") == 0) {
    Ast *freq_ast = ast->data.AST_APPLICATION.args;
    LLVMValueRef freq = dsp_build_expr(freq_ast, dsp_ctx, ctx, module, builder);
    freq = ensure_float(freq_ast->type, freq, builder);

    bool freq_is_const_zero = ast_is_const_zero(freq_ast, ctx);

    return builtin_trig(freq, freq_is_const_zero, dsp_ctx, ctx, module,
                        builder);
  }
  if (strcmp(f->data.AST_IDENTIFIER.value, "+") == 0) {
    LLVMValueRef l = ensure_float(ast->data.AST_APPLICATION.args->type,
                                  dsp_build_expr(ast->data.AST_APPLICATION.args,
                                                 dsp_ctx, ctx, module, builder),
                                  builder);

    LLVMValueRef r =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    return LLVMBuildFAdd(builder, l, r, "signal.add");
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "-") == 0) {

    LLVMValueRef l = ensure_float(ast->data.AST_APPLICATION.args->type,
                                  dsp_build_expr(ast->data.AST_APPLICATION.args,
                                                 dsp_ctx, ctx, module, builder),
                                  builder);

    LLVMValueRef r =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);

    return LLVMBuildFSub(builder, l, r, "signal.sub");
  }
  if (strcmp(f->data.AST_IDENTIFIER.value, "*") == 0) {

    LLVMValueRef l = ensure_float(ast->data.AST_APPLICATION.args->type,
                                  dsp_build_expr(ast->data.AST_APPLICATION.args,
                                                 dsp_ctx, ctx, module, builder),
                                  builder);

    LLVMValueRef r =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    if (!l || !r) {
      fprintf(stderr, "audio_jit: null operand in '*' expression\n");
      print_ast(ast);
      return NULL;
    }

    return LLVMBuildFMul(builder, l, r, "signal.mul");
  }
  if (strcmp(f->data.AST_IDENTIFIER.value, "/") == 0) {
    LLVMValueRef l = ensure_float(ast->data.AST_APPLICATION.args->type,
                                  dsp_build_expr(ast->data.AST_APPLICATION.args,
                                                 dsp_ctx, ctx, module, builder),
                                  builder);

    LLVMValueRef r =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    return LLVMBuildFDiv(builder, l, r, "signal.div");
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "%") == 0) {
    LLVMValueRef l = ensure_float(ast->data.AST_APPLICATION.args->type,
                                  dsp_build_expr(ast->data.AST_APPLICATION.args,
                                                 dsp_ctx, ctx, module, builder),
                                  builder);

    LLVMValueRef r =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
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

    return LLVMBuildCall2(builder, wn_ty, wn_fn,
                          (LLVMValueRef[]){LLVMConstReal(LLVMDoubleType(), -1.),
                                           LLVMConstReal(LLVMDoubleType(), 1.)},
                          2, "white_noise.sample");
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "kill_on_end") == 0) {
    LLVMValueRef signal =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    return build_kill_on_end(signal, dsp_ctx, module, builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "adsr") == 0) {
    LLVMValueRef attack =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef decay =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef sustain =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef release =
        ensure_float(ast->data.AST_APPLICATION.args[3].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 3, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef trig =
        ensure_float(ast->data.AST_APPLICATION.args[4].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 4, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    return build_adsr(attack, decay, sustain, release, trig, dsp_ctx, module,
                      builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "rect") == 0) {
    LLVMValueRef duration =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef trig =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    return build_rect(duration, trig, dsp_ctx, module, builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "tabread1") == 0) {
    LLVMValueRef table = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder);

    LLVMValueRef phase = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                        dsp_ctx, ctx, module, builder);
    return build_tabread(table, phase, dsp_ctx, ctx, module, builder);
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "array_set") == 0) {
    print_ast(ast);
    // print_type((ast->data.AST_APPLICATION.args + 2)->type);

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
    // unipolar scale - ie [0,1] -> [a, b]
    LLVMValueRef lo =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder),
                     builder);

    LLVMValueRef hi =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);

    LLVMValueRef v =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");
    LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    return LLVMBuildFAdd(builder, lo, scaled, "scale.out");
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "scale_bp") == 0) {
    // bipolar scale - ie [-1,1] -> [a, b]

    LLVMValueRef lo =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder),
                     builder);

    LLVMValueRef hi =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder),
                     builder);

    LLVMValueRef v =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder),
                     builder);
    LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");

    v = LLVMBuildFMul(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
                      "input.half");

    v = LLVMBuildFAdd(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
                      "input.add_half");
    LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    return LLVMBuildFAdd(builder, lo, scaled, "scale.out");
  }

  if (is_ident(f, "lfnoise")) {
    // linearly interpolated noise [0, 1)
    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder);

    LLVMValueRef lo = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                     dsp_ctx, ctx, module, builder);

    LLVMValueRef hi = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                     dsp_ctx, ctx, module, builder);
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
    lo = ensure_float((ast->data.AST_APPLICATION.args + 1)->type, lo, builder);
    hi = ensure_float((ast->data.AST_APPLICATION.args + 2)->type, hi, builder);

    return build_lfnoise_lin(freq, lo, hi, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "lfnoise0")) {

    // step noise [0, 1)

    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder);

    LLVMValueRef lo = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                     dsp_ctx, ctx, module, builder);

    LLVMValueRef hi = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                     dsp_ctx, ctx, module, builder);
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
    lo = ensure_float((ast->data.AST_APPLICATION.args + 1)->type, lo, builder);
    hi = ensure_float((ast->data.AST_APPLICATION.args + 2)->type, hi, builder);

    return build_lfnoise_step(freq, lo, hi, dsp_ctx, ctx, module, builder);
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
            dsp_build_expr(ast->data.AST_APPLICATION.args + idx, dsp_ctx, &lctx,
                           module, builder);
        JITSymbol *sym =
            new_symbol(STYPE_LOCAL_VAR, param_type, arg_val,
                       type_to_llvm_type(param_type, &lctx, module));

        const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
        int id_len = param_ast->data.AST_IDENTIFIER.length;
        ht_set_hash(lctx.frame->table, id_chars, hash_string(id_chars, id_len),
                    sym);

        ltype = ltype->data.T_FN.to;
      }
    }

    LLVMValueRef res = dsp_build_expr(lambda_ast->data.AST_LAMBDA.body, dsp_ctx,
                                      &lctx, module, builder);

    destroy_ctx(&lctx);

    return res;
  }

  if (callable_sym) {
    LLVMValueRef callable = callable_sym->val;

    if (callable_sym->type == STYPE_LAZY_EXTERN_FUNCTION) {

      callable = instantiate_extern_fn_sym(callable_sym, ctx, module, builder);
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
      args[i] = dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx, ctx,
                               module, builder);
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
}
