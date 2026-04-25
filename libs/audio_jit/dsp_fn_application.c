#include "../../engine/ctx.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/parse.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./audio_jit.h"
#include "./common.h"
#include "./compile_synth.h"
#include "./dsp_array_proc.h"
#include "./dsp_build_expr.h"
#include "./dsp_fork.h"
#include "./dsp_spectral.h"
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define _EPSILON 0.0001
#define BUILD_ON_TRIG(builder, trig, label_prefix, ...)                        \
  do {                                                                         \
    LLVMTypeRef f64_ty = LLVMDoubleType();                                     \
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
  // TODO: HACK
  double spf = 1. / 48000.;
  if (T <= 0.0 || spf <= 0.0) {
    return 0.0;
  }
  return pow(epsilon, spf / T);
}

int ylc_rand_int_range_i32(int lo, int hi) {
  if (hi <= lo) {
    return lo;
  }
  int span = hi - lo;
  return lo + (rand() % span);
}

// Correctly defined _matrix_vec_mul implementation
void dsp_matrix_vec_mul(int rows, int cols, double *matrix_data,
                        double *vector_data, double *out_data) {
  for (int i = 0; i < rows; i++) {
    out_data[i] = 0.0;
    for (int j = 0; j < cols; j++) {
      out_data[i] += matrix_data[i * cols + j] * vector_data[j];
    }
  }
}
double dsp_vec_dot(int cols, double *a, double *b) {
  double res = 0.;
  for (int i = 0; i < cols; i++) {
    res += a[i] * b[i];
  }
  return res;
}

static LLVMValueRef SinOscHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module_ref,
                                  LLVMBuilderRef builder) {

  // Ast *sym_id = ast->data.AST_APPLICATION.function;

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

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

#ifndef GRAIN_WINDOW_TABSIZE
#define GRAIN_WINDOW_TABSIZE (1 << 9)
#endif
static const double ylc_grain_win[GRAIN_WINDOW_TABSIZE] = {
#include "../../engine/assets/grain_win.csv"
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

static bool ast_try_eval_const_num(Ast *ast, DspBuildCtx *dsp_ctx,
                                   JITLangCtx *jit_ctx, double *out) {
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
  case AST_IDENTIFIER: {
    const char *name = ast->data.AST_IDENTIFIER.value;
    if (dsp_ctx && name) {
      if (strcmp(name, "sample_rate") == 0) {
        *out = (double)dsp_ctx->sample_rate;
        return true;
      }
      if (strcmp(name, "spf") == 0) {
        *out = 1. / 48000;
        return true;
      }
    }
    // // Look up let-bound constants in the JIT scope
    // if (jit_ctx) {
    //   JITSymbol *sym = lookup_id_ast(ast, jit_ctx);
    //   if (sym && sym->val) {
    //     if (LLVMIsAConstantInt(sym->val)) {
    //       *out = (double)LLVMConstIntGetSExtValue(sym->val);
    //       return true;
    //     }
    //     if (LLVMIsAConstantFP(sym->val)) {
    //       LLVMBool loses_info;
    //       *out = LLVMConstRealGetDouble(sym->val, &loses_info);
    //       return true;
    //     }
    //   }
    // }
    return false;
  }
  case AST_APPLICATION: {
    Ast *fn = ast->data.AST_APPLICATION.function;
    Ast *args = ast->data.AST_APPLICATION.args;
    if (fn && fn->tag == AST_IDENTIFIER && ast->data.AST_APPLICATION.len == 1) {
      const char *name = fn->data.AST_IDENTIFIER.value;
      if (dsp_ctx && name) {
        if (strcmp(name, "sample_rate") == 0) {
          *out = (double)dsp_ctx->sample_rate;
          return true;
        }
        if (strcmp(name, "spf") == 0) {
          *out = 1. / 48000;
          return true;
        }
      }
      return false;
    }
    const char *name = NULL;
    double a = 0.0;
    double b = 0.0;

    if (fn && fn->tag == AST_IDENTIFIER && ast->data.AST_APPLICATION.len == 2) {

      if (!ast_try_eval_const_num(args, dsp_ctx, jit_ctx, &a) ||
          !ast_try_eval_const_num(args + 1, dsp_ctx, jit_ctx, &b)) {
        return false;
      }

      name = fn->data.AST_IDENTIFIER.value;

    } else {
      return false;
    }

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

static bool ast_is_const_zero(Ast *ast, DspBuildCtx *dsp_ctx,
                              JITLangCtx *jit_ctx) {
  double value = 1.0;
  return ast_try_eval_const_num(ast, dsp_ctx, jit_ctx, &value) && value == 0.0;
}

LLVMValueRef dsp_consume_state_cursor(LLVMValueRef cursor_ptr,
                                      LLVMBuilderRef builder, int size,
                                      int align, const char *name) {
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
  LLVMTypeRef i64_ty = LLVMInt64Type();

  LLVMValueRef cur =
      LLVMBuildLoad2(builder, i8_ptr_ty, cursor_ptr, "state.cur");
  LLVMValueRef cur_i64 =
      LLVMBuildPtrToInt(builder, cur, i64_ty, "state.cur.i64");

  LLVMValueRef aligned_i64 = cur_i64;
  if (align > 1) {
    LLVMValueRef addend = LLVMConstInt(i64_ty, (uint64_t)(align - 1), 0);
    LLVMValueRef mask = LLVMConstInt(i64_ty, ~(uint64_t)(align - 1), 0);
    aligned_i64 = LLVMBuildAnd(
        builder, LLVMBuildAdd(builder, cur_i64, addend, "state.align.add"),
        mask, "state.align.mask");
  }

  LLVMValueRef aligned =
      LLVMBuildIntToPtr(builder, aligned_i64, i8_ptr_ty, name);
  LLVMValueRef next =
      LLVMBuildGEP2(builder, i8_ty, aligned,
                    (LLVMValueRef[]){LLVMConstInt(i64_ty, (uint64_t)size, 0)},
                    1, "state.next");
  LLVMBuildStore(builder, next, cursor_ptr);
  return aligned;
}

LLVMValueRef dsp_consume_frame_state(DspBuildCtx *dsp_ctx,
                                     LLVMBuilderRef builder, int size,
                                     int align, const char *name) {
  if (!dsp_ctx->state_cursor_ptr) {
    return NULL;
  }
  return dsp_consume_state_cursor(dsp_ctx->state_cursor_ptr, builder, size,
                                  align, name);
}

LLVMValueRef dsp_consume_init_state(DspBuildCtx *dsp_ctx,
                                    LLVMBuilderRef builder, int size, int align,
                                    const char *name) {
  if (!dsp_ctx->init_state_cursor_ptr) {
    return NULL;
  }
  return dsp_consume_state_cursor(dsp_ctx->init_state_cursor_ptr, builder, size,
                                  align, name);
}

LLVMValueRef builtin_phasor(LLVMValueRef freq, DspBuildCtx *dsp_ctx,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)off;
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 8, 8, "phasor.init.base");
    LLVMValueRef init_phase_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_base, f64_ptr_ty, "phasor.init.phase_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_phase_ptr);
  }

  LLVMValueRef phase_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "phasor.phase_base");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_base, f64_ptr_ty, "phasor.phase_ptr");
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
  LLVMValueRef wrapped_ovf =
      LLVMBuildFSub(builder, advanced, one, "phasor.wrap_ovf_val");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, wrapped_ovf, advanced, "phasor.wrap_ovf");
  LLVMValueRef wrapped_udf =
      LLVMBuildFAdd(builder, next, one, "phasor.wrap_udf_val");
  next = LLVMBuildSelect(builder, udf, wrapped_udf, next, "phasor.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  return phase;
}

LLVMValueRef builtin_phasor_sinc(LLVMValueRef freq, LLVMValueRef trig,
                                 DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 16;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)off;
  if (dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 16, 8, "phasor.init.base");
    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_base,
                      (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
                      "phasor.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, prev_init_ptr_i8, f64_ptr_ty,
                         "phasor.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   prev_init_ptr);
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 16, 8, "phasor.base");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "phasor.phase_ptr");
  LLVMValueRef prev_trig_ptr_i8 = LLVMBuildGEP2(
      builder, i8_ty, base, (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
      "phasor.prev_trig_ptr_i8");
  LLVMValueRef prev_trig_ptr = LLVMBuildBitCast(
      builder, prev_trig_ptr_i8, f64_ptr_ty, "phasor.prev_trig_ptr");
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
  LLVMValueRef wrapped_ovf =
      LLVMBuildFSub(builder, advanced, one, "phasor.wrap_ovf_val");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, wrapped_ovf, advanced, "phasor.wrap_ovf");
  LLVMValueRef wrapped_udf =
      LLVMBuildFAdd(builder, next, one, "phasor.wrap_udf_val");
  next = LLVMBuildSelect(builder, udf, wrapped_udf, next, "phasor.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  LLVMBuildStore(builder, trig, prev_trig_ptr);

  return cur_phase;
}

LLVMValueRef builtin_trig(LLVMValueRef freq, bool freq_is_const_zero,
                          DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i64_ptr_ty = LLVMPointerType(i64_ty, 0);
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMValueRef zero_f = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one_f = LLVMConstReal(f64_ty, 1.0);

  if (freq_is_const_zero) {
    if (dsp_ctx->init_state_ptr) {
      (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                                   "trig.init.latch");
    }
    LLVMValueRef cur_base =
        dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "trig.cur_base");
    LLVMValueRef cur_ptr =
        LLVMBuildBitCast(builder, cur_base, i64_ptr_ty, "trig.cur_ptr");
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

  if (dsp_ctx->init_state_ptr) {
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                                 "trig.init.phase");
  }
  LLVMValueRef phase_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "trig.phase_base");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_base, f64_ptr_ty, "trig.phase_ptr");
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

static LLVMValueRef get_floor_intrinsic(LLVMModuleRef module) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef floor_fn_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty}, 1, 0);
  LLVMValueRef floor_fn = LLVMGetNamedFunction(module, "llvm.floor.f64");
  if (!floor_fn) {
    floor_fn = LLVMAddFunction(module, "llvm.floor.f64", floor_fn_ty);
    LLVMSetLinkage(floor_fn, LLVMExternalLinkage);
  }
  return floor_fn;
}

static LLVMValueRef build_wrap_unit_phase(LLVMValueRef phase,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder,
                                          const char *name) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef floor_fn_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty}, 1, 0);
  LLVMValueRef floor_fn = get_floor_intrinsic(module);
  LLVMValueRef floored =
      LLVMBuildCall2(builder, floor_fn_ty, floor_fn, &phase, 1, "pm.floor");
  return LLVMBuildFSub(builder, phase, floored, name);
}

static LLVMValueRef build_pow2_cubic_tabread(LLVMValueRef phase,
                                             LLVMValueRef table_ptr,
                                             int32_t table_size,
                                             LLVMBuilderRef builder,
                                             const char *prefix) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();

  LLVMValueRef scaled =
      LLVMBuildFMul(builder, phase, LLVMConstReal(f64_ty, (double)table_size),
                    "pm.tab.scaled");
  LLVMValueRef index = LLVMBuildFPToSI(builder, scaled, i32_ty, "pm.tab.index");
  LLVMValueRef index_f =
      LLVMBuildSIToFP(builder, index, f64_ty, "pm.tab.index_f");
  LLVMValueRef frac = LLVMBuildFSub(builder, scaled, index_f, "pm.tab.frac");

  LLVMValueRef mask = LLVMConstInt(i32_ty, (uint64_t)(table_size - 1), 0);
  LLVMValueRef one = LLVMConstInt(i32_ty, 1, 0);
  LLVMValueRef two = LLVMConstInt(i32_ty, 2, 0);

  LLVMValueRef idx1 = LLVMBuildAnd(builder, index, mask, "pm.tab.idx1");
  LLVMValueRef idx0 = LLVMBuildAnd(
      builder, LLVMBuildSub(builder, index, one, "pm.tab.idx0_raw"), mask,
      "pm.tab.idx0");
  LLVMValueRef idx2 = LLVMBuildAnd(
      builder, LLVMBuildAdd(builder, index, one, "pm.tab.idx2_raw"), mask,
      "pm.tab.idx2");
  LLVMValueRef idx3 = LLVMBuildAnd(
      builder, LLVMBuildAdd(builder, index, two, "pm.tab.idx3_raw"), mask,
      "pm.tab.idx3");

  LLVMValueRef idx0_i64 =
      LLVMBuildZExt(builder, idx0, i64_ty, "pm.tab.idx0.64");
  LLVMValueRef idx1_i64 =
      LLVMBuildZExt(builder, idx1, i64_ty, "pm.tab.idx1.64");
  LLVMValueRef idx2_i64 =
      LLVMBuildZExt(builder, idx2, i64_ty, "pm.tab.idx2.64");
  LLVMValueRef idx3_i64 =
      LLVMBuildZExt(builder, idx3, i64_ty, "pm.tab.idx3.64");

  LLVMValueRef y0_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx0_i64, 1, "pm.tab.y0_ptr");
  LLVMValueRef y1_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx1_i64, 1, "pm.tab.y1_ptr");
  LLVMValueRef y2_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx2_i64, 1, "pm.tab.y2_ptr");
  LLVMValueRef y3_ptr =
      LLVMBuildGEP2(builder, f64_ty, table_ptr, &idx3_i64, 1, "pm.tab.y3_ptr");

  LLVMValueRef y0 = LLVMBuildLoad2(builder, f64_ty, y0_ptr, "pm.tab.y0");
  LLVMValueRef y1 = LLVMBuildLoad2(builder, f64_ty, y1_ptr, "pm.tab.y1");
  LLVMValueRef y2 = LLVMBuildLoad2(builder, f64_ty, y2_ptr, "pm.tab.y2");
  LLVMValueRef y3 = LLVMBuildLoad2(builder, f64_ty, y3_ptr, "pm.tab.y3");

  LLVMValueRef half = LLVMConstReal(f64_ty, 0.5);
  LLVMValueRef one_half = LLVMConstReal(f64_ty, 1.5);
  LLVMValueRef two_f = LLVMConstReal(f64_ty, 2.0);
  LLVMValueRef two_half = LLVMConstReal(f64_ty, 2.5);

  LLVMValueRef c0 = y1;
  LLVMValueRef c1 = LLVMBuildFMul(
      builder, half, LLVMBuildFSub(builder, y2, y0, "pm.tab.c1.diff"),
      "pm.tab.c1");
  LLVMValueRef c2 = LLVMBuildFSub(
      builder,
      LLVMBuildFAdd(
          builder,
          LLVMBuildFSub(builder, y0,
                        LLVMBuildFMul(builder, two_half, y1, "pm.tab.c2.2p5y1"),
                        "pm.tab.c2.left"),
          LLVMBuildFMul(builder, two_f, y2, "pm.tab.c2.2y2"), "pm.tab.c2.mid"),
      LLVMBuildFMul(builder, half, y3, "pm.tab.c2.0p5y3"), "pm.tab.c2");
  LLVMValueRef c3 = LLVMBuildFAdd(
      builder,
      LLVMBuildFMul(builder, half,
                    LLVMBuildFSub(builder, y3, y0, "pm.tab.c3.diff0"),
                    "pm.tab.c3.a"),
      LLVMBuildFMul(builder, one_half,
                    LLVMBuildFSub(builder, y1, y2, "pm.tab.c3.diff1"),
                    "pm.tab.c3.b"),
      "pm.tab.c3");

  LLVMValueRef acc =
      LLVMBuildFAdd(builder, LLVMBuildFMul(builder, c3, frac, "pm.tab.poly0"),
                    c2, "pm.tab.poly1");
  acc =
      LLVMBuildFAdd(builder, LLVMBuildFMul(builder, acc, frac, "pm.tab.poly2"),
                    c1, "pm.tab.poly3");
  acc = LLVMBuildFAdd(
      builder, LLVMBuildFMul(builder, acc, frac, "pm.tab.poly4"), c0, prefix);
  return acc;
}

static LLVMValueRef build_pm_osc(LLVMValueRef carrier_freq,
                                 LLVMValueRef mod_index, LLVMValueRef mod_ratio,
                                 DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 16, 8, "pm.init.base");
    LLVMValueRef init_carrier_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_base, f64_ptr_ty, "pm.init.carrier_ptr");
    LLVMValueRef init_mod_ptr_i8 = LLVMBuildGEP2(
        dsp_ctx->init_builder, i8_ty, init_base,
        (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1, "pm.init.mod_ptr_i8");
    LLVMValueRef init_mod_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_mod_ptr_i8, f64_ptr_ty, "pm.init.mod_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_carrier_ptr);
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_mod_ptr);
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 16, 8, "pm.base");
  LLVMValueRef carrier_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "pm.carrier_ptr");
  LLVMValueRef mod_ptr_i8 = LLVMBuildGEP2(
      builder, i8_ty, base, (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
      "pm.mod_ptr_i8");
  LLVMValueRef mod_ptr =
      LLVMBuildBitCast(builder, mod_ptr_i8, f64_ptr_ty, "pm.mod_ptr");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ty, f64_ptr_ty, f64_ptr_ty},
      6, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "pm_osc_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "pm_osc_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  LLVMValueRef call_args[] = {carrier_freq, mod_index,   mod_ratio,
                              dsp_ctx->spf, carrier_ptr, mod_ptr};
  return LLVMBuildCall2(builder, fn_ty, fn, call_args, 6, "pm.sample");
}

static LLVMValueRef build_phase_osc(LLVMValueRef phase_input,
                                    LLVMValueRef carrier_freq,
                                    DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 8;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 8, 8, "phase_osc.init.base");
    LLVMValueRef init_carrier_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_base, f64_ptr_ty,
                         "phase_osc.init.carrier_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_carrier_ptr);
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "phase_osc.base");
  LLVMValueRef carrier_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "phase_osc.carrier_ptr");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty, (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ptr_ty}, 4, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "phase_osc_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "phase_osc_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  LLVMValueRef call_args[] = {phase_input, carrier_freq, dsp_ctx->spf,
                              carrier_ptr};
  return LLVMBuildCall2(builder, fn_ty, fn, call_args, 4, "phase_osc.sample");
}

static LLVMValueRef build_bank_osc(LLVMValueRef amps, LLVMValueRef phases,
                                   LLVMValueRef freq, DspBuildCtx *dsp_ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 8;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 8, 8, "bank_osc.init.base");
    LLVMValueRef init_phase_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_base, f64_ptr_ty,
                         "bank_osc.init.phase_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_phase_ptr);
  }

  LLVMValueRef phase_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "bank_osc.phase_base");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_base, f64_ptr_ty, "bank_osc.phase_ptr");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty, (LLVMTypeRef[]){arr_ty, arr_ty, f64_ty, f64_ty, f64_ptr_ty}, 5,
      0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "bank_osc_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "bank_osc_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  LLVMValueRef call_args[] = {amps, phases, freq, dsp_ctx->spf, phase_ptr};
  return LLVMBuildCall2(builder, fn_ty, fn, call_args, 5, "bank_osc.sample");
}
LLVMValueRef ensure_float(Type *in_type, LLVMValueRef val,
                          LLVMBuilderRef builder) {
  if (!val) {
    return val;
  }
  // Guard on the actual LLVM type, not just the YLC type: the explicit
  // frame_ty for fold callbacks forces the index param to i32 regardless of
  // what type inference said, so we must check both.
  if (LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMIntegerTypeKind) {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "i2f");
  }
  if (types_equal(in_type, &t_int)) {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "freq.f64");
  }
  return val;
}

LLVMValueRef _builtin_tab_osc(const char *tab_sym, int32_t tabsize,
                              LLVMValueRef freq, DspBuildCtx *dsp_ctx,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  LLVMValueRef phasor = builtin_phasor(freq, dsp_ctx, ctx, module, builder);
  LLVMValueRef table_ptr =
      get_table_global_ptr(tab_sym, tabsize, module, builder);
  return pow2_tabread(phasor, table_ptr, tabsize, builder);
}

LLVMValueRef builtin_tab_osc(const char *tab_sym, int32_t tabsize, Ast *ast,
                             DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *in_type = ast->data.AST_APPLICATION.args->type;

  LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                     ctx, module, builder)
                          .scalar;
  freq = ensure_float(in_type, freq, builder);
  LLVMValueRef phasor = builtin_phasor(freq, dsp_ctx, ctx, module, builder);
  LLVMValueRef table_ptr =
      get_table_global_ptr(tab_sym, tabsize, module, builder);
  return pow2_tabread(phasor, table_ptr, tabsize, builder);
}

static LLVMValueRef build_tabread_core(LLVMValueRef tab, LLVMValueRef phase,
                                       bool phase_is_normalized,
                                       DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

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
      phase_is_normalized
          ? LLVMBuildFMul(builder, phasor_f, len_f, "tabread.scaled_idx")
          : phasor_f;

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

LLVMValueRef build_tabread(LLVMValueRef tab, LLVMValueRef phase,
                           DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder) {
  return build_tabread_core(tab, phase, true, dsp_ctx, ctx, module, builder);
}

LLVMValueRef build_tabread_samp(LLVMValueRef tab, LLVMValueRef phase,
                                DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  return build_tabread_core(tab, phase, false, dsp_ctx, ctx, module, builder);
}

static LLVMValueRef build_interleaved_tabread_channel(
    LLVMValueRef buf_struct, LLVMValueRef phase, int num_channels,
    int channel_idx, LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef len_i32 =
      LLVMBuildExtractValue(builder, buf_struct, 0, "mbufplay.len.i32");
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, buf_struct, 1, "mbufplay.data");
  LLVMValueRef frame_len_i64 =
      LLVMBuildSExt(builder, len_i32, i64_ty, "mbufplay.frame_len.i64");
  LLVMValueRef frame_len_f =
      LLVMBuildSIToFP(builder, len_i32, f64_ty, "mbufplay.frame_len.f64");

  LLVMValueRef idx_f =
      LLVMBuildFMul(builder, phase, frame_len_f, "mbufplay.scaled_idx");

  LLVMTypeRef floor_param_tys[] = {f64_ty};
  LLVMTypeRef floor_fn_ty = LLVMFunctionType(f64_ty, floor_param_tys, 1, 0);
  LLVMValueRef floor_fn = LLVMGetNamedFunction(module, "llvm.floor.f64");
  if (!floor_fn) {
    floor_fn = LLVMAddFunction(module, "llvm.floor.f64", floor_fn_ty);
    LLVMSetLinkage(floor_fn, LLVMExternalLinkage);
  }

  LLVMValueRef idx_over_len =
      LLVMBuildFDiv(builder, idx_f, frame_len_f, "mbufplay.idx_over_len");
  LLVMValueRef wrap_q = LLVMBuildCall2(builder, floor_fn_ty, floor_fn,
                                       &idx_over_len, 1, "mbufplay.wrap_q");
  LLVMValueRef wrapped_idx = LLVMBuildFSub(
      builder, idx_f,
      LLVMBuildFMul(builder, frame_len_f, wrap_q, "mbufplay.wrap_off"),
      "mbufplay.wrapped_idx");

  LLVMValueRef i0_f = LLVMBuildCall2(builder, floor_fn_ty, floor_fn,
                                     &wrapped_idx, 1, "mbufplay.i0f");
  LLVMValueRef frac =
      LLVMBuildFSub(builder, wrapped_idx, i0_f, "mbufplay.frac");
  LLVMValueRef i0 = LLVMBuildFPToSI(builder, i0_f, i64_ty, "mbufplay.i0");
  LLVMValueRef i1_raw =
      LLVMBuildAdd(builder, i0, LLVMConstInt(i64_ty, 1, 0), "mbufplay.i1_raw");
  LLVMValueRef i1_ge_len = LLVMBuildICmp(builder, LLVMIntSGE, i1_raw,
                                         frame_len_i64, "mbufplay.i1_ge_len");
  LLVMValueRef i1 = LLVMBuildSelect(
      builder, i1_ge_len, LLVMConstInt(i64_ty, 0, 0), i1_raw, "mbufplay.i1");

  LLVMValueRef nch_i64 = LLVMConstInt(i64_ty, (uint64_t)num_channels, 0);
  LLVMValueRef ch_i64 = LLVMConstInt(i64_ty, (uint64_t)channel_idx, 0);
  LLVMValueRef y0_idx =
      LLVMBuildAdd(builder, LLVMBuildMul(builder, i0, nch_i64, "mbufplay.y0f"),
                   ch_i64, "mbufplay.y0_idx");
  LLVMValueRef y1_idx =
      LLVMBuildAdd(builder, LLVMBuildMul(builder, i1, nch_i64, "mbufplay.y1f"),
                   ch_i64, "mbufplay.y1_idx");
  LLVMValueRef y0_ptr =
      LLVMBuildGEP2(builder, f64_ty, data_ptr, &y0_idx, 1, "mbufplay.y0_ptr");
  LLVMValueRef y1_ptr =
      LLVMBuildGEP2(builder, f64_ty, data_ptr, &y1_idx, 1, "mbufplay.y1_ptr");
  LLVMValueRef y0 = LLVMBuildLoad2(builder, f64_ty, y0_ptr, "mbufplay.y0");
  LLVMValueRef y1 = LLVMBuildLoad2(builder, f64_ty, y1_ptr, "mbufplay.y1");
  LLVMValueRef dy = LLVMBuildFSub(builder, y1, y0, "mbufplay.dy");
  return LLVMBuildFAdd(builder, y0,
                       LLVMBuildFMul(builder, frac, dy, "mbufplay.mix"),
                       "mbufplay.sample");
}

static DspValue build_bufplay(LLVMValueRef buf, LLVMValueRef rate,
                              LLVMValueRef start_pos, LLVMValueRef trig,
                              int num_channels, DspBuildCtx *dsp_ctx,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);

  LLVMValueRef buf_struct = buf;
  if (LLVMGetTypeKind(LLVMTypeOf(buf)) == LLVMPointerTypeKind) {
    buf_struct = LLVMBuildLoad2(builder, arr_ty, buf, "bufplay.arr");
  }
  LLVMValueRef len_i32 =
      LLVMBuildExtractValue(builder, buf_struct, 0, "bufplay.len.i32");
  LLVMValueRef len_f =
      LLVMBuildSIToFP(builder, len_i32, f64_ty, "bufplay.len.f64");

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                           "bufplay.init.phase_base");
  }
  LLVMValueRef phase_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "bufplay.phase_base");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_base, f64_ptr_ty, "bufplay.phase_ptr");
  BUILD_ON_TRIG(builder, trig, "bufplay",
                LLVMBuildStore(builder, start_pos, phase_ptr););

  LLVMValueRef cur_phase =
      LLVMBuildLoad2(builder, f64_ty, phase_ptr, "bufplay.phase");
  LLVMValueRef step = LLVMBuildFDiv(builder, rate, len_f, "bufplay.step");
  LLVMValueRef advanced =
      LLVMBuildFAdd(builder, cur_phase, step, "bufplay.advanced");

  LLVMValueRef zero = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one = LLVMConstReal(f64_ty, 1.0);
  LLVMValueRef ovf =
      LLVMBuildFCmp(builder, LLVMRealOGE, advanced, one, "bufplay.ovf");
  LLVMValueRef udf =
      LLVMBuildFCmp(builder, LLVMRealOLT, advanced, zero, "bufplay.udf");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, zero, advanced, "bufplay.wrap_ovf");
  next = LLVMBuildSelect(builder, udf, one, next, "bufplay.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);

  if (num_channels <= 1) {
    return DSP_SCALAR(
        build_tabread(buf, cur_phase, dsp_ctx, ctx, module, builder));
  }

  LLVMValueRef *vals =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)num_channels, 8);
  if (!vals) {
    return DSP_NULL;
  }
  for (int ch = 0; ch < num_channels; ch++) {
    vals[ch] = build_interleaved_tabread_channel(
        buf_struct, cur_phase, num_channels, ch, module, builder);
  }
  return DSP_MULTI(num_channels, vals);
}

static LLVMValueRef build_grains(int32_t max_grains, LLVMValueRef buf,
                                 LLVMValueRef rate, LLVMValueRef pos,
                                 LLVMValueRef width, LLVMValueRef trig,
                                 DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef i32_ptr_ty = LLVMPointerType(i32_ty, 0);
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);

  LLVMValueRef buf_struct = buf;
  if (LLVMGetTypeKind(LLVMTypeOf(buf)) == LLVMPointerTypeKind) {
    buf_struct = LLVMBuildLoad2(builder, arr_ty, buf, "grains.arr");
  }
  LLVMValueRef len_i32 =
      LLVMBuildExtractValue(builder, buf_struct, 0, "grains.len.i32");
  LLVMValueRef buf_ptr =
      LLVMBuildExtractValue(builder, buf_struct, 1, "grains.data");
  LLVMValueRef buf_size_i64 =
      LLVMBuildSExt(builder, len_i32, i64_ty, "grains.len.i64");

  int spawn_trig_off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  int active_grains_off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  int arrays_off = dsp_ctx->state_offset;
  int array_bytes = max_grains * 8 + max_grains * 8 + max_grains * 8 +
                    max_grains * 8 + max_grains * 8 + max_grains * 4;
  dsp_ctx->state_offset += array_bytes;

  (void)spawn_trig_off;
  (void)active_grains_off;
  (void)arrays_off;
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                                 "grains.init.spawn_base");
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                                 "grains.init.active_grains_base");
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, array_bytes, 8,
                                 "grains.init.arrays_base");
  }

  LLVMValueRef spawn_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "grains.spawn_base");
  LLVMValueRef active_grains_base = dsp_consume_frame_state(
      dsp_ctx, builder, 8, 8, "grains.active_grains_base");
  LLVMValueRef base_ptr = dsp_consume_frame_state(dsp_ctx, builder, array_bytes,
                                                  8, "grains.base_ptr");

  LLVMValueRef f64_size =
      LLVMConstInt(i32_ty, (uint64_t)(sizeof(double) * max_grains), 0);

  LLVMValueRef rates_ptr = base_ptr;
  LLVMValueRef phases_ptr = LLVMBuildGEP2(builder, i8_ty, rates_ptr, &f64_size,
                                          1, "grains.phases_ptr");
  LLVMValueRef widths_ptr = LLVMBuildGEP2(builder, i8_ty, phases_ptr, &f64_size,
                                          1, "grains.widths_ptr");
  LLVMValueRef remaining_ptr =
      LLVMBuildGEP2(builder, i8_ty, widths_ptr, &f64_size, 1, "grains.rem_ptr");
  LLVMValueRef starts_ptr = LLVMBuildGEP2(builder, i8_ty, remaining_ptr,
                                          &f64_size, 1, "grains.starts_ptr");
  LLVMValueRef active_ptr = LLVMBuildGEP2(builder, i8_ty, starts_ptr, &f64_size,
                                          1, "grains.active_ptr");

  LLVMValueRef spawn_ptr =
      LLVMBuildBitCast(builder, spawn_base, f64_ptr_ty, "grains.spawn_ptr");
  LLVMBuildStore(builder, LLVMConstReal(f64_ty, 0.0), spawn_ptr);
  BUILD_ON_TRIG(
      builder, trig, "grains",
      LLVMBuildStore(builder, LLVMConstReal(f64_ty, 1.0), spawn_ptr););
  LLVMValueRef spawn_trig =
      LLVMBuildLoad2(builder, f64_ty, spawn_ptr, "grains.spawn_trig");

  LLVMValueRef active_grains_ptr = LLVMBuildBitCast(
      builder, active_grains_base, i32_ptr_ty, "grains.active_grains_ptr");

  LLVMTypeRef grain_fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){f64_ptr_ty, i64_ty, f64_ty, f64_ty, f64_ty, f64_ty,
                      f64_ty, i32_ty, f64_ptr_ty, f64_ptr_ty, f64_ptr_ty,
                      f64_ptr_ty, f64_ptr_ty, i32_ptr_ty, i32_ptr_ty},
      15, 0);
  LLVMValueRef grain_fn = LLVMGetNamedFunction(module, "grain_samp");
  if (!grain_fn) {
    grain_fn = LLVMAddFunction(module, "grain_samp", grain_fn_ty);
    LLVMSetLinkage(grain_fn, LLVMExternalLinkage);
  }

  LLVMValueRef max_grains_val = LLVMConstInt(i32_ty, (uint64_t)max_grains, 0);
  LLVMValueRef grain_args[] = {
      buf_ptr,
      buf_size_i64,
      spawn_trig,
      pos,
      rate,
      width,
      dsp_ctx->spf,
      max_grains_val,
      LLVMBuildBitCast(builder, rates_ptr, f64_ptr_ty, "grains.rates.f64_ptr"),
      LLVMBuildBitCast(builder, phases_ptr, f64_ptr_ty,
                       "grains.phases.f64_ptr"),
      LLVMBuildBitCast(builder, widths_ptr, f64_ptr_ty,
                       "grains.widths.f64_ptr"),
      LLVMBuildBitCast(builder, remaining_ptr, f64_ptr_ty,
                       "grains.rem.f64_ptr"),
      LLVMBuildBitCast(builder, starts_ptr, f64_ptr_ty,
                       "grains.starts.f64_ptr"),
      LLVMBuildBitCast(builder, active_ptr, i32_ptr_ty,
                       "grains.active.i32_ptr"),
      LLVMBuildBitCast(builder, active_grains_ptr, i32_ptr_ty,
                       "grains.active_grains.i32_ptr"),
  };

  return LLVMBuildCall2(builder, grain_fn_ty, grain_fn, grain_args, 15,
                        "grains.sample");
}

LLVMValueRef build_exp_decay(LLVMValueRef T, LLVMValueRef trig,
                             DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  (void)ctx;

  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 16;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)off;
  if (dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 16, 8, "exp_decay.init.base");
    LLVMValueRef val_init_ptr_i8 = init_base;
    LLVMValueRef val_init_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, val_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.val_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   val_init_ptr);

    LLVMValueRef prev_init_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_base,
                      (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
                      "exp_decay.prev_trig_init_ptr");
    LLVMValueRef prev_init_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, prev_init_ptr_i8, f64_ptr_ty,
                         "exp_decay.prev_trig_init_f64_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   prev_init_ptr);
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 16, 8, "exp_decay.base");
  LLVMValueRef val_ptr_i8 = base;
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, val_ptr_i8, f64_ptr_ty, "exp_decay.val_ptr");
  LLVMValueRef prev_trig_ptr_i8 = LLVMBuildGEP2(
      builder, i8_ty, base, (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
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

void dsp_write_output(void *node_raw, int64_t frame, double val) {
  ((Node *)node_raw)->output.buf[frame] = val;
}

void dsp_write_outputs(void *node_raw, int64_t frame, double *vals,
                       int32_t count) {
  Node *node = (Node *)node_raw;
  int layout = node->output.layout;
  if (count < layout) {
    layout = count;
  }
  for (int32_t ch = 0; ch < layout; ch++) {
    node->output.buf[(frame * node->output.layout) + ch] = vals[ch];
  }
}

double ylc_read_inlet_node(void *node_raw, int64_t frame) {
  return ((Node *)node_raw)->output.buf[frame];
}

void ylc_kill_node(void *node_raw) { ((Node *)node_raw)->trig_end = true; }

double adsr_samp(double trig, double prev_trig, double attack, double decay,
                 double sustain, double release, double spf, double *value_ptr,
                 double *phase_ptr, double *prev_trig_ptr) {
  double value = *value_ptr;
  double phase = *phase_ptr;
  const double threshold = 0.5;

  int rising = (prev_trig < threshold && trig >= threshold);
  int falling = (prev_trig >= threshold && trig < threshold);

  if (rising) {
    phase = 1.0;
  } else if (falling && phase == 3.0) {
    phase = 4.0;
  }

  if (phase == 1.0) {
    double rate = (attack > 0.0) ? (1.0 / attack) : 1e6;
    value += rate * spf;
    if (value >= 1.0) {
      value = 1.0;
      phase = 2.0;
    }
  } else if (phase == 2.0) {
    double rate = (decay > 0.0) ? ((1.0 - sustain) / decay) : 1e6;
    value -= rate * spf;
    if (value <= sustain) {
      value = sustain;
      phase = (trig >= threshold) ? 3.0 : 4.0;
    }
  } else if (phase == 3.0) {
    value = sustain;
  } else if (phase == 4.0) {
    double rate = (release > 0.0) ? (1.0 / release) : 1e6;
    value -= rate * spf;
    if (value <= 0.0) {
      value = 0.0;
      phase = 0.0;
    }
  } else {
    value = 0.0;
  }

  *value_ptr = value;
  *phase_ptr = phase;
  *prev_trig_ptr = trig;
  return value;
}

double rect_samp(double duration, double trig, double prev_trig, double spf,
                 double *remaining_ptr, double *prev_trig_ptr) {
  double remaining = *remaining_ptr;
  const double threshold = 0.5;

  int rising = (prev_trig < threshold && trig >= threshold);
  if (rising) {
    remaining = duration > 0.0 ? duration : 0.0;
  }

  double out = remaining > 0.0 ? 1.0 : 0.0;
  if (remaining > 0.0) {
    remaining -= spf;
    if (remaining < 0.0) {
      remaining = 0.0;
    }
  }

  *remaining_ptr = remaining;
  *prev_trig_ptr = trig;
  return out;
}

double pm_osc_samp(double carrier_freq, double mod_index, double mod_ratio,
                   double spf, double *carrier_phase_ptr,
                   double *modulator_phase_ptr) {
  const double turns_per_unit = 0.5;
  double carrier_phase = *carrier_phase_ptr;
  double modulator_phase = *modulator_phase_ptr;

  const double table_size = (double)SIN_TABSIZE;
  const int table_mask = SIN_TABSIZE - 1;
  double modulator_freq = carrier_freq * mod_ratio;

  double mod_phase_scaled = modulator_phase * table_size;
  int mod_index_int = (int)mod_phase_scaled;
  double mod_frac = mod_phase_scaled - (double)mod_index_int;
  mod_index_int &= table_mask;

  int mod_idx_0 = (mod_index_int - 1) & table_mask;
  int mod_idx_1 = mod_index_int;
  int mod_idx_2 = (mod_index_int + 1) & table_mask;
  int mod_idx_3 = (mod_index_int + 2) & table_mask;

  double mod_y0 = ylc_sin_table[mod_idx_0];
  double mod_y1 = ylc_sin_table[mod_idx_1];
  double mod_y2 = ylc_sin_table[mod_idx_2];
  double mod_y3 = ylc_sin_table[mod_idx_3];

  double mod_c0 = mod_y1;
  double mod_c1 = 0.5 * (mod_y2 - mod_y0);
  double mod_c2 = mod_y0 - 2.5 * mod_y1 + 2.0 * mod_y2 - 0.5 * mod_y3;
  double mod_c3 = 0.5 * (mod_y3 - mod_y0) + 1.5 * (mod_y1 - mod_y2);

  double modulator_value =
      ((mod_c3 * mod_frac + mod_c2) * mod_frac + mod_c1) * mod_frac + mod_c0;
  modulator_value *= (mod_index * turns_per_unit);

  double modulated_phase = carrier_phase + modulator_value;
  modulated_phase -= floor(modulated_phase);

  double carrier_phase_scaled = modulated_phase * table_size;
  int carrier_index_int = (int)carrier_phase_scaled;
  double carrier_frac = carrier_phase_scaled - (double)carrier_index_int;
  carrier_index_int &= table_mask;

  int carr_idx_0 = (carrier_index_int - 1) & table_mask;
  int carr_idx_1 = carrier_index_int;
  int carr_idx_2 = (carrier_index_int + 1) & table_mask;
  int carr_idx_3 = (carrier_index_int + 2) & table_mask;

  double carr_y0 = ylc_sin_table[carr_idx_0];
  double carr_y1 = ylc_sin_table[carr_idx_1];
  double carr_y2 = ylc_sin_table[carr_idx_2];
  double carr_y3 = ylc_sin_table[carr_idx_3];

  double carr_c0 = carr_y1;
  double carr_c1 = 0.5 * (carr_y2 - carr_y0);
  double carr_c2 = carr_y0 - 2.5 * carr_y1 + 2.0 * carr_y2 - 0.5 * carr_y3;
  double carr_c3 = 0.5 * (carr_y3 - carr_y0) + 1.5 * (carr_y1 - carr_y2);

  double carrier_value =
      ((carr_c3 * carrier_frac + carr_c2) * carrier_frac + carr_c1) *
          carrier_frac +
      carr_c0;

  modulator_phase += modulator_freq * spf;
  modulator_phase -= floor(modulator_phase);

  carrier_phase += carrier_freq * spf;
  carrier_phase -= floor(carrier_phase);

  *modulator_phase_ptr = modulator_phase;
  *carrier_phase_ptr = carrier_phase;
  return carrier_value;
}

double phase_osc_samp(double phase_input, double carrier_freq, double spf,
                      double *carrier_phase_ptr) {
  const double turns_per_unit = 0.5;
  const double table_size = (double)SIN_TABSIZE;
  const int table_mask = SIN_TABSIZE - 1;

  double carrier_phase = *carrier_phase_ptr;
  double modulated_phase = carrier_phase + (phase_input * turns_per_unit);
  modulated_phase -= floor(modulated_phase);

  double carrier_phase_scaled = modulated_phase * table_size;
  int carrier_index_int = (int)carrier_phase_scaled;
  double carrier_frac = carrier_phase_scaled - (double)carrier_index_int;
  carrier_index_int &= table_mask;

  int idx_0 = (carrier_index_int - 1) & table_mask;
  int idx_1 = carrier_index_int;
  int idx_2 = (carrier_index_int + 1) & table_mask;
  int idx_3 = (carrier_index_int + 2) & table_mask;

  double y0 = ylc_sin_table[idx_0];
  double y1 = ylc_sin_table[idx_1];
  double y2 = ylc_sin_table[idx_2];
  double y3 = ylc_sin_table[idx_3];

  double c0 = y1;
  double c1 = 0.5 * (y2 - y0);
  double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
  double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);

  double out =
      ((c3 * carrier_frac + c2) * carrier_frac + c1) * carrier_frac + c0;

  carrier_phase += carrier_freq * spf;
  carrier_phase -= floor(carrier_phase);

  *carrier_phase_ptr = carrier_phase;
  return out;
}

double bank_osc_samp(_DoubleArray amps, _DoubleArray phases, double freq,
                     double spf, double *phase_ptr) {
  if (!amps.data || amps.size <= 0 || !phase_ptr) {
    return 0.0;
  }

  double phase = *phase_ptr;
  double output = 0.0;
  double norm = 1.0;
  const double turns_per_unit = 0.5;
  const int table_mask = SIN_TABSIZE - 1;

  for (int32_t i = 0; i < amps.size; i++) {
    double amp = amps.data[i];
    double phase_offset = (phases.data && i < phases.size)
                              ? phases.data[i] * turns_per_unit
                              : 0.0;
    double partial_phase = ((double)(i + 1) * phase) + phase_offset;
    partial_phase -= floor(partial_phase);

    double d_index = partial_phase * (double)SIN_TABSIZE;
    int index = (int)d_index;
    double frac = d_index - (double)index;

    double a = ylc_sin_table[index & table_mask];
    double b = ylc_sin_table[(index + 1) & table_mask];
    double sample = ((1.0 - frac) * a) + (frac * b);

    output += sample * amp;
    norm += amp;
  }

  phase += freq * spf;
  phase -= floor(phase);
  *phase_ptr = phase;

  return output / norm;
}

double grain_samp(double *buf, int64_t buf_size, double trig, double pos,
                  double rate, double width, double spf, int32_t max_grains,
                  double *rates, double *phases, double *widths,
                  double *remaining_secs, double *starts, int32_t *active,
                  int32_t *active_grains) {
  if (!buf || buf_size <= 1 || max_grains <= 0 || !rates || !phases ||
      !widths || !remaining_secs || !starts || !active || !active_grains) {
    return 0.0;
  }

  double sample = 0.0;

  if (trig >= 0.5 && *active_grains < max_grains) {
    for (int32_t i = 0; i < max_grains; i++) {
      if (active[i] == 0) {
        rates[i] = rate;
        phases[i] = 0.0;
        starts[i] = pos * (double)buf_size;
        widths[i] = width;
        remaining_secs[i] = width;
        active[i] = 1;
        (*active_grains)++;
        break;
      }
    }
  }

  for (int32_t i = 0; i < max_grains; i++) {
    if (!active[i]) {
      continue;
    }

    double r = rates[i];
    double p = phases[i];
    double s = starts[i];
    double w = widths[i];
    double rem = remaining_secs[i];

    double d_index = s + (p * (double)buf_size);
    int64_t index = (int64_t)d_index;
    double frac = d_index - (double)index;

    int64_t i0 = index % buf_size;
    if (i0 < 0) {
      i0 += buf_size;
    }
    int64_t i1 = (i0 + 1) % buf_size;
    double a = buf[i0];
    double b_val = buf[i1];

    double grain_elapsed = 1.0 - (rem / w);
    int mask = GRAIN_WINDOW_TABSIZE - 1;
    double env_pos = grain_elapsed * (double)mask;
    int env_idx = (int)env_pos;
    double env_frac = env_pos - (double)env_idx;
    double env_val = ylc_grain_win[env_idx & mask] * (1.0 - env_frac) +
                     ylc_grain_win[(env_idx + 1) & mask] * env_frac;

    sample += env_val * ((1.0 - frac) * a + (frac * b_val));
    phases[i] += (r / (double)buf_size);

    remaining_secs[i] -= spf;
    if (remaining_secs[i] <= 0.0) {
      active[i] = 0;
      (*active_grains)--;
    }
  }

  return sample;
}
LLVMValueRef build_adsr(LLVMValueRef attack, LLVMValueRef decay,
                        LLVMValueRef sustain, LLVMValueRef release,
                        LLVMValueRef trig, DspBuildCtx *dsp_ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 24;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)off;
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 24, 8,
                                 "adsr.init.base");
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 24, 8, "adsr.base");
  LLVMValueRef value_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "adsr.value_ptr");
  LLVMValueRef phase_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), base,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 8, 0)}, 1,
                    "adsr.phase_ptr_i8");
  LLVMValueRef prev_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), base,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 16, 0)}, 1,
                    "adsr.prev_ptr_i8");
  LLVMValueRef phase_ptr =
      LLVMBuildBitCast(builder, phase_ptr_i8, f64_ptr_ty, "adsr.phase_ptr");
  LLVMValueRef prev_ptr =
      LLVMBuildBitCast(builder, prev_ptr_i8, f64_ptr_ty, "adsr.prev_ptr");

  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_ptr, "adsr.prev_trig");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ty, f64_ty, f64_ty, f64_ty,
                      f64_ptr_ty, f64_ptr_ty, f64_ptr_ty},
      10, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "adsr_samp");
  if (!fn) {
    fn = LLVMAddFunction(module, "adsr_samp", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  return LLVMBuildCall2(builder, fn_ty, fn,
                        (LLVMValueRef[]){trig, prev_trig, attack, decay,
                                         sustain, release, dsp_ctx->spf,
                                         value_ptr, phase_ptr, prev_ptr},
                        10, "adsr.sample");
}

LLVMValueRef build_rect(LLVMValueRef duration, LLVMValueRef trig,
                        DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 16;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)off;
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    (void)dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 16, 8,
                                 "rect.init.base");
  }

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 16, 8, "rect.base");
  LLVMValueRef rem_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "rect.rem_ptr");
  LLVMValueRef prev_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), base,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 8, 0)}, 1,
                    "rect.prev_ptr_i8");
  LLVMValueRef prev_ptr =
      LLVMBuildBitCast(builder, prev_ptr_i8, f64_ptr_ty, "rect.prev_ptr");

  LLVMValueRef prev_trig =
      LLVMBuildLoad2(builder, f64_ty, prev_ptr, "rect.prev_trig");

  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ty, f64_ptr_ty, f64_ptr_ty},
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

DspValue call_registered_synth_in_audio_fn(Ast *ast, SynthRecord rec,
                                           DspBuildCtx *dsp_ctx,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  if (!rec.frame_fn) {
    fprintf(stderr, "audio_jit: missing frame_fn for registered synth\n");
    return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
  }

  LLVMValueRef frame_fn = rec.frame_fn;
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(frame_fn);

  unsigned formal_count = LLVMCountParamTypes(frame_fn_ty);
  if (formal_count < 3) {
    fprintf(stderr, "audio_jit: malformed frame_fn signature\n");
    return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
  }

  LLVMTypeRef formal_tys[formal_count];
  LLVMGetParamTypes(frame_fn_ty, formal_tys);

  int arg_count = ast->data.AST_APPLICATION.len;
  DspValue arg_vals[arg_count];
  int call_lanes = 1;
  for (int i = 0; i < arg_count; i++) {
    arg_vals[i] = dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx,
                                 ctx, module, builder);
    DspSynthArgKind arg_kind = (rec.arg_kinds && i < rec.arg_count)
                                   ? rec.arg_kinds[i]
                                   : DSP_SYNTH_ARG_SCALAR_OR_MULTICHANNEL;
    if (arg_kind == DSP_SYNTH_ARG_SCALAR_OR_MULTICHANNEL &&
        arg_vals[i].lanes > call_lanes) {
      call_lanes = arg_vals[i].lanes;
    }
  }

  int off = (dsp_ctx->state_offset + 7) & ~7;
  int state_bytes = (rec.state_bytes + 7) & ~7;
  int total_state_bytes = state_bytes * call_lanes;
  dsp_ctx->state_offset = off + total_state_bytes;

  LLVMValueRef state_base = dsp_consume_frame_state(
      dsp_ctx, builder, total_state_bytes, 8, "sub_synth.state_ptr");

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_state_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, total_state_bytes, 8,
        "sub_synth.init_state_ptr");
    if (rec.init_fn) {
      LLVMTypeRef init_fn_ty = LLVMGlobalGetValueType(rec.init_fn);
      for (int lane = 0; lane < call_lanes; lane++) {
        LLVMValueRef lane_init_state = init_state_base;
        if (lane > 0) {
          LLVMValueRef lane_off =
              LLVMConstInt(LLVMInt64Type(), (uint64_t)(lane * state_bytes), 0);
          lane_init_state = LLVMBuildGEP2(dsp_ctx->init_builder, LLVMInt8Type(),
                                          init_state_base, &lane_off, 1,
                                          "sub_synth.init_state_lane");
        }
        LLVMBuildCall2(dsp_ctx->init_builder, init_fn_ty, rec.init_fn,
                       (LLVMValueRef[]){lane_init_state}, 1, "sub_synth.init");
      }
    }
  }

  LLVMValueRef out_storage = LLVMBuildArrayAlloca(
      builder, LLVMDoubleType(),
      LLVMConstInt(LLVMInt32Type(), (uint64_t)rec.output_lanes, 0),
      "reg_synth.out");

  int total_output_lanes = call_lanes * rec.output_lanes;
  LLVMValueRef *vals =
      total_output_lanes > 1
          ? dsp_tmp_alloc(dsp_ctx,
                          sizeof(LLVMValueRef) * (size_t)total_output_lanes, 8)
          : NULL;
  if (!vals) {
    if (total_output_lanes > 1) {
      return DSP_NULL;
    }
  }

  for (int lane = 0; lane < call_lanes; lane++) {
    LLVMValueRef frame_args[formal_count];
    LLVMValueRef lane_state = state_base;
    if (lane > 0) {
      LLVMValueRef lane_off =
          LLVMConstInt(LLVMInt64Type(), (uint64_t)(lane * state_bytes), 0);
      lane_state = LLVMBuildGEP2(builder, LLVMInt8Type(), state_base, &lane_off,
                                 1, "sub_synth.state_lane");
    }

    frame_args[0] = lane_state;
    frame_args[1] = dsp_ctx->node_ptr;
    frame_args[2] = out_storage;

    for (unsigned i = 3; i < formal_count; i++) {
      int arg_idx = (int)i - 3;
      if (arg_idx < arg_count) {
        Ast *arg_ast = ast->data.AST_APPLICATION.args + arg_idx;
        DspValue arg_dv = arg_vals[arg_idx];
        DspSynthArgKind arg_kind = (rec.arg_kinds && arg_idx < rec.arg_count)
                                       ? rec.arg_kinds[arg_idx]
                                       : DSP_SYNTH_ARG_SCALAR_OR_MULTICHANNEL;
        LLVMValueRef arg_val = arg_dv.scalar;
        if (arg_kind == DSP_SYNTH_ARG_ARRAY_ONLY) {
          if (arg_dv.lanes > 1) {
            fprintf(stderr,
                    "audio_jit: warning: subsynth '%s' arg %d expects array; "
                    "using lane 0 from multichannel value\n",
                    rec.name ? rec.name : "?", arg_idx);
          }
        } else if (arg_kind == DSP_SYNTH_ARG_SCALAR_ONLY) {
          if (arg_dv.lanes > 1) {
            fprintf(stderr,
                    "audio_jit: warning: subsynth '%s' arg %d expects scalar; "
                    "using lane 0 from multichannel value\n",
                    rec.name ? rec.name : "?", arg_idx);
          }
        } else {
          arg_val = arg_dv.lanes > 1 ? arg_dv.vec[lane % arg_dv.lanes]
                                     : arg_dv.scalar;
        }
        if (LLVMGetTypeKind(formal_tys[i]) == LLVMDoubleTypeKind) {
          arg_val = ensure_float(arg_ast->type, arg_val, builder);
        }
        frame_args[i] = arg_val;
      } else {
        frame_args[i] = LLVMConstNull(formal_tys[i]);
      }
    }

    LLVMBuildCall2(builder, frame_fn_ty, frame_fn, frame_args, formal_count,
                   "reg_synth.frame_call");

    for (int out_lane = 0; out_lane < rec.output_lanes; out_lane++) {
      LLVMValueRef idx = LLVMConstInt(LLVMInt64Type(), (uint64_t)out_lane, 0);
      LLVMValueRef lane_ptr =
          LLVMBuildGEP2(builder, LLVMDoubleType(), out_storage, &idx, 1,
                        "reg_synth.lane.ptr");
      LLVMValueRef lane_val =
          LLVMBuildLoad2(builder, LLVMDoubleType(), lane_ptr, "reg_synth.lane");
      if (total_output_lanes == 1) {
        return DSP_SCALAR(lane_val);
      }
      vals[lane * rec.output_lanes + out_lane] = lane_val;
    }
  }
  return DSP_MULTI(total_output_lanes, vals);
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
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 16, 8, "lfnoise1.base");
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "lfnoise1.val_ptr");
  LLVMValueRef slp_ptr_i8 = LLVMBuildGEP2(
      builder, i8_ty, base, (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
      "lfnoise1.slp_ptr_i8");
  LLVMValueRef slp_ptr =
      LLVMBuildBitCast(builder, slp_ptr_i8, f64_ptr_ty, "lfnoise1.slp_ptr");

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
  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 16, 8, "lfnoise1.init.base");
    LLVMValueRef init_val_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_base, f64_ptr_ty, "lfnoise1.init.val_ptr");
    LLVMValueRef init_new_rand =
        LLVMBuildCall2(dsp_ctx->init_builder, rdr_ty, rdr_fn,
                       (LLVMValueRef[]){lo, hi}, 2, "lfnoise1.init.new_rand");
    LLVMBuildStore(dsp_ctx->init_builder, init_new_rand, init_val_ptr);

    LLVMValueRef init_slp_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_base,
                      (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
                      "lfnoise1.init.slp_ptr_i8");
    LLVMValueRef init_slp_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_slp_ptr_i8, f64_ptr_ty,
                         "lfnoise1.init.slp_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_slp_ptr);
  }
  BUILD_ON_TRIG(
      builder, trig, "lfnoise1",
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

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "lfnoise0.base");
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "lfnoise0.val_ptr");

  LLVMTypeRef rdr_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMValueRef rdr_fn = LLVMGetNamedFunction(module, "rand_double_range");
  if (!rdr_fn) {
    rdr_fn = LLVMAddFunction(module, "rand_double_range", rdr_ty);
    LLVMSetLinkage(rdr_fn, LLVMExternalLinkage);
  }

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 8, 8, "lfnoise0.init.base");
    LLVMValueRef init_val_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_base, f64_ptr_ty, "lfnoise0.init.val_ptr");
    LLVMValueRef init_new_rand =
        LLVMBuildCall2(dsp_ctx->init_builder, rdr_ty, rdr_fn,
                       (LLVMValueRef[]){lo, hi}, 2, "lfnoise0.init.new_rand");
    LLVMBuildStore(dsp_ctx->init_builder, init_new_rand, init_val_ptr);
  }
  BUILD_ON_TRIG(builder, trig, "lfnoise0",
                LLVMValueRef new_rand = LLVMBuildCall2(builder, rdr_ty, rdr_fn,
                                                       (LLVMValueRef[]){lo, hi},
                                                       2, "lfnoise0.new_rand");
                LLVMBuildStore(builder, new_rand, val_ptr););

  return LLVMBuildLoad2(builder, f64_ty, val_ptr, "lfnoise0.next_val");
}
void kill_on_end(NodeRef node) {
  printf("kill node %p\n", node);
  node->trig_end = true;
}

LLVMValueRef build_kill_on_end(LLVMValueRef signal, DspBuildCtx *dsp_ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef void_ty = LLVMVoidType();

  // State: prev_val(8)
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder, 8, 8,
                           "kill_on_end.init.base");
  }

  LLVMValueRef prev_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "kill_on_end.prev_base");
  LLVMValueRef prev_ptr = LLVMBuildBitCast(
      builder, prev_base, LLVMPointerType(f64_ty, 0), "kill_on_end.prev_ptr");
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
  LLVMValueRef crossed = LLVMBuildAnd(builder, prev_above_eps, cur_below_eps,
                                      "kill_on_end.crossed");
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
double allpass1_sample(_DoubleArray buf, double delay_secs, double spf,
                       int *write_pos, double input, double g) {
  int32_t buf_size = buf.size;
  double *buf_data = buf.data;
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;

  int delay_i = (int)delay_samps_f;
  double frac = delay_samps_f - delay_i;

  int read0 = (*write_pos - delay_i + buf_size) % buf_size;
  int read1 =
      (read0 - 1 + buf_size) % buf_size; // important: older sample, not +1

  double delayed = buf_data[read0] * (1.0 - frac) + buf_data[read1] * frac;

  double out = delayed - g * input;           // Schroeder allpass output
  buf_data[*write_pos] = input + g * delayed; // state write
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

double allpass_sample(_DoubleArray buf, double delay_secs, double spf,
                      int *write_pos, double input, double g) {
  int32_t buf_size = buf.size;
  double *buf_data = buf.data;
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;

  int delay_i = (int)delay_samps_f;
  int read0 = (*write_pos - delay_i + buf_size) % buf_size;
  double delayed = buf_data[read0];

  double out = delayed - g * input;
  buf_data[*write_pos] = input + g * delayed;
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

// Interpolating feedback delay:
// delayed = lerp(buf[read0], buf[read1]), out = input + delayed,
// buf[write] = fb * out
double delay_sample(_DoubleArray buf, double delay_secs, double spf,
                    int *write_pos, double input, double fb) {
  int32_t buf_size = buf.size;
  double *buf_data = buf.data;
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;

  int delay_i = (int)delay_samps_f;
  double frac = delay_samps_f - delay_i;

  int read0 = (*write_pos - delay_i + buf_size) % buf_size;
  int read1 = (read0 - 1 + buf_size) % buf_size;
  double delayed = buf_data[read0] * (1.0 - frac) + buf_data[read1] * frac;

  double out = input + delayed;
  buf_data[*write_pos] = fb * out;
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

// Interpolating feedback comb:
// delayed = lerp(...), out = input + fb*delayed, buf[write] = out
double comb_sample(_DoubleArray buf, double delay_secs, double spf,
                   int *write_pos, double input, double fb) {
  int32_t buf_size = buf.size;
  double *buf_data = buf.data;
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;

  int delay_i = (int)delay_samps_f;
  double frac = delay_samps_f - delay_i;

  int read0 = (*write_pos - delay_i + buf_size) % buf_size;
  int read1 = (read0 - 1 + buf_size) % buf_size;
  double delayed = buf_data[read0] * (1.0 - frac) + buf_data[read1] * frac;

  double out = input + fb * delayed;
  buf_data[*write_pos] = out;
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

double lag_sample(double input, double lag_secs, double spf, double *y1_ptr,
                  double *b1_ptr, double *lag_ptr) {
  const double log001 = -6.907755278982137; // log(0.001)
  if (lag_secs < 0.0) {
    lag_secs = 0.0;
  }

  double y1 = *y1_ptr;
  double b1 = *b1_ptr;
  double prev_lag = *lag_ptr;

  if (lag_secs != prev_lag) {
    b1 = (lag_secs == 0.0 || spf <= 0.0) ? 0.0 : exp(log001 / (lag_secs / spf));
    *b1_ptr = b1;
    *lag_ptr = lag_secs;
  }

  y1 = input + b1 * (y1 - input);
  if (!isfinite(y1)) {
    y1 = input;
  }
  *y1_ptr = y1;
  return y1;
}

static LLVMValueRef build_lag(LLVMValueRef input, LLVMValueRef lag_secs,
                              DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  int y1_off = off;
  int b1_off = off + 8;
  int lag_off = off + 16;
  dsp_ctx->state_offset = off + 24;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)y1_off;
  (void)b1_off;
  (void)lag_off;
  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 24, 8, "lag.base");
  LLVMValueRef y1_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "lag.y1_ptr");
  LLVMValueRef b1_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), base,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 8, 0)}, 1,
                    "lag.b1_ptr_i8");
  LLVMValueRef lag_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), base,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 16, 0)}, 1,
                    "lag.lag_ptr_i8");
  LLVMValueRef b1_ptr =
      LLVMBuildBitCast(builder, b1_ptr_i8, f64_ptr_ty, "lag.b1_ptr");
  LLVMValueRef lag_ptr =
      LLVMBuildBitCast(builder, lag_ptr_i8, f64_ptr_ty, "lag.lag_ptr");

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 24, 8, "lag.init.base");
    LLVMValueRef init_y1_ptr_i8 = init_base;
    LLVMValueRef init_b1_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, LLVMInt8Type(), init_base,
                      (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 8, 0)}, 1,
                      "lag.init.b1_ptr_i8");
    LLVMValueRef init_lag_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, LLVMInt8Type(), init_base,
                      (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 16, 0)}, 1,
                      "lag.init.lag_ptr_i8");

    LLVMValueRef init_y1_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_y1_ptr_i8, f64_ptr_ty, "lag.init.y1_ptr");
    LLVMValueRef init_b1_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_b1_ptr_i8, f64_ptr_ty, "lag.init.b1_ptr");
    LLVMValueRef init_lag_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_lag_ptr_i8, f64_ptr_ty, "lag.init.lag_ptr");

    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_y1_ptr);
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, 0.0),
                   init_b1_ptr);
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, -1.0),
                   init_lag_ptr);
  }

  LLVMTypeRef fn_ty =
      LLVMFunctionType(f64_ty,
                       (LLVMTypeRef[]){f64_ty, f64_ty, f64_ty, f64_ptr_ty,
                                       f64_ptr_ty, f64_ptr_ty},
                       6, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "lag_sample");
  if (!fn) {
    fn = LLVMAddFunction(module, "lag_sample", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  LLVMValueRef lag_args[] = {input,  lag_secs, dsp_ctx->spf,
                             y1_ptr, b1_ptr,   lag_ptr};
  return LLVMBuildCall2(builder, fn_ty, fn, lag_args, 6, "lag.sample");
}

typedef struct {
  int32_t buf_size;
  int write_pos_off;
  int buf_struct_off;
  int buf_data_off;
  LLVMValueRef write_pos_ptr;
  LLVMValueRef buf_arr;
} DelayBufIR;

typedef struct {
  const char *fn_name;
  const char *call_name;
} DelayKernelOp;

static bool eval_delay_buf_size(Ast *max_delay_ast, DspBuildCtx *dsp_ctx,
                                JITLangCtx *ctx, int32_t *out_buf_size) {
  double max_delay_secs = 0.0;

  if (!ast_try_eval_const_num(max_delay_ast, dsp_ctx, ctx, &max_delay_secs)) {
    fprintf(stderr, "Error: max_delay must be constant\n");
    return false;
  }
  if (max_delay_secs <= 0.0) {
    fprintf(stderr, "Error: max_delay must be > 0\n");
    return false;
  }

  int sample_rate = ctx_sample_rate();
  if (sample_rate <= 0) {
    sample_rate = 48000;
  }
  int32_t buf_size = (int32_t)(ceil(max_delay_secs * (double)sample_rate));
  // int32_t buf_size = (int32_t)(max_delay_secs * (double)sample_rate);
  if (buf_size < 2) {
    buf_size = 2;
  }

  *out_buf_size = buf_size;
  return true;
}

static Ast *delay_kernel_max_delay_lane_ast(Ast *max_delay_ast, int lane_idx,
                                            int *out_lanes) {
  Ast *items = NULL;
  int len = 0;

  if ((max_delay_ast->tag == AST_ARRAY || max_delay_ast->tag == AST_LIST) &&
      max_delay_ast->data.AST_LIST.len > 0) {
    items = max_delay_ast->data.AST_LIST.items;
    len = max_delay_ast->data.AST_LIST.len;
  } else if (max_delay_ast->tag == AST_APPLICATION &&
             is_ident(max_delay_ast->data.AST_APPLICATION.function,
                      _FORK_OPERATOR_ID)) {
    Ast *fork_arg = max_delay_ast->data.AST_APPLICATION.args;
    if ((fork_arg->tag == AST_ARRAY || fork_arg->tag == AST_LIST) &&
        fork_arg->data.AST_LIST.len > 0) {
      items = fork_arg->data.AST_LIST.items;
      len = fork_arg->data.AST_LIST.len;
    }
  }

  if (items) {
    if (out_lanes) {
      *out_lanes = len;
    }
    return items + (lane_idx % len);
  }

  if (out_lanes) {
    *out_lanes = 1;
  }
  return max_delay_ast;
}

static bool eval_delay_buf_size_for_lane(Ast *max_delay_ast, int lane_idx,
                                         DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                         int32_t *out_buf_size) {
  return eval_delay_buf_size(
      delay_kernel_max_delay_lane_ast(max_delay_ast, lane_idx, NULL), dsp_ctx,
      ctx, out_buf_size);
}

static int delay_kernel_max_delay_lanes(Ast *max_delay_ast) {
  int lanes = 1;
  (void)delay_kernel_max_delay_lane_ast(max_delay_ast, 0, &lanes);
  return lanes;
}

static DelayBufIR build_delay_buf_ir(DspBuildCtx *dsp_ctx,
                                     LLVMBuilderRef builder, int32_t buf_size,
                                     bool emit_init, bool include_write_pos) {
  DelayBufIR ir = {0};
  ir.buf_size = buf_size;

  int off = (dsp_ctx->state_offset + 7) & ~7;
  int struct_off = include_write_pos ? off + 8 : off;
  int data_off = struct_off + 16;
  ir.write_pos_off = include_write_pos ? off : -1;
  ir.buf_struct_off = struct_off;
  ir.buf_data_off = data_off;
  dsp_ctx->state_offset = data_off + (int)(buf_size * (int32_t)sizeof(double));

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ptr_ty = LLVMPointerType(i32_ty, 0);
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);
  LLVMTypeRef arr_ptr_ty = LLVMPointerType(arr_ty, 0);
  int total_bytes = (include_write_pos ? 8 : 0) + 16 +
                    (int)(buf_size * (int32_t)sizeof(double));

  if (emit_init && dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, total_bytes, 8, "delaybuf.init.base");

    if (include_write_pos) {
      LLVMValueRef init_write_pos_ptr =
          LLVMBuildBitCast(dsp_ctx->init_builder, init_base, i32_ptr_ty,
                           "delaybuf.init.write_pos_ptr");
      LLVMBuildStore(dsp_ctx->init_builder, LLVMConstInt(i32_ty, 0, 0),
                     init_write_pos_ptr);
    }

    int64_t struct_rel = include_write_pos ? 8 : 0;
    int64_t data_rel = struct_rel + 16;
    LLVMValueRef init_buf_struct_ptr_i8 =
        struct_rel ? LLVMBuildGEP2(
                         dsp_ctx->init_builder, i8_ty, init_base,
                         (LLVMValueRef[]){LLVMConstInt(i64_ty, struct_rel, 0)},
                         1, "delaybuf.init.struct_ptr_i8")
                   : init_base;
    LLVMValueRef init_buf_data_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_base,
                      (LLVMValueRef[]){LLVMConstInt(i64_ty, data_rel, 0)}, 1,
                      "delaybuf.init.data_ptr_i8");
    LLVMValueRef init_buf_data_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_buf_data_ptr_i8,
                         f64_ptr_ty, "delaybuf.init.data_ptr");

    LLVMValueRef init_buf_struct_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_buf_struct_ptr_i8,
                         arr_ptr_ty, "delaybuf.init.struct_ptr");
    LLVMValueRef init_buf_arr = LLVMGetUndef(arr_ty);
    init_buf_arr = LLVMBuildInsertValue(
        dsp_ctx->init_builder, init_buf_arr,
        LLVMConstInt(i32_ty, (uint64_t)buf_size, 0), 0, "delaybuf.init.size");
    init_buf_arr =
        LLVMBuildInsertValue(dsp_ctx->init_builder, init_buf_arr,
                             init_buf_data_ptr, 1, "delaybuf.init.data");
    LLVMBuildStore(dsp_ctx->init_builder, init_buf_arr, init_buf_struct_ptr);

    LLVMBuildMemSet(
        dsp_ctx->init_builder, init_buf_data_ptr_i8,
        LLVMConstInt(LLVMInt8Type(), 0, 0),
        LLVMConstInt(i64_ty, (uint64_t)(buf_size * (int32_t)sizeof(double)), 0),
        8);
  }

  LLVMValueRef base = dsp_consume_frame_state(dsp_ctx, builder, total_bytes, 8,
                                              "delaybuf.base");
  if (include_write_pos) {
    ir.write_pos_ptr =
        LLVMBuildBitCast(builder, base, i32_ptr_ty, "delaybuf.write_pos_ptr");
  }

  int64_t struct_rel = include_write_pos ? 8 : 0;
  LLVMValueRef buf_struct_ptr_i8 =
      struct_rel
          ? LLVMBuildGEP2(builder, i8_ty, base,
                          (LLVMValueRef[]){LLVMConstInt(i64_ty, struct_rel, 0)},
                          1, "delaybuf.struct_ptr_i8")
          : base;
  LLVMValueRef buf_struct_ptr = LLVMBuildBitCast(
      builder, buf_struct_ptr_i8, arr_ptr_ty, "delaybuf.struct_ptr");
  ir.buf_arr = LLVMBuildLoad2(builder, arr_ty, buf_struct_ptr, "delaybuf.arr");

  return ir;
}

static LLVMValueRef build_random_array_element(LLVMValueRef arr,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMValueRef len_i32 = codegen_get_array_size(builder, arr, LLVMDoubleType());

  LLVMTypeRef rnd_ty =
      LLVMFunctionType(i32_ty, (LLVMTypeRef[]){i32_ty, i32_ty}, 2, 0);
  LLVMValueRef rnd_fn = LLVMGetNamedFunction(module, "ylc_rand_int_range_i32");
  if (!rnd_fn) {
    rnd_fn = LLVMAddFunction(module, "ylc_rand_int_range_i32", rnd_ty);
    LLVMSetLinkage(rnd_fn, LLVMExternalLinkage);
  }

  LLVMValueRef idx_i32 =
      LLVMBuildCall2(builder, rnd_ty, rnd_fn,
                     (LLVMValueRef[]){LLVMConstInt(i32_ty, 0, 0), len_i32}, 2,
                     "array_choose.idx_i32");
  return get_array_element(builder, arr, idx_i32, LLVMDoubleType());
}

LLVMValueRef build_array_choose(LLVMValueRef arr, LLVMValueRef trig,
                                DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  int off = (dsp_ctx->state_offset + 7) & ~7;
  int val_off = off;
  dsp_ctx->state_offset = off + 8;

  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

  (void)val_off;
  LLVMValueRef val_base =
      dsp_consume_frame_state(dsp_ctx, builder, 8, 8, "array_choose.val_base");
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, val_base, f64_ptr_ty, "array_choose.val_ptr");

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_val_ptr_i8 = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 8, 8, "array_choose.init.val_base");
    LLVMValueRef init_val_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_val_ptr_i8, f64_ptr_ty,
                         "array_choose.init.val_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, NAN),
                   init_val_ptr);
  }

  LLVMValueRef cur_val =
      LLVMBuildLoad2(builder, f64_ty, val_ptr, "array_choose.cur_val");
  LLVMValueRef is_uninit = LLVMBuildFCmp(builder, LLVMRealUNO, cur_val, cur_val,
                                         "array_choose.is_uninit");

  LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef fn_parent = LLVMGetBasicBlockParent(cur_bb);
  LLVMBasicBlockRef init_bb =
      LLVMAppendBasicBlock(fn_parent, "array_choose.init");
  LLVMBasicBlockRef cont_bb =
      LLVMAppendBasicBlock(fn_parent, "array_choose.cont");
  LLVMBuildCondBr(builder, is_uninit, init_bb, cont_bb);

  LLVMPositionBuilderAtEnd(builder, init_bb);
  LLVMValueRef init_pick = build_random_array_element(arr, module, builder);
  LLVMBuildStore(builder, init_pick, val_ptr);
  LLVMBuildBr(builder, cont_bb);

  LLVMPositionBuilderAtEnd(builder, cont_bb);
  BUILD_ON_TRIG(builder, trig, "array_choose",
                LLVMValueRef pick =
                    build_random_array_element(arr, module, builder);
                LLVMBuildStore(builder, pick, val_ptr););

  return LLVMBuildLoad2(builder, f64_ty, val_ptr, "array_choose.out");
}

LLVMValueRef build_array_seq(LLVMValueRef arr, LLVMValueRef trig,
                             DspBuildCtx *dsp_ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  // State layout: val (f64, 8 bytes) + counter (i32, 4 bytes) = 12 bytes
  int off = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset = off + 12;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef i32_ptr_ty = LLVMPointerType(i32_ty, 0);

  LLVMValueRef base =
      dsp_consume_frame_state(dsp_ctx, builder, 12, 8, "array_seq.base");
  LLVMValueRef val_ptr =
      LLVMBuildBitCast(builder, base, f64_ptr_ty, "array_seq.val_ptr");
  LLVMValueRef counter_ptr_i8 = LLVMBuildGEP2(
      builder, i8_ty, base, (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
      "array_seq.counter_ptr_i8");
  LLVMValueRef counter_ptr = LLVMBuildBitCast(
      builder, counter_ptr_i8, i32_ptr_ty, "array_seq.counter_ptr");

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, 12, 8, "array_seq.init.base");
    LLVMValueRef init_val_ptr = LLVMBuildBitCast(
        dsp_ctx->init_builder, init_base, f64_ptr_ty, "array_seq.init.val_ptr");
    LLVMValueRef init_counter_ptr_i8 =
        LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_base,
                      (LLVMValueRef[]){LLVMConstInt(i64_ty, 8, 0)}, 1,
                      "array_seq.init.counter_ptr_i8");
    LLVMValueRef init_counter_ptr =
        LLVMBuildBitCast(dsp_ctx->init_builder, init_counter_ptr_i8, i32_ptr_ty,
                         "array_seq.init.counter_ptr");
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstReal(f64_ty, NAN),
                   init_val_ptr);
    LLVMBuildStore(dsp_ctx->init_builder, LLVMConstInt(i32_ty, 0, 0),
                   init_counter_ptr);
  }

  LLVMValueRef cur_val =
      LLVMBuildLoad2(builder, f64_ty, val_ptr, "array_seq.cur_val");
  LLVMValueRef is_uninit = LLVMBuildFCmp(builder, LLVMRealUNO, cur_val, cur_val,
                                         "array_seq.is_uninit");

  LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef fn_parent = LLVMGetBasicBlockParent(cur_bb);
  LLVMBasicBlockRef init_bb = LLVMAppendBasicBlock(fn_parent, "array_seq.init");
  LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlock(fn_parent, "array_seq.cont");
  LLVMBuildCondBr(builder, is_uninit, init_bb, cont_bb);

  // First call: counter = -1 so the first trig increments to 0, val = arr[0]
  LLVMPositionBuilderAtEnd(builder, init_bb);
  LLVMValueRef zero = LLVMConstInt(i32_ty, 0, 0);
  LLVMBuildStore(builder, LLVMConstInt(i32_ty, (uint64_t)-1, 1), counter_ptr);
  LLVMBuildStore(builder, get_array_element(builder, arr, zero, f64_ty),
                 val_ptr);
  LLVMBuildBr(builder, cont_bb);

  // On trig: counter = (counter + 1) % len, val = arr[counter]
  LLVMPositionBuilderAtEnd(builder, cont_bb);
  LLVMValueRef len_i32 = codegen_get_array_size(builder, arr, f64_ty);
  BUILD_ON_TRIG(
      builder, trig, "array_seq",
      LLVMValueRef cur_counter =
          LLVMBuildLoad2(builder, i32_ty, counter_ptr, "array_seq.counter");
      LLVMValueRef next_counter =
          LLVMBuildAdd(builder, cur_counter, LLVMConstInt(i32_ty, 1, 0),
                       "array_seq.next_counter");
      LLVMValueRef wrapped =
          LLVMBuildSRem(builder, next_counter, len_i32, "array_seq.wrapped");
      LLVMBuildStore(builder, wrapped, counter_ptr); LLVMBuildStore(
          builder, get_array_element(builder, arr, wrapped, f64_ty), val_ptr););

  return LLVMBuildLoad2(builder, f64_ty, val_ptr, "array_seq.out");
}

int min(int a, int b) {
  if (a >= b)
    return b;

  return a;
}

int max(int a, int b) {
  if (a <= b)
    return b;

  return a;
}

static inline int dsp_value_lane_count(DspValue v) {
  return v.lanes > 0 ? v.lanes : 0;
}

static inline LLVMValueRef dsp_value_lane(DspValue v, int i) {
  return v.lanes > 1 ? v.vec[i % v.lanes] : v.scalar;
}

static LLVMValueRef *dsp_alloc_lane_vals(DspBuildCtx *dsp_ctx, int lanes) {
  if (lanes <= 0) {
    return NULL;
  }
  return dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)lanes, 8);
}

typedef LLVMValueRef (*DspUnaryKernel)(LLVMValueRef x, void *userdata);

static DspValue dsp_lift_unary(DspBuildCtx *dsp_ctx, DspValue in,
                               DspUnaryKernel kernel, void *userdata) {
  int lanes = dsp_value_lane_count(in);
  if (lanes <= 0) {
    return DSP_NULL;
  }

  if (lanes == 1) {
    return DSP_SCALAR(kernel(in.scalar, userdata));
  }

  LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, lanes);
  if (!vals) {
    return DSP_NULL;
  }

  for (int i = 0; i < lanes; i++) {
    vals[i] = kernel(in.vec[i], userdata);
  }
  return DSP_MULTI(lanes, vals);
}

typedef LLVMValueRef (*DspBinaryKernel)(LLVMValueRef a, LLVMValueRef b,
                                        void *userdata);

static DspValue dsp_lift_binary(DspBuildCtx *dsp_ctx, DspValue a, DspValue b,
                                DspBinaryKernel kernel, void *userdata) {
  int lanes = max(dsp_value_lane_count(a), dsp_value_lane_count(b));
  if (lanes <= 0) {
    return DSP_NULL;
  }

  if (lanes == 1) {
    return DSP_SCALAR(
        kernel(dsp_value_lane(a, 0), dsp_value_lane(b, 0), userdata));
  }

  LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, lanes);
  if (!vals) {
    return DSP_NULL;
  }

  for (int i = 0; i < lanes; i++) {
    vals[i] = kernel(dsp_value_lane(a, i), dsp_value_lane(b, i), userdata);
  }
  return DSP_MULTI(lanes, vals);
}

typedef LLVMValueRef (*DspTernaryKernel)(LLVMValueRef a, LLVMValueRef b,
                                         LLVMValueRef c, void *userdata);

static DspValue dsp_lift_ternary(DspBuildCtx *dsp_ctx, DspValue a, DspValue b,
                                 DspValue c, DspTernaryKernel kernel,
                                 void *userdata) {
  int lanes = max(dsp_value_lane_count(a),
                  max(dsp_value_lane_count(b), dsp_value_lane_count(c)));
  if (lanes <= 0) {
    return DSP_NULL;
  }

  if (lanes == 1) {
    return DSP_SCALAR(kernel(dsp_value_lane(a, 0), dsp_value_lane(b, 0),
                             dsp_value_lane(c, 0), userdata));
  }

  LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, lanes);
  if (!vals) {
    return DSP_NULL;
  }

  for (int i = 0; i < lanes; i++) {
    vals[i] = kernel(dsp_value_lane(a, i), dsp_value_lane(b, i),
                     dsp_value_lane(c, i), userdata);
  }
  return DSP_MULTI(lanes, vals);
}

typedef struct DspValueList {
  DspValue val;
  Type *in_type;
  struct DspValueList *next;
} DspValueList;

typedef LLVMValueRef (*DspVariadicKernel)(LLVMValueRef *args, int argc,
                                          void *userdata);

static int dsp_value_list_len(DspValueList *l) {
  if (!l) {
    return 0;
  }
  return 1 + dsp_value_list_len(l->next);
}

static int dsp_value_list_max_lanes(DspValueList *l) {
  if (!l) {
    return 0;
  }
  int self_lanes = (l->in_type && !is_array_type(l->in_type))
                       ? dsp_value_lane_count(l->val)
                       : 1;
  return max(self_lanes, dsp_value_list_max_lanes(l->next));
}

static void dsp_value_list_collect_lane(DspValueList *l, int lane,
                                        LLVMValueRef *args, int *idx) {
  if (!l) {
    return;
  }
  int arg_lane = (l->in_type && !is_array_type(l->in_type)) ? lane : 0;
  args[*idx] = dsp_value_lane(l->val, arg_lane);
  (*idx)++;
  dsp_value_list_collect_lane(l->next, lane, args, idx);
}

static DspValue dsp_lift_variadic(DspBuildCtx *dsp_ctx, DspValueList *l,
                                  DspVariadicKernel kernel, void *userdata) {
  int argc = dsp_value_list_len(l);
  int lanes = dsp_value_list_max_lanes(l);
  if (argc <= 0 || lanes <= 0) {
    return DSP_NULL;
  }

  LLVMValueRef *call_args =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)argc, 8);
  if (!call_args) {
    return DSP_NULL;
  }

  if (lanes == 1) {
    int idx = 0;
    dsp_value_list_collect_lane(l, 0, call_args, &idx);
    return DSP_SCALAR(kernel(call_args, argc, userdata));
  }

  LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, lanes);
  if (!vals) {
    return DSP_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    int idx = 0;
    dsp_value_list_collect_lane(l, lane, call_args, &idx);
    vals[lane] = kernel(call_args, argc, userdata);
  }
  return DSP_MULTI(lanes, vals);
}
typedef struct {
  Type *ltype;
  Type *rtype;
  LLVMBuilderRef builder;
  LLVMValueRef (*int_op)(LLVMBuilderRef, LLVMValueRef, LLVMValueRef,
                         const char *);
  LLVMValueRef (*float_op)(LLVMBuilderRef, LLVMValueRef, LLVMValueRef,
                           const char *);
  const char *name;
} DspArithOp;

static LLVMValueRef dsp_apply_arith_kernel(LLVMValueRef l, LLVMValueRef r,
                                           void *userdata) {
  DspArithOp *op = userdata;
  bool both_ints = op->ltype->kind == T_INT && op->rtype->kind == T_INT;

  if (both_ints) {
    return op->int_op(op->builder, l, r, op->name);
  }

  l = ensure_float(op->ltype, l, op->builder);
  r = ensure_float(op->rtype, r, op->builder);
  return op->float_op(op->builder, l, r, op->name);
}

static DspValue
multi_chan_arith(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                 LLVMModuleRef module, LLVMBuilderRef builder,
                 LLVMValueRef (*int_op)(LLVMBuilderRef, LLVMValueRef,
                                        LLVMValueRef, const char *),
                 LLVMValueRef (*float_op)(LLVMBuilderRef, LLVMValueRef,
                                          LLVMValueRef, const char *),
                 const char *name) {
  Type *ltype = ast->data.AST_APPLICATION.args->type;
  Type *rtype = (ast->data.AST_APPLICATION.args + 1)->type;
  DspValue l = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                              module, builder);
  DspValue r = dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx, ctx,
                              module, builder);
  DspArithOp op = {.ltype = ltype,
                   .rtype = rtype,
                   .builder = builder,
                   .int_op = int_op,
                   .float_op = float_op,
                   .name = name};
  return dsp_lift_binary(dsp_ctx, l, r, dsp_apply_arith_kernel, &op);
}

typedef struct {
  Type *ltype;
  Type *rtype;
  LLVMBuilderRef builder;
  LLVMIntPredicate int_pred;
  LLVMRealPredicate float_pred;
  const char *name;
} DspCmpOp;

static LLVMValueRef dsp_apply_cmp_kernel(LLVMValueRef l, LLVMValueRef r,
                                         void *userdata) {
  DspCmpOp *op = userdata;
  bool both_ints = op->ltype->kind == T_INT && op->rtype->kind == T_INT;

  if (both_ints) {
    return LLVMBuildICmp(op->builder, op->int_pred, l, r, op->name);
  }

  l = ensure_float(op->ltype, l, op->builder);
  r = ensure_float(op->rtype, r, op->builder);
  return LLVMBuildFCmp(op->builder, op->float_pred, l, r, op->name);
}

static DspValue multi_chan_cmp(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder,
                               LLVMIntPredicate int_pred,
                               LLVMRealPredicate float_pred, const char *name) {
  Type *ltype = ast->data.AST_APPLICATION.args->type;
  Type *rtype = (ast->data.AST_APPLICATION.args + 1)->type;
  DspValue l = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                              module, builder);
  DspValue r = dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx, ctx,
                              module, builder);
  DspCmpOp op = {.ltype = ltype,
                 .rtype = rtype,
                 .builder = builder,
                 .int_pred = int_pred,
                 .float_pred = float_pred,
                 .name = name};
  return dsp_lift_binary(dsp_ctx, l, r, dsp_apply_cmp_kernel, &op);
}

typedef struct {
  const char *table_name;
  int table_size;
  Type *in_type;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspTabOscOp;

static LLVMValueRef dsp_apply_tab_osc_kernel(LLVMValueRef x, void *userdata) {
  DspTabOscOp *op = userdata;
  return _builtin_tab_osc(op->table_name, op->table_size,
                          ensure_float(op->in_type, x, op->builder),
                          op->dsp_ctx, op->ctx, op->module, op->builder);
}

typedef struct {
  Type *in_type;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspPhasorOp;

static LLVMValueRef dsp_apply_phasor_kernel(LLVMValueRef x, void *userdata) {
  DspPhasorOp *op = userdata;
  return builtin_phasor(ensure_float(op->in_type, x, op->builder), op->dsp_ctx,
                        op->ctx, op->module, op->builder);
}

typedef struct {
  Type *in_type;
  bool freq_is_const_zero;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspTrigOp;

static LLVMValueRef dsp_apply_trig_kernel(LLVMValueRef x, void *userdata) {
  DspTrigOp *op = userdata;
  return builtin_trig(ensure_float(op->in_type, x, op->builder),
                      op->freq_is_const_zero, op->dsp_ctx, op->ctx, op->module,
                      op->builder);
}

typedef struct {
  Type *freq_type;
  Type *mod_index_type;
  Type *mod_ratio_type;
  DspBuildCtx *dsp_ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspPmOp;

static LLVMValueRef dsp_apply_pm_kernel(LLVMValueRef freq,
                                        LLVMValueRef mod_index,
                                        LLVMValueRef mod_ratio,
                                        void *userdata) {
  DspPmOp *op = userdata;
  return build_pm_osc(ensure_float(op->freq_type, freq, op->builder),
                      ensure_float(op->mod_index_type, mod_index, op->builder),
                      ensure_float(op->mod_ratio_type, mod_ratio, op->builder),
                      op->dsp_ctx, op->module, op->builder);
}

typedef struct {
  Type *lag_secs_type;
  Type *input_type;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspLagOp;

static LLVMValueRef dsp_apply_lag_kernel(LLVMValueRef lag_secs,
                                         LLVMValueRef input, void *userdata) {
  DspLagOp *op = userdata;
  lag_secs = ensure_float(op->lag_secs_type, lag_secs, op->builder);
  input = ensure_float(op->input_type, input, op->builder);
  return build_lag(input, lag_secs, op->dsp_ctx, op->module, op->builder);
}

typedef struct {
  Type *t_type;
  Type *trig_type;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} DspDecayOp;

static LLVMValueRef dsp_apply_decay_kernel(LLVMValueRef t, LLVMValueRef trig,
                                           void *userdata) {
  DspDecayOp *op = userdata;
  t = ensure_float(op->t_type, t, op->builder);
  trig = ensure_float(op->trig_type, trig, op->builder);
  return build_exp_decay(t, trig, op->dsp_ctx, op->ctx, op->module,
                         op->builder);
}

static DspValue build_lag_op(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  if (ast->data.AST_APPLICATION.len < 2) {
    fprintf(stderr, "Error: lag expects 2 args (lag_secs, input)\n");
    return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
  }

  Ast *args = ast->data.AST_APPLICATION.args;
  DspValue lag_secs_v = dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
  DspValue input_v = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
  DspLagOp op = {.lag_secs_type = args[0].type,
                 .input_type = args[1].type,
                 .dsp_ctx = dsp_ctx,
                 .ctx = ctx,
                 .module = module,
                 .builder = builder};
  return dsp_lift_binary(dsp_ctx, lag_secs_v, input_v, dsp_apply_lag_kernel,
                         &op);
}

static DspValue build_delay_kernel_op(Ast *ast, DspBuildCtx *dsp_ctx,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      DelayKernelOp op) {
  Ast *args = ast->data.AST_APPLICATION.args;
  if (ast->data.AST_APPLICATION.len < 4) {
    fprintf(stderr, "Error: %s expects 4 args\n",
            ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);
    return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
  }

  DspValue delay_secs_v =
      dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
  DspValue fb_v = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);
  DspValue input_v = dsp_build_expr(args + 3, dsp_ctx, ctx, module, builder);

  int max_delay_lanes = delay_kernel_max_delay_lanes(args + 1);
  int lanes = max(max(delay_secs_v.lanes, fb_v.lanes),
                  max(input_v.lanes, max_delay_lanes));
  if (lanes <= 0) {
    return DSP_NULL;
  }

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef i32_ptr_ty = LLVMPointerType(i32_ty, 0);
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);
  LLVMTypeRef fn_ty = LLVMFunctionType(
      f64_ty,
      (LLVMTypeRef[]){arr_ty, f64_ty, f64_ty, i32_ptr_ty, f64_ty, f64_ty}, 6,
      0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, op.fn_name);
  if (!fn) {
    fn = LLVMAddFunction(module, op.fn_name, fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  LLVMValueRef *vals = lanes > 1 ? dsp_alloc_lane_vals(dsp_ctx, lanes) : NULL;
  if (lanes > 1 && !vals) {
    return DSP_NULL;
  }

  for (int i = 0; i < lanes; i++) {
    int32_t buf_size = 0;
    if (!eval_delay_buf_size_for_lane(args + 1, i, dsp_ctx, ctx, &buf_size)) {
      return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
    }

    DelayBufIR db = build_delay_buf_ir(dsp_ctx, builder, buf_size, true, true);

    LLVMValueRef delay_secs =
        ensure_float(args[0].type, dsp_value_lane(delay_secs_v, i), builder);
    LLVMValueRef fb =
        ensure_float(args[2].type, dsp_value_lane(fb_v, i), builder);
    LLVMValueRef input =
        ensure_float(args[3].type, dsp_value_lane(input_v, i), builder);

    LLVMValueRef call_args[] = {db.buf_arr,       delay_secs, dsp_ctx->spf,
                                db.write_pos_ptr, input,      fb};
    LLVMValueRef out =
        LLVMBuildCall2(builder, fn_ty, fn, call_args, 6, op.call_name);
    if (lanes == 1) {
      return DSP_SCALAR(out);
    }
    vals[i] = out;
  }

  return DSP_MULTI(lanes, vals);
}

DspValue multi_chan_mul(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildMul,
                          LLVMBuildFMul, "signal.mul");
}

typedef struct {
  Type *in_type;
  DspBuildCtx *dsp_ctx;
  JITLangCtx *ctx;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef tab;
  bool sample_scale;
} DspTabreadOp;

static LLVMValueRef dsp_apply_tabread_kernel(LLVMValueRef x, void *userdata) {
  DspTabreadOp *op = userdata;
  if (op->sample_scale) {
    return build_tabread_samp(op->tab,
                              ensure_float(op->in_type, x, op->builder),
                              op->dsp_ctx, op->ctx, op->module, op->builder);
  }

  return build_tabread(op->tab, ensure_float(op->in_type, x, op->builder),
                       op->dsp_ctx, op->ctx, op->module, op->builder);
}

DspValue dsp_fn_application(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *f = ast->data.AST_APPLICATION.function;

  // fprintf(stderr, "dsp_app: %s (scope=%d)\n", f->data.AST_IDENTIFIER.value,
  //         ctx->stack_ptr);

  if (is_ident(f, "spf")) {
    return DSP_SCALAR(dsp_ctx->spf);
  }

  if (is_ident(f, "samplerate")) {
    return DSP_SCALAR(LLVMConstInt(LLVMInt32Type(), dsp_ctx->sample_rate, 0));
  }

  if (is_ident(f, "sin_osc")) {
    DspValue freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                                   module, builder);
    DspTabOscOp op = {.table_name = "ylc_sin_table",
                      .table_size = SIN_TABSIZE,
                      .in_type = ast->data.AST_APPLICATION.args->type,
                      .dsp_ctx = dsp_ctx,
                      .ctx = ctx,
                      .module = module,
                      .builder = builder};
    return dsp_lift_unary(dsp_ctx, freq, dsp_apply_tab_osc_kernel, &op);
  }

  if (is_ident(f, "sq_osc")) {
    DspValue freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                                   module, builder);
    DspTabOscOp op = {.table_name = "ylc_sq_table",
                      .table_size = SQ_TABSIZE,
                      .in_type = ast->data.AST_APPLICATION.args->type,
                      .dsp_ctx = dsp_ctx,
                      .ctx = ctx,
                      .module = module,
                      .builder = builder};
    return dsp_lift_unary(dsp_ctx, freq, dsp_apply_tab_osc_kernel, &op);
  }

  if (is_ident(f, "saw_osc")) {
    DspValue freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                                   module, builder);
    DspTabOscOp op = {.table_name = "ylc_saw_table",
                      .table_size = SAW_TABSIZE,
                      .in_type = ast->data.AST_APPLICATION.args->type,
                      .dsp_ctx = dsp_ctx,
                      .ctx = ctx,
                      .module = module,
                      .builder = builder};
    return dsp_lift_unary(dsp_ctx, freq, dsp_apply_tab_osc_kernel, &op);
  }

  if (is_ident(f, "phasor")) {
    DspValue freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx,
                                   module, builder);
    DspPhasorOp op = {.in_type = ast->data.AST_APPLICATION.args->type,
                      .dsp_ctx = dsp_ctx,
                      .ctx = ctx,
                      .module = module,
                      .builder = builder};
    return dsp_lift_unary(dsp_ctx, freq, dsp_apply_phasor_kernel, &op);
  }

  if (is_ident(f, "pm_osc")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    DspValue mod_index =
        dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
    DspValue mod_ratio =
        dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
    DspValue freq = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);

    int max_lanes =
        max(dsp_value_lane_count(mod_index),
            max(dsp_value_lane_count(mod_ratio), dsp_value_lane_count(freq)));
    if (max_lanes <= 0) {
      return DSP_NULL;
    }

    if (max_lanes == 1) {
      return DSP_SCALAR(build_pm_osc(
          ensure_float(args[2].type, dsp_value_lane(freq, 0), builder),
          ensure_float(args[0].type, dsp_value_lane(mod_index, 0), builder),
          ensure_float(args[1].type, dsp_value_lane(mod_ratio, 0), builder),
          dsp_ctx, module, builder));
    }

    LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, max_lanes);
    if (!vals) {
      return DSP_NULL;
    }

    for (int i = 0; i < max_lanes; i++) {
      vals[i] = build_pm_osc(
          ensure_float(args[2].type, dsp_value_lane(freq, i), builder),
          ensure_float(args[0].type, dsp_value_lane(mod_index, i), builder),
          ensure_float(args[1].type, dsp_value_lane(mod_ratio, i), builder),
          dsp_ctx, module, builder);
    }

    return DSP_MULTI(max_lanes, vals);
  }

  if (is_ident(f, "phase_osc")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    DspValue phase_input =
        dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
    DspValue freq = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);

    int max_lanes =
        max(dsp_value_lane_count(phase_input), dsp_value_lane_count(freq));
    if (max_lanes <= 0) {
      return DSP_NULL;
    }

    if (max_lanes == 1) {
      return DSP_SCALAR(build_phase_osc(
          ensure_float(args[0].type, dsp_value_lane(phase_input, 0), builder),
          ensure_float(args[1].type, dsp_value_lane(freq, 0), builder), dsp_ctx,
          module, builder));
    }

    LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, max_lanes);
    if (!vals) {
      return DSP_NULL;
    }

    for (int i = 0; i < max_lanes; i++) {
      vals[i] = build_phase_osc(
          ensure_float(args[0].type, dsp_value_lane(phase_input, i), builder),
          ensure_float(args[1].type, dsp_value_lane(freq, i), builder), dsp_ctx,
          module, builder);
    }

    return DSP_MULTI(max_lanes, vals);
  }
  if (is_ident(f, "bank_osc")) {

    Ast *args = ast->data.AST_APPLICATION.args;

    DspValue amps = dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
    DspValue phases = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
    DspValue freq = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);

    int max_lanes = dsp_value_lane_count(freq);
    if (max_lanes <= 0) {
      return DSP_NULL;
    }

    if (max_lanes == 1) {
      return DSP_SCALAR(build_bank_osc(
          amps.scalar, phases.scalar,
          ensure_float(args[2].type, dsp_value_lane(freq, 0), builder), dsp_ctx,
          module, builder));
    }

    LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, max_lanes);
    if (!vals) {
      return DSP_NULL;
    }

    for (int i = 0; i < max_lanes; i++) {
      vals[i] = build_bank_osc(
          amps.scalar, phases.scalar,
          ensure_float(args[2].type, dsp_value_lane(freq, i), builder), dsp_ctx,
          module, builder);
    }

    return DSP_MULTI(max_lanes, vals);
  }

  if (is_ident(f, "phasor_sinc")) {
    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder)
                            .scalar;
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);

    LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                       dsp_ctx, ctx, module, builder)
                            .scalar;
    return DSP_SCALAR(
        builtin_phasor_sinc(freq, trig, dsp_ctx, ctx, module, builder));
  }

  if (is_ident(f, "trig")) {
    Ast *freq_ast = ast->data.AST_APPLICATION.args;
    DspValue freq = dsp_build_expr(freq_ast, dsp_ctx, ctx, module, builder);
    DspTrigOp op = {.in_type = freq_ast->type,
                    .freq_is_const_zero =
                        ast_is_const_zero(freq_ast, dsp_ctx, ctx),
                    .dsp_ctx = dsp_ctx,
                    .ctx = ctx,
                    .module = module,
                    .builder = builder};
    return dsp_lift_unary(dsp_ctx, freq, dsp_apply_trig_kernel, &op);
  }
  if (is_ident(f, "+")) {
    return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildAdd,
                            LLVMBuildFAdd, "signal.add");
  }

  if (is_ident(f, "-")) {
    return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildSub,
                            LLVMBuildFSub, "signal.sub");
  }

  if (is_ident(f, "*")) {

    return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildMul,
                            LLVMBuildFMul, "signal.mul");
  }

  if (is_ident(f, "/")) {
    return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildSDiv,
                            LLVMBuildFDiv, "signal.div");
  }

  if (is_ident(f, "%")) {
    return multi_chan_arith(ast, dsp_ctx, ctx, module, builder, LLVMBuildSRem,
                            LLVMBuildFRem, "signal.rem");
  }
  if (is_ident(f, ">")) {
    return multi_chan_cmp(ast, dsp_ctx, ctx, module, builder, LLVMIntSGT,
                          LLVMRealOGT, "signal.gt");
  }

  if (is_ident(f, ">=")) {
    return multi_chan_cmp(ast, dsp_ctx, ctx, module, builder, LLVMIntSGE,
                          LLVMRealOGE, "signal.ge");
  }

  if (is_ident(f, "<")) {
    return multi_chan_cmp(ast, dsp_ctx, ctx, module, builder, LLVMIntSLT,
                          LLVMRealOLT, "signal.lt");
  }

  if (is_ident(f, "<=")) {
    return multi_chan_cmp(ast, dsp_ctx, ctx, module, builder, LLVMIntSLE,
                          LLVMRealOLE, "signal.le");
  }

  if (is_ident(f, "lag")) {
    return build_lag_op(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "arr_choose")) {

    LLVMValueRef array = dsp_build_expr(ast->data.AST_APPLICATION.args + 0,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;
    LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                       dsp_ctx, ctx, module, builder)
                            .scalar;
    trig = ensure_float(ast->data.AST_APPLICATION.args[1].type, trig, builder);

    return DSP_SCALAR(
        build_array_choose(array, trig, dsp_ctx, module, builder));
  }

  if (is_ident(f, "arr_seq")) {

    LLVMValueRef array = dsp_build_expr(ast->data.AST_APPLICATION.args + 0,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;
    LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                       dsp_ctx, ctx, module, builder)
                            .scalar;
    trig = ensure_float(ast->data.AST_APPLICATION.args[1].type, trig, builder);

    return DSP_SCALAR(build_array_seq(array, trig, dsp_ctx, module, builder));
  }

  if (is_ident(f, "white")) {

    LLVMTypeRef wn_ty = LLVMFunctionType(
        LLVMDoubleType(), (LLVMTypeRef[]){LLVMDoubleType(), LLVMDoubleType()},
        2, 0);
    LLVMValueRef wn_fn = LLVMGetNamedFunction(module, "rand_double_range");
    if (!wn_fn) {
      wn_fn = LLVMAddFunction(module, "rand_double_range", wn_ty);
      LLVMSetLinkage(wn_fn, LLVMExternalLinkage);
    }

    return DSP_SCALAR(
        LLVMBuildCall2(builder, wn_ty, wn_fn,
                       (LLVMValueRef[]){LLVMConstReal(LLVMDoubleType(), -1.),
                                        LLVMConstReal(LLVMDoubleType(), 1.)},
                       2, "white_noise.sample"));
  }

  if (is_ident(f, "kill_on_end")) {
    LLVMValueRef signal =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    return DSP_SCALAR(build_kill_on_end(signal, dsp_ctx, module, builder));
  }

  if (is_ident(f, "adsr")) {
    LLVMValueRef attack =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef decay =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef sustain =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef release =
        ensure_float(ast->data.AST_APPLICATION.args[3].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 3, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef trig =
        ensure_float(ast->data.AST_APPLICATION.args[4].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 4, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    return DSP_SCALAR(build_adsr(attack, decay, sustain, release, trig, dsp_ctx,
                                 module, builder));
  }

  if (strcmp(f->data.AST_IDENTIFIER.value, "rect") == 0) {
    LLVMValueRef duration =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef trig =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    return DSP_SCALAR(build_rect(duration, trig, dsp_ctx, module, builder));
  }

  if (is_ident(f, "delay")) {
    return build_delay_kernel_op(ast, dsp_ctx, ctx, module, builder,
                                 (DelayKernelOp){.fn_name = "delay_sample",
                                                 .call_name = "delay.sample"});
  }

  if (is_ident(f, "array_size")) {
    LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder)
                           .scalar;
    LLVMValueRef len_i32 =
        codegen_get_array_size(builder, arr, LLVMDoubleType());
    return DSP_SCALAR(len_i32);
  }

  if (is_ident(f, "delay_line")) {

    Ast *args = ast->data.AST_APPLICATION.args;
    // LLVMValueRef size = ensure_float(
    //     args[0].type, dsp_build_expr(args + 0, dsp_ctx, ctx, module,
    //     builder), builder);

    int32_t buf_size = 0;
    if (!eval_delay_buf_size(args, dsp_ctx, ctx, &buf_size)) {
      return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
    }

    DelayBufIR db = build_delay_buf_ir(dsp_ctx, builder, buf_size, true, false);

    return DSP_SCALAR(db.buf_arr);
  }

  if (is_ident(f, "comb")) {
    return build_delay_kernel_op(
        ast, dsp_ctx, ctx, module, builder,
        (DelayKernelOp){.fn_name = "comb_sample", .call_name = "comb.sample"});
  }

  // if (is_ident(f, "allpass") || is_ident(f, "allpass1")) {
  if (is_ident(f, "allpass")) {
    return build_delay_kernel_op(
        ast, dsp_ctx, ctx, module, builder,
        (DelayKernelOp){.fn_name = "allpass1_sample",
                        .call_name = "allpass.sample"});
  }

  if (is_ident(f, "tabread")) {
    LLVMValueRef table = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder)
                             .scalar;

    DspValue phase = dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder);
    // .scalar;
    // return DSP_SCALAR(
    //     build_tabread(table, phase, dsp_ctx, ctx, module, builder));

    DspTabreadOp op = {.in_type = (ast->data.AST_APPLICATION.args + 1)->type,
                       .dsp_ctx = dsp_ctx,
                       .ctx = ctx,
                       .module = module,
                       .builder = builder,
                       .sample_scale = false,
                       .tab = table};
    return dsp_lift_unary(dsp_ctx, phase, dsp_apply_tabread_kernel, &op);
  }

  if (is_ident(f, "tabread_samp")) {

    LLVMValueRef table = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder)
                             .scalar;

    DspValue phase = dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder);
    // .scalar;
    // return DSP_SCALAR(
    //     build_tabread(table, phase, dsp_ctx, ctx, module, builder));

    DspTabreadOp op = {.in_type = (ast->data.AST_APPLICATION.args + 1)->type,
                       .dsp_ctx = dsp_ctx,
                       .ctx = ctx,
                       .module = module,
                       .builder = builder,
                       .sample_scale = true,
                       .tab = table};
    return dsp_lift_unary(dsp_ctx, phase, dsp_apply_tabread_kernel, &op);
  }

  if (is_ident(f, "array_set")) {
    // print_type((ast->data.AST_APPLICATION.args + 2)->type);
    // printf("ARRAY_SET - if operand is symbol then mark it as mutable thus "
    //        "needs to be in state scope\n");
    // print_ast(ast);

    Type *arr_type = ast->data.AST_APPLICATION.args->type;
    Type *el_type = arr_type->data.T_CONS.args[0];

    LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder)
                           .scalar;

    LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;

    LLVMValueRef value = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;

    LLVMTypeRef el_llvm_ty = type_to_llvm_type(el_type, ctx, module);
    if (LLVMGetTypeKind(el_llvm_ty) == LLVMIntegerTypeKind &&
        LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMDoubleTypeKind) {
      value = LLVMBuildFPToSI(builder, value, el_llvm_ty, "f2i");
    }
    set_array_element(builder, arr, index, value, el_llvm_ty);
    return DSP_SCALAR(arr);
  }

  if (is_ident(f, "array_at")) {

    LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                      ctx, module, builder)
                           .scalar;
    LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;
    // print_type(ast->type);

    return DSP_SCALAR(get_array_element(
        builder, arr, index, type_to_llvm_type(ast->type, ctx, module)));
  }
  if (is_ident(f, "decay")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    DspValue t_v = dsp_build_expr(args + 0, dsp_ctx, ctx, module, builder);
    DspValue trig_v = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
    DspDecayOp op = {.t_type = args[0].type,
                     .trig_type = args[1].type,
                     .dsp_ctx = dsp_ctx,
                     .ctx = ctx,
                     .module = module,
                     .builder = builder};
    return dsp_lift_binary(dsp_ctx, t_v, trig_v, dsp_apply_decay_kernel, &op);
  }
  if (is_ident(f, "scale")) {
    // unipolar scale - ie [0,1] -> [a, b]
    LLVMValueRef lo =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);

    LLVMValueRef hi =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);

    LLVMValueRef v =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");
    LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    return DSP_SCALAR(LLVMBuildFAdd(builder, lo, scaled, "scale.out"));
  }

  if (is_ident(f, "scale_bp")) {
    // bipolar scale - ie [-1,1] -> [a, b]

    LLVMValueRef lo =
        ensure_float(ast->data.AST_APPLICATION.args->type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);

    LLVMValueRef hi =
        ensure_float(ast->data.AST_APPLICATION.args[1].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);

    LLVMValueRef v =
        ensure_float(ast->data.AST_APPLICATION.args[2].type,
                     dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx,
                                    ctx, module, builder)
                         .scalar,
                     builder);
    LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");

    v = LLVMBuildFMul(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
                      "input.half");

    v = LLVMBuildFAdd(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
                      "input.add_half");
    LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    return DSP_SCALAR(LLVMBuildFAdd(builder, lo, scaled, "scale.out"));
  }

  if (is_ident(f, "lfnoise")) {
    // linearly interpolated noise [0, 1)
    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder)
                            .scalar;

    LLVMValueRef lo = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                     dsp_ctx, ctx, module, builder)
                          .scalar;

    LLVMValueRef hi = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                     dsp_ctx, ctx, module, builder)
                          .scalar;
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
    lo = ensure_float((ast->data.AST_APPLICATION.args + 1)->type, lo, builder);
    hi = ensure_float((ast->data.AST_APPLICATION.args + 2)->type, hi, builder);

    return DSP_SCALAR(
        build_lfnoise_lin(freq, lo, hi, dsp_ctx, ctx, module, builder));
  }

  if (is_ident(f, "lfnoise0")) {

    // step noise [0, 1)

    LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                       ctx, module, builder)
                            .scalar;

    LLVMValueRef lo = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                     dsp_ctx, ctx, module, builder)
                          .scalar;

    LLVMValueRef hi = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                     dsp_ctx, ctx, module, builder)
                          .scalar;
    freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq, builder);
    lo = ensure_float((ast->data.AST_APPLICATION.args + 1)->type, lo, builder);
    hi = ensure_float((ast->data.AST_APPLICATION.args + 2)->type, hi, builder);

    return DSP_SCALAR(
        build_lfnoise_step(freq, lo, hi, dsp_ctx, ctx, module, builder));
  }

  JITSymbol *callable_sym =
      lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (callable_sym && callable_sym->type == STYPE_AUDIO_JIT_SYM) {

    int synth_id = audio_sym_synth_id(callable_sym);
    SynthRecord rec = synth_registry_get(synth_id);

    return call_registered_synth_in_audio_fn(ast, rec, dsp_ctx, ctx, module,
                                             builder);
  }

  if (is_ident(f, "bufplay")) {
    Ast *args = ast->data.AST_APPLICATION.args;

    DspValue buf = dsp_build_expr(args, dsp_ctx, ctx, module, builder);

    DspValue rate = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
    DspValue start_pos =
        dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);
    DspValue trig = dsp_build_expr(args + 3, dsp_ctx, ctx, module, builder);

    int max_lanes =
        max(dsp_value_lane_count(buf),
            max(dsp_value_lane_count(rate), max(dsp_value_lane_count(start_pos),
                                                dsp_value_lane_count(trig))));

    if (max_lanes <= 1) {
      return build_bufplay(
          dsp_value_lane(buf, 0),
          ensure_float(args[1].type, dsp_value_lane(rate, 0), builder),
          ensure_float(args[2].type, dsp_value_lane(start_pos, 0), builder),
          ensure_float(args[3].type, dsp_value_lane(trig, 0), builder), 1,
          dsp_ctx, ctx, module, builder);
    }

    LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, max_lanes);
    if (!vals) {
      return DSP_NULL;
    }

    for (int i = 0; i < max_lanes; i++) {
      vals[i] =
          build_bufplay(
              dsp_value_lane(buf, i),
              ensure_float(args[1].type, dsp_value_lane(rate, i), builder),
              ensure_float(args[2].type, dsp_value_lane(start_pos, i), builder),
              ensure_float(args[3].type, dsp_value_lane(trig, i), builder), 1,
              dsp_ctx, ctx, module, builder)
              .scalar;
    }

    return DSP_MULTI(max_lanes, vals);
  }

  if (is_ident(f, "mbufplay")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    double num_channels_num = 0.0;
    if (!ast_try_eval_const_num(&args[0], dsp_ctx, ctx, &num_channels_num)) {
      fprintf(stderr,
              "Error: mbufplay expects a constant num_channels argument\n");
      return DSP_NULL;
    }

    int num_channels = (int)num_channels_num;
    if (num_channels <= 0) {
      fprintf(stderr, "Error: mbufplay num_channels must be > 0\n");
      return DSP_NULL;
    }

    DspValue buf_v = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
    DspValue rate_v = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);
    DspValue start_pos_v =
        dsp_build_expr(args + 3, dsp_ctx, ctx, module, builder);
    DspValue trig_v = dsp_build_expr(args + 4, dsp_ctx, ctx, module, builder);

    LLVMValueRef buf = dsp_value_lane(buf_v, 0);
    LLVMValueRef rate =
        ensure_float(args[2].type, dsp_value_lane(rate_v, 0), builder);
    LLVMValueRef start_pos =
        ensure_float(args[3].type, dsp_value_lane(start_pos_v, 0), builder);
    LLVMValueRef trig =
        ensure_float(args[4].type, dsp_value_lane(trig_v, 0), builder);

    return build_bufplay(buf, rate, start_pos, trig, num_channels, dsp_ctx, ctx,
                         module, builder);
  }

  if (is_ident(f, "grains")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    double max_grains_num = 0.0;
    if (!ast_try_eval_const_num(&args[0], dsp_ctx, ctx, &max_grains_num)) {
      fprintf(stderr, "Error - max_grains needs to be a constant\n");
      return DSP_NULL;
    }

    int32_t max_grains = (int32_t)max_grains_num;
    if (max_grains <= 0) {
      return DSP_SCALAR(LLVMConstReal(LLVMDoubleType(), 0.0));
    }

    LLVMValueRef buf =
        dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder).scalar;
    DspValue rate = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);
    DspValue pos = dsp_build_expr(args + 3, dsp_ctx, ctx, module, builder);
    DspValue width = dsp_build_expr(args + 4, dsp_ctx, ctx, module, builder);
    DspValue trig = dsp_build_expr(args + 5, dsp_ctx, ctx, module, builder);

    int max_lanes =
        max(dsp_value_lane_count(rate),
            max(dsp_value_lane_count(pos),
                max(dsp_value_lane_count(width), dsp_value_lane_count(trig))));

    if (max_lanes <= 1) {
      return DSP_SCALAR(build_grains(
          max_grains, buf,
          ensure_float(args[2].type, dsp_value_lane(rate, 0), builder),
          ensure_float(args[3].type, dsp_value_lane(pos, 0), builder),
          ensure_float(args[4].type, dsp_value_lane(width, 0), builder),
          ensure_float(args[5].type, dsp_value_lane(trig, 0), builder), dsp_ctx,
          module, builder));
    }

    LLVMValueRef *vals = dsp_alloc_lane_vals(dsp_ctx, max_lanes);
    if (!vals) {
      return DSP_NULL;
    }

    for (int i = 0; i < max_lanes; i++) {
      vals[i] = build_grains(
          max_grains, buf,
          ensure_float(args[2].type, dsp_value_lane(rate, i), builder),
          ensure_float(args[3].type, dsp_value_lane(pos, i), builder),
          ensure_float(args[4].type, dsp_value_lane(width, i), builder),
          ensure_float(args[5].type, dsp_value_lane(trig, i), builder), dsp_ctx,
          module, builder);
    }

    return DSP_MULTI(max_lanes, vals);
  }

  if (is_ident(f, "array_fill_const")) {
    Ast *args = ast->data.AST_APPLICATION.args;
    Type *arg_type = args->type;

    if (!has_attr(arg_type->attr, ATTR_COMPILE_TIME_CONST)) {
      fprintf(stderr, "Error: not implemented- emit non-constant / computed "
                      "array length instructions\n");
      print_ast_err(ast);
      return DSP_NULL;
    }

    int len;
    if (args->tag == AST_INT) {
      len = args->data.AST_INT.value;
    } else if (args->tag == AST_IDENTIFIER && is_ident(args, "fft_size")) {
      len = dsp_ctx->spectral_fft_size;
    } else {
      fprintf(stderr,
              "Error: could not determine array size at compile time\n");
    }

    dsp_ctx->array_attrs.comptime_size = len;

    Type *el_type = ast->type->data.T_CONS.args[0];
    LLVMTypeRef el_llvm_ty = type_to_llvm_type(el_type, ctx, module);
    LLVMTypeRef arr_ty = codegen_array_type(el_llvm_ty);
    LLVMTypeRef arr_ptr_ty = LLVMPointerType(arr_ty, 0);
    LLVMTypeRef el_ptr_ty = LLVMPointerType(el_llvm_ty, 0);

    LLVMTypeRef i8_ty = LLVMInt8Type();
    LLVMTypeRef i32_ty = LLVMInt32Type();
    LLVMTypeRef i64_ty = LLVMInt64Type();

    LLVMTargetDataRef data_layout = LLVMGetModuleDataLayout(module);
    int el_size = (int)LLVMABISizeOfType(data_layout, el_llvm_ty);
    int arr_size = (int)LLVMABISizeOfType(data_layout, arr_ty);

    bool hoist_to_synth_lifetime =
        ast->ea_md && (ast->ea_md->attributes & EA_ATTR_MUTABLE);

    if (hoist_to_synth_lifetime) {
      int off = (dsp_ctx->state_offset + 7) & ~7;
      int data_off = off + arr_size;
      int total_bytes = arr_size + (len * el_size);
      dsp_ctx->state_offset = data_off + (len * el_size);

      if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
        LLVMValueRef init_base = dsp_consume_init_state(
            dsp_ctx, dsp_ctx->init_builder, total_bytes, 8, "array.ctor.base");
        LLVMValueRef arr_ptr = LLVMBuildBitCast(
            dsp_ctx->init_builder, init_base, arr_ptr_ty, "array.ctor.ptr");
        LLVMValueRef ctor_base_i8 = LLVMBuildGEP2(
            dsp_ctx->init_builder, i8_ty, init_base,
            (LLVMValueRef[]){LLVMConstInt(i64_ty, (uint64_t)arr_size, 0)}, 1,
            "array.ctor.base");
        LLVMValueRef ctor_base = LLVMBuildBitCast(
            dsp_ctx->init_builder, ctor_base_i8, el_ptr_ty, "array.ctor.data");

        LLVMValueRef arr_init = LLVMGetUndef(arr_ty);
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        LLVMConstInt(i32_ty, (uint64_t)len, 0),
                                        0, "array.ctor.size");
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        ctor_base, 1, "array.ctor.data_ptr");
        LLVMBuildStore(dsp_ctx->init_builder, arr_init, arr_ptr);

        LLVMValueRef fill_elem = dsp_build_expr(args + 1, dsp_ctx, ctx, module,
                                                dsp_ctx->init_builder)
                                     .scalar;
        fill_elem =
            handle_type_conversions(fill_elem, (args + 1)->type, el_type, ctx,
                                    module, dsp_ctx->init_builder);

        for (int i = 0; i < len; i++) {
          LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
          LLVMValueRef elem_ptr =
              LLVMBuildGEP2(dsp_ctx->init_builder, el_llvm_ty, ctor_base,
                            &idx_i64, 1, "array.init.ptr");
          LLVMBuildStore(dsp_ctx->init_builder, fill_elem, elem_ptr);
        }
      }

      LLVMValueRef arr_ptr_i8 = dsp_consume_frame_state(
          dsp_ctx, builder, total_bytes, 8, "array.ptr");
      LLVMValueRef arr_ptr =
          LLVMBuildBitCast(builder, arr_ptr_i8, arr_ptr_ty, "array.ptr");
      return DSP_SCALAR(LLVMBuildLoad2(builder, arr_ty, arr_ptr, "array.load"));
    }

    // Non-hoisted: allocate on frame stack and fill each frame
    LLVMValueRef len_i32 = LLVMConstInt(i32_ty, (uint64_t)len, 0);
    LLVMValueRef frame_data_ptr =
        LLVMBuildArrayAlloca(builder, el_llvm_ty, len_i32, "array.frame.data");

    LLVMValueRef fill_elem =
        dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder).scalar;
    fill_elem = handle_type_conversions(fill_elem, (args + 1)->type, el_type,
                                        ctx, module, builder);

    for (int i = 0; i < len; i++) {
      LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
      LLVMValueRef elem_ptr = LLVMBuildGEP2(builder, el_llvm_ty, frame_data_ptr,
                                            &idx_i64, 1, "array.frame.ptr");
      LLVMBuildStore(builder, fill_elem, elem_ptr);
    }

    LLVMValueRef arr_val = LLVMGetUndef(arr_ty);
    arr_val = LLVMBuildInsertValue(builder, arr_val, len_i32, 0, "array.size");
    arr_val =
        LLVMBuildInsertValue(builder, arr_val, frame_data_ptr, 1, "array.data");
    return DSP_SCALAR(arr_val);
  }

  if (is_ident(f, "pv_env")) {
    return dsp_spectral_env(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "pv_env_real")) {
    return dsp_spectral_env_real(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "pv_scale_mag")) {
    return dsp_spectral_scale_mag(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "pv_div_mag")) {
    return dsp_spectral_div_mag(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "pv_bin_scramble")) {
    return dsp_spectral_bin_scramble(ast, dsp_ctx, ctx, module, builder);
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

        DspValue arg_val = dsp_build_expr(ast->data.AST_APPLICATION.args + idx,
                                          dsp_ctx, &lctx, module, builder);
        LLVMValueRef scalar_arg = arg_val.scalar;
        if (types_equal(param_type, &t_num)) {
          if (arg_val.lanes > 1) {
            for (int lane = 0; lane < arg_val.lanes; lane++) {
              arg_val.vec[lane] =
                  ensure_float(ast->data.AST_APPLICATION.args[idx].type,
                               arg_val.vec[lane], builder);
            }
            arg_val.scalar = arg_val.vec[0];
            scalar_arg = arg_val.scalar;
          } else {
            scalar_arg = ensure_float(ast->data.AST_APPLICATION.args[idx].type,
                                      scalar_arg, builder);
            arg_val.scalar = scalar_arg;
          }
        }

        JITSymbol *sym = new_symbol(
            (symbol_type)STYPE_AUDIO_JIT_DSP_VALUE, param_type, scalar_arg,
            type_to_llvm_type(param_type, &lctx, module));
        DspValue *stored =
            dsp_ctx ? dsp_tmp_alloc(dsp_ctx, sizeof(DspValue), 8) : NULL;
        if (stored) {
          *stored = arg_val;
          sym->symbol_data._USER_DEFINED_SYMBOL = stored;
        }

        const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
        int id_len = param_ast->data.AST_IDENTIFIER.length;
        ht_set_hash(lctx.frame->table, id_chars, hash_string(id_chars, id_len),
                    sym);

        ltype = ltype->data.T_FN.to;
      }
    }

    DspValue res = dsp_build_expr(lambda_ast->data.AST_LAMBDA.body, dsp_ctx,
                                  &lctx, module, builder);

    destroy_ctx(&lctx);

    return res;
  }

  if (is_ident(f, "vec_dot")) {

    LLVMValueRef vec_a = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                        ctx, module, builder)
                             .scalar;
    LLVMValueRef vec_b = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                        dsp_ctx, ctx, module, builder)
                             .scalar;

    LLVMTypeRef f64_ty = LLVMDoubleType();
    LLVMTypeRef i32_ty = LLVMInt32Type();
    LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

    // void dsp_matrix_vec_mul(int rows, int cols, double*, double*, double*)
    LLVMTypeRef mmul_fn_ty =
        LLVMFunctionType(LLVMDoubleType(),
                         (LLVMTypeRef[]){i32_ty, f64_ptr_ty, f64_ptr_ty}, 3, 0);

    LLVMValueRef dot_fn = LLVMGetNamedFunction(module, "dsp_vec_dot");
    if (!dot_fn) {
      dot_fn = LLVMAddFunction(module, "dsp_vec_dot", mmul_fn_ty);
      LLVMSetLinkage(dot_fn, LLVMExternalLinkage);
    }

    // cols = size of input vec, rows = size of output
    //
    LLVMValueRef a_data =
        LLVMBuildExtractValue(builder, vec_a, 1, "vec_data_ptr");

    LLVMValueRef b_data =
        LLVMBuildExtractValue(builder, vec_b, 1, "vec_data_ptr");
    LLVMValueRef cols = LLVMBuildExtractValue(builder, vec_a, 0, "vec_size");
    return DSP_SCALAR(LLVMBuildCall2(builder, mmul_fn_ty, dot_fn,
                                     (LLVMValueRef[]){cols, a_data, b_data}, 3,
                                     "vec_dot_prod"));
  }

  if (is_ident(f, "dsp_mmul_op")) {
    LLVMTypeRef f64_ty = LLVMDoubleType();
    LLVMTypeRef i32_ty = LLVMInt32Type();
    LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);

    LLVMValueRef matrix = dsp_build_expr(ast->data.AST_APPLICATION.args,
                                         dsp_ctx, ctx, module, builder)
                              .scalar;

    LLVMValueRef vec = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
                                      dsp_ctx, ctx, module, builder)
                           .scalar;
    LLVMValueRef out = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
                                      dsp_ctx, ctx, module, builder)
                           .scalar;

    // All three are flat array structs: { i32 size, f64* data }
    LLVMTypeRef arr_ty = codegen_array_type(f64_ty);

    LLVMValueRef vec_struct =
        (LLVMGetTypeKind(LLVMTypeOf(vec)) == LLVMPointerTypeKind)
            ? LLVMBuildLoad2(builder, arr_ty, vec, "vec_struct")
            : vec;
    LLVMValueRef out_struct =
        (LLVMGetTypeKind(LLVMTypeOf(out)) == LLVMPointerTypeKind)
            ? LLVMBuildLoad2(builder, arr_ty, out, "out_struct")
            : out;
    LLVMValueRef mat_struct =
        (LLVMGetTypeKind(LLVMTypeOf(matrix)) == LLVMPointerTypeKind)
            ? LLVMBuildLoad2(builder, arr_ty, matrix, "mat_struct")
            : matrix;

    // cols = size of input vec, rows = size of output
    LLVMValueRef cols =
        LLVMBuildExtractValue(builder, vec_struct, 0, "vec_size");
    LLVMValueRef rows =
        LLVMBuildExtractValue(builder, out_struct, 0, "out_size");

    LLVMValueRef matrix_data =
        LLVMBuildExtractValue(builder, mat_struct, 1, "mat_data_ptr");
    LLVMValueRef vec_data =
        LLVMBuildExtractValue(builder, vec_struct, 1, "vec_data_ptr");
    LLVMValueRef out_data =
        LLVMBuildExtractValue(builder, out_struct, 1, "out_data_ptr");

    // void dsp_matrix_vec_mul(int rows, int cols, double*, double*, double*)
    LLVMTypeRef mmul_fn_ty = LLVMFunctionType(
        LLVMVoidType(),
        (LLVMTypeRef[]){i32_ty, i32_ty, f64_ptr_ty, f64_ptr_ty, f64_ptr_ty}, 5,
        0);
    LLVMValueRef mmul_fn = LLVMGetNamedFunction(module, "dsp_matrix_vec_mul");
    if (!mmul_fn) {
      mmul_fn = LLVMAddFunction(module, "dsp_matrix_vec_mul", mmul_fn_ty);
      LLVMSetLinkage(mmul_fn, LLVMExternalLinkage);
    }

    LLVMBuildCall2(
        builder, mmul_fn_ty, mmul_fn,
        (LLVMValueRef[]){rows, cols, matrix_data, vec_data, out_data}, 5, "");
    return DSP_SCALAR(out);
  }

  if (is_ident(f, "delay_proc")) {
    return dsp_delay_proc(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, _FORK_OPERATOR_ID)) {
    return dsp_fork(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, _FORK_MIX_ID)) {
    return dsp_mix(ast, dsp_ctx, ctx, module, builder);
  }

  if (is_ident(f, "fft")) {
    return dsp_fft_region(ast, dsp_ctx, ctx, module, builder);
  }

  // if (is_ident(f, "fft")) {
  //
  //   return dsp_fft_region(ast, dsp_ctx, ctx, module, builder);
  // }

  if (callable_sym) {
    LLVMValueRef callable = callable_sym->val;

    if (callable_sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
      callable = instantiate_extern_fn_sym(callable_sym, ctx, module, builder);
    }

    Type *expected_fn_type = ast->data.AST_APPLICATION.function->type;
    Type *callable_type = expected_fn_type;

    Ast *fn_ast = ast->data.AST_APPLICATION.function;
    Ast *collection_proc_fn_ast;

    if ((collection_proc_fn_ast = get_collection_proc_func(fn_ast))) {
      return call_dsp_list_proc(collection_proc_fn_ast, ast, dsp_ctx, ctx,
                                module, builder);
    }

    // if (callable_sym->type == STYPE_GENERIC_FUNCTION &&
    //     !is_closure(callable_sym->symbol_type)) {
    //   callable_type = resolve_sym_type(expected_fn_type,
    //                                    callable_sym->symbol_type,
    //                                    ctx->env);
    //   callable = get_specific_callable(callable_sym, callable_type, ctx,
    //   module,
    //                                    builder);
    // }
    //
    if (!callable) {
      fprintf(stderr, "dsp fn application failed, callable not found\n");
      print_ast_err(ast);
      return DSP_NULL;
    }

    Type *f = callable_sym->symbol_type;

    if (is_void_func(f)) {
      return DSP_SCALAR(LLVMBuildCall2(builder,
                                       LLVMGlobalGetValueType(callable),
                                       callable, NULL, 0, "call.ylc-function"));
    }

    int args_len = ast->data.AST_APPLICATION.len;
    LLVMValueRef args[args_len];
    for (int i = 0; i < args_len; i++) {
      args[i] = dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx, ctx,
                               module, builder)
                    .scalar;
      if (args[i] == NULL) {
        fprintf(stderr, "Application Error: null operand to function %d\n",
                fn_ast->tag);
        print_ast_err(ast->data.AST_APPLICATION.args + i);
        print_ast_err(fn_ast);
        return DSP_NULL;
      }
      Type *t = f->data.T_FN.from;

      if (types_equal(t, &t_num)) {
        args[i] = ensure_float((ast->data.AST_APPLICATION.args + i)->type,
                               args[i], builder);
      }
      f = f->data.T_FN.to;
    }

    return DSP_SCALAR(LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable),
                                     callable, args, args_len,
                                     "call.ylc-function"));
  }

  return DSP_NULL;
}
