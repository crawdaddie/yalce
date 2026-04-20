#include "./dsp_spectral.h"
#include "./dsp_fn_application.h"
#include "../../engine/ctx.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./common.h"
#include "function.h"
#include <fftw3.h>
#include <llvm-c/Target.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int32_t fft_size;
  int32_t hop_size;
  int32_t num_inputs;
  int32_t write_pos;
  int32_t hop_pos;
  int32_t ola_read_pos;
  double *window;
  double *ola;
  double *in_ring;
  double *frame_tmp;
  fftw_complex *spec;
  fftw_complex *out_spec;
  fftw_complex *fft_in;
  fftw_complex *ifft_out;
  fftw_plan forward_plan;
  fftw_plan inverse_plan;
} SpectralRegionState;

typedef struct {
  double re;
  double im;
} Complex;

Complex polar(double mag, double theta) {
  return (Complex){mag * cos(theta), mag * sin(theta)};
}

typedef struct {
  int32_t size;
  Complex *data;
} ComplexArray;

_Static_assert(sizeof(Complex) == sizeof(fftw_complex),
               "SpectralComplex must match fftw_complex layout");
_Static_assert(_Alignof(Complex) == _Alignof(fftw_complex),
               "SpectralComplex must match fftw_complex alignment");

// A compiled spectral kernel receives real YLC-style arrays of complex tuples:
// `{ i32 size, ptr data }` where each element is `(Double, Double)`.
//
// The underlying storage still remains the existing flat/interleaved FFTW
// buffers. We just build array views over them before invoking the kernel:
// - `inputs[i]` is the analyzed spectrum for input `i`
// - `out` is the writable output spectrum for this frame
//
// This runs once per hop after all input FFTs are ready and before the inverse
// FFT. If `kernel` is NULL, the runtime falls back to copying input 0 through
// unchanged, which corresponds to `(fn mod car -> mod)` for the current
// two-input experiments.
typedef void (*SpectralKernelFn)(void *state_raw, void *node_raw,
                                 const ComplexArray *inputs, ComplexArray out);

static int spectral_kernel_counter = 0;

typedef struct {
  LLVMValueRef init_fn;
  LLVMValueRef frame_fn;
  int state_bytes;
} SpectralKernelRecord;

static inline double spectral_complex_mag(Complex z) {
  return sqrt(z.re * z.re + z.im * z.im);
}

static inline Complex spectral_complex_conjugate(Complex z) {
  return (Complex){.re = z.re, .im = -z.im};
}

static inline Complex spectral_complex_scale(Complex z, double s) {
  return (Complex){.re = z.re * s, .im = z.im * s};
}

static inline Complex spectral_complex_normalize_phase(Complex z) {
  double m = spectral_complex_mag(z);
  if (m < 1.0e-12) {
    return (Complex){.re = 1.0, .im = 0.0};
  }
  return spectral_complex_scale(z, 1.0 / m);
}

static LLVMValueRef spectral_bin_mag_ir(LLVMBuilderRef builder,
                                        LLVMValueRef bin,
                                        LLVMModuleRef module) {
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMValueRef re = LLVMBuildExtractValue(builder, bin, 0, "spectral.bin.re");
  LLVMValueRef im = LLVMBuildExtractValue(builder, bin, 1, "spectral.bin.im");
  LLVMValueRef re2 = LLVMBuildFMul(builder, re, re, "spectral.bin.re2");
  LLVMValueRef im2 = LLVMBuildFMul(builder, im, im, "spectral.bin.im2");
  LLVMValueRef sum = LLVMBuildFAdd(builder, re2, im2, "spectral.bin.mag2");
  LLVMTypeRef sqrt_fn_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){f64_ty}, 1, 0);
  LLVMValueRef sqrt_fn = LLVMGetNamedFunction(module, "llvm.sqrt.f64");
  if (!sqrt_fn) {
    sqrt_fn = LLVMAddFunction(module, "llvm.sqrt.f64", sqrt_fn_ty);
  }
  return LLVMBuildCall2(builder, sqrt_fn_ty, sqrt_fn, (LLVMValueRef[]){sum}, 1,
                        "spectral.bin.mag");
}

static void spectral_smooth_envelope(const ComplexArray in, double *env_out,
                                     int32_t n, int32_t nyq, int radius) {
  if (!env_out || !in.data || n <= 0) {
    return;
  }

  for (int32_t i = 0; i <= nyq; i++) {
    int32_t lo = i - radius;
    int32_t hi = i + radius;
    if (lo < 0) {
      lo = 0;
    }
    if (hi > nyq) {
      hi = nyq;
    }

    double acc = 0.0;
    int32_t count = 0;
    for (int32_t j = lo; j <= hi; j++) {
      acc += spectral_complex_mag(in.data[j]);
      count++;
    }
    env_out[i] = count > 0 ? acc / (double)count : 0.0;
  }
}

// Reference spectral vocoder helper with the YLC-callable ABI:
//   FreqSignal -> FreqSignal -> FreqSignal -> FreqSignal
//
// `out` is treated as a writable destination and also returned so it fits the
// current spectral-function lowering path.
ComplexArray spectral_vocoder_mod_carrier(ComplexArray mod, ComplexArray car,
                                          ComplexArray out) {
  int32_t n = out.size;
  int32_t nyq = n / 2;
  const double eps = 1.0e-9;
  const int env_radius = 8;

  if (!mod.data || !car.data || mod.size < n || car.size < n) {
    return out;
  }

  // CCRMA-style cross-synthesis:
  // 1. Estimate smooth spectral envelopes for modulator and carrier
  // 2. Flatten the carrier by dividing by its own envelope
  // 3. Impose the modulator envelope on the flattened carrier
  double mod_env[nyq + 1];
  double car_env[nyq + 1];
  spectral_smooth_envelope(mod, mod_env, n, nyq, env_radius);
  spectral_smooth_envelope(car, car_env, n, nyq, env_radius);

  // DC bin must stay purely real for real-valued IFFT output.
  {
    double gain = mod_env[0] / fmax(car_env[0], eps);
    double re = car.data[0].re * gain;
    out.data[0] = (Complex){.re = re, .im = 0.0};
  }

  for (int32_t i = 1; i < nyq; i++) {
    double gain = mod_env[i] / fmax(car_env[i], eps);
    Complex z = spectral_complex_scale(car.data[i], gain);

    out.data[i] = z;
    out.data[n - i] = spectral_complex_conjugate(z);
  }

  // Nyquist bin must also be purely real for even-length real FFTs.
  {
    double gain = mod_env[nyq] / fmax(car_env[nyq], eps);
    double re = car.data[nyq].re * gain;
    out.data[nyq] = (Complex){.re = re, .im = 0.0};
  }
  return out;
}

ComplexArray spectral_env_fill(ComplexArray in, ComplexArray out) {
  int32_t n = out.size;
  int32_t radius = 8;
  if (!in.data || !out.data || in.size < n || n <= 0) {
    return out;
  }

  for (int32_t i = 0; i < n; i++) {
    int32_t lo = i - radius;
    int32_t hi = i + radius;
    if (lo < 0) {
      lo = 0;
    }
    if (hi >= n) {
      hi = n - 1;
    }

    double acc = 0.0;
    int32_t count = 0;
    for (int32_t j = lo; j <= hi; j++) {
      acc += spectral_complex_mag(in.data[j]);
      count++;
    }

    double avg = count > 0 ? acc / (double)count : 0.0;
    out.data[i].re = avg;
    out.data[i].im = 0.0;
  }

  return out;
}

ComplexArray spectral_scale_mag(ComplexArray in, ComplexArray env,
                                ComplexArray out) {
  int32_t n = out.size;
  if (!in.data || !env.data || !out.data || in.size < n || env.size < n ||
      n <= 0) {
    return out;
  }

  for (int32_t i = 0; i < n; i++) {
    double gain = env.data[i].re;
    out.data[i].re = in.data[i].re * gain;
    out.data[i].im = in.data[i].im * gain;
  }
  return out;
}

ComplexArray spectral_div_mag(ComplexArray in, ComplexArray env,
                              ComplexArray out) {
  int32_t n = out.size;
  const double eps = 1.0e-9;
  if (!in.data || !env.data || !out.data || in.size < n || env.size < n ||
      n <= 0) {
    return out;
  }

  for (int32_t i = 0; i < n; i++) {
    double denom = fmax(env.data[i].re, eps);
    double gain = 1.0 / denom;
    out.data[i].re = in.data[i].re * gain;
    out.data[i].im = in.data[i].im * gain;
  }
  return out;
}

_DoubleArray pv_env_real_fill(ComplexArray in, _DoubleArray out) {
  int32_t in_n = in.size;
  int32_t out_n = out.size;
  if (!in.data || !out.data || in_n <= 0 || out_n <= 0 || out_n > in_n) {
    return out;
  }

  for (int32_t i = 0; i < out_n; i++) {
    int64_t lo64 = ((int64_t)i * (int64_t)in_n) / (int64_t)out_n;
    int64_t hi64 = ((int64_t)(i + 1) * (int64_t)in_n) / (int64_t)out_n;
    int32_t lo = (int32_t)lo64;
    int32_t hi = (int32_t)hi64 - 1;
    if (hi < lo) {
      hi = lo;
    }
    if (hi >= in_n) {
      hi = in_n - 1;
    }

    double acc = 0.0;
    int32_t count = 0;
    for (int32_t j = lo; j <= hi; j++) {
      acc += spectral_complex_mag(in.data[j]);
      count++;
    }
    out.data[i] = count > 0 ? acc / (double)count : 0.0;
  }
  return out;
}

static int spectral_align_up_i32(int v, int align) {
  return (v + (align - 1)) & ~(align - 1);
}

static int spectral_region_state_bytes(int fft_size, int num_inputs) {
  int bytes = spectral_align_up_i32((int)sizeof(SpectralRegionState), 16);
  bytes += spectral_align_up_i32((int)(sizeof(double) * (size_t)fft_size), 16);
  bytes += spectral_align_up_i32((int)(sizeof(double) * (size_t)fft_size), 16);
  bytes += spectral_align_up_i32(
      (int)(sizeof(double) * (size_t)fft_size * (size_t)num_inputs), 16);
  bytes += spectral_align_up_i32(
      (int)(sizeof(double) * (size_t)fft_size * (size_t)num_inputs), 16);
  bytes += spectral_align_up_i32(
      (int)(sizeof(fftw_complex) * (size_t)fft_size * (size_t)num_inputs), 16);
  bytes +=
      spectral_align_up_i32((int)(sizeof(fftw_complex) * (size_t)fft_size), 16);
  bytes +=
      spectral_align_up_i32((int)(sizeof(fftw_complex) * (size_t)fft_size), 16);
  bytes +=
      spectral_align_up_i32((int)(sizeof(fftw_complex) * (size_t)fft_size), 16);
  return bytes;
}

static LLVMValueRef spectral_ensure_float(Type *in_type, LLVMValueRef val,
                                          LLVMBuilderRef builder) {
  if (!val) {
    return val;
  }
  if (LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMIntegerTypeKind) {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "spectral.i2f");
  }
  if (types_equal(in_type, &t_int)) {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "spectral.freq.f64");
  }
  return val;
}

TypeEnv *create_env_for_spectral_fn(TypeEnv *env, Type *generic_type,
                                    Type *specific_type) {

  Subst *subst = NULL;

  TICtx unify_ctx = {};
  while (generic_type->kind == T_FN) {
    Type *gen = generic_type->data.T_FN.from;
    Type *spec = specific_type->data.T_FN.from;
    unify(gen, spec, &unify_ctx);
    specific_type = specific_type->data.T_FN.to;
    generic_type = generic_type->data.T_FN.to;
  }

  subst = solve_constraints(unify_ctx.constraints);
  env = create_env_from_subst(env, subst);

  return env;
}

void spectral_region_init(void *state_raw, int32_t fft_size, int32_t hop_size,
                          int32_t num_inputs) {
  if (!state_raw || fft_size <= 0 || hop_size <= 0 || num_inputs <= 0) {
    return;
  }

  SpectralRegionState *st = (SpectralRegionState *)state_raw;
  memset(st, 0, sizeof(*st));

  st->fft_size = fft_size;
  st->hop_size = hop_size;
  st->num_inputs = num_inputs;
  st->write_pos = 0;
  st->hop_pos = 0;
  st->ola_read_pos = 0;

  char *cursor =
      (char *)state_raw + spectral_align_up_i32((int)sizeof(*st), 16);

  st->window = (double *)cursor;
  cursor += spectral_align_up_i32((int)(sizeof(double) * (size_t)fft_size), 16);

  st->ola = (double *)cursor;
  cursor += spectral_align_up_i32((int)(sizeof(double) * (size_t)fft_size), 16);

  st->in_ring = (double *)cursor;
  cursor += spectral_align_up_i32(
      (int)(sizeof(double) * (size_t)fft_size * (size_t)num_inputs), 16);

  st->frame_tmp = (double *)cursor;
  cursor += spectral_align_up_i32(
      (int)(sizeof(double) * (size_t)fft_size * (size_t)num_inputs), 16);

  st->spec = (fftw_complex *)cursor;
  cursor += spectral_align_up_i32(
      (int)(sizeof(fftw_complex) * (size_t)fft_size * (size_t)num_inputs), 16);

  st->out_spec = (fftw_complex *)cursor;
  cursor +=
      spectral_align_up_i32((int)(sizeof(fftw_complex) * (size_t)fft_size), 16);

  st->fft_in = (fftw_complex *)cursor;
  cursor +=
      spectral_align_up_i32((int)(sizeof(fftw_complex) * (size_t)fft_size), 16);

  st->ifft_out = (fftw_complex *)cursor;

  memset(st->window, 0, sizeof(double) * (size_t)fft_size);
  memset(st->ola, 0, sizeof(double) * (size_t)fft_size);
  memset(st->in_ring, 0,
         sizeof(double) * (size_t)fft_size * (size_t)num_inputs);
  memset(st->frame_tmp, 0,
         sizeof(double) * (size_t)fft_size * (size_t)num_inputs);
  memset(st->spec, 0,
         sizeof(fftw_complex) * (size_t)fft_size * (size_t)num_inputs);
  memset(st->out_spec, 0, sizeof(fftw_complex) * (size_t)fft_size);
  memset(st->fft_in, 0, sizeof(fftw_complex) * (size_t)fft_size);
  memset(st->ifft_out, 0, sizeof(fftw_complex) * (size_t)fft_size);

  st->forward_plan = fftw_plan_dft_1d(fft_size, st->fft_in, st->fft_in,
                                      FFTW_FORWARD, FFTW_MEASURE);
  st->inverse_plan = fftw_plan_dft_1d(fft_size, st->out_spec, st->ifft_out,
                                      FFTW_BACKWARD, FFTW_MEASURE);
  if (!st->forward_plan || !st->inverse_plan) {
    if (st->forward_plan) {
      fftw_destroy_plan(st->forward_plan);
    }
    if (st->inverse_plan) {
      fftw_destroy_plan(st->inverse_plan);
    }
    return;
  }

  double *window = st->window;
  if (fft_size == 1) {
    window[0] = 1.0;
    return;
  }

  for (int i = 0; i < fft_size; i++) {
    window[i] =
        0.5 - 0.5 * cos((2.0 * M_PI * (double)i) / (double)(fft_size - 1));
  }
}

double spectral_region_samp(void *state_raw, double *inputs, void *kernel_raw,
                            void *kernel_state_raw) {
  if (!state_raw || !inputs) {
    return 0.0;
  }

  SpectralRegionState *st = (SpectralRegionState *)state_raw;
  SpectralKernelFn kernel = (SpectralKernelFn)kernel_raw;
  if (st->fft_size <= 0 || st->num_inputs <= 0) {
    return 0.0;
  }

  double *window = st->window;
  double *ola = st->ola;
  double *in_ring = st->in_ring;
  double *frame_tmp = st->frame_tmp;
  fftw_complex *spec = st->spec;

  int write_pos = st->write_pos;
  for (int i = 0; i < st->num_inputs; i++) {
    in_ring[(i * st->fft_size) + write_pos] = inputs[i];
  }

  double out = ola[st->ola_read_pos];
  ola[st->ola_read_pos] = 0.0;

  st->write_pos = (write_pos + 1) % st->fft_size;
  st->hop_pos++;
  st->ola_read_pos = (st->ola_read_pos + 1) % st->fft_size;

  if (st->hop_pos >= st->hop_size) {
    const double synth_gain = 2.0 / 3.0;
    st->hop_pos = 0;

    for (int ch = 0; ch < st->num_inputs; ch++) {
      double *ring = in_ring + ((size_t)ch * (size_t)st->fft_size);
      double *frame = frame_tmp + ((size_t)ch * (size_t)st->fft_size);
      fftw_complex *chan_spec = spec + ((size_t)ch * (size_t)st->fft_size);

      for (int n = 0; n < st->fft_size; n++) {
        int src = (st->write_pos + n) % st->fft_size;
        double sample = ring[src] * window[n];
        frame[n] = sample;
        st->fft_in[n][0] = sample;
        st->fft_in[n][1] = 0.0;
      }

      fftw_execute(st->forward_plan);

      for (int k = 0; k < st->fft_size; k++) {
        chan_spec[k][0] = st->fft_in[k][0];
        chan_spec[k][1] = st->fft_in[k][1];
      }
    }

    if (kernel) {
      ComplexArray input_views[st->num_inputs];
      ComplexArray out_view = {
          .size = st->fft_size,
          .data = (Complex *)st->out_spec,
      };

      for (int ch = 0; ch < st->num_inputs; ch++) {
        input_views[ch] = (ComplexArray){
            .size = st->fft_size,
            .data = (Complex *)(spec + ((size_t)ch * st->fft_size)),
        };
      }

      // The compiled spectral lambda runs here. It sees one YLC array per
      // analyzed input spectrum and writes its result directly into the
      // preallocated output spectrum view.
      kernel(kernel_state_raw, NULL, input_views, out_view);
    } else {
      // Default passthrough while spectral-lambda compilation is still being
      // wired up: resynthesise input 0 (`mod`) unchanged.
      for (int k = 0; k < st->fft_size; k++) {
        st->out_spec[k][0] = spec[k][0];
        st->out_spec[k][1] = spec[k][1];
      }
    }

    fftw_execute(st->inverse_plan);

    for (int n = 0; n < st->fft_size; n++) {
      int dst = (st->ola_read_pos + n) % st->fft_size;
      double sample =
          (st->ifft_out[n][0] / (double)st->fft_size) * window[n] * synth_gain;
      ola[dst] += sample;
    }
  }

  return out;
}

LLVMValueRef spectral_expr() {}

static void spectral_build_array_copy(LLVMBuilderRef builder,
                                      LLVMModuleRef module,
                                      LLVMValueRef src_arr,
                                      LLVMValueRef dst_arr, LLVMTypeRef elem_ty,
                                      const char *prefix) {
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef arr_ty = codegen_array_type(elem_ty);
  LLVMValueRef src_size =
      LLVMBuildExtractValue(builder, src_arr, 0, "spectral.ret.size");
  LLVMValueRef src_data =
      LLVMBuildExtractValue(builder, src_arr, 1, "spectral.ret.data");
  LLVMValueRef dst_size =
      LLVMBuildExtractValue(builder, dst_arr, 0, "spectral.out.size");
  LLVMValueRef dst_data =
      LLVMBuildExtractValue(builder, dst_arr, 1, "spectral.out.data");
  LLVMValueRef smin_fn = LLVMGetNamedFunction(module, "llvm.smin.i32");
  if (!smin_fn) {
    smin_fn = LLVMAddFunction(
        module, "llvm.smin.i32",
        LLVMFunctionType(i32_ty, (LLVMTypeRef[]){i32_ty, i32_ty}, 2, 0));
  }
  LLVMValueRef copy_count = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(smin_fn), smin_fn,
      (LLVMValueRef[]){src_size, dst_size}, 2, "spectral.copy.count");

  LLVMValueRef idx_ptr = LLVMBuildAlloca(builder, i32_ty, "spectral.copy.i");
  LLVMBuildStore(builder, LLVMConstInt(i32_ty, 0, 0), idx_ptr);

  char loop_name[64];
  char body_name[64];
  char exit_name[64];
  snprintf(loop_name, sizeof(loop_name), "%s.copy.loop", prefix);
  snprintf(body_name, sizeof(body_name), "%s.copy.body", prefix);
  snprintf(exit_name, sizeof(exit_name), "%s.copy.exit", prefix);

  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(fn, loop_name);
  LLVMBasicBlockRef body_bb = LLVMAppendBasicBlock(fn, body_name);
  LLVMBasicBlockRef exit_bb = LLVMAppendBasicBlock(fn, exit_name);
  LLVMBuildBr(builder, loop_bb);

  LLVMPositionBuilderAtEnd(builder, loop_bb);
  LLVMValueRef idx_val =
      LLVMBuildLoad2(builder, i32_ty, idx_ptr, "spectral.copy.idx");
  LLVMValueRef keep_going = LLVMBuildICmp(builder, LLVMIntSLT, idx_val,
                                          copy_count, "spectral.copy.lt");
  LLVMBuildCondBr(builder, keep_going, body_bb, exit_bb);

  LLVMPositionBuilderAtEnd(builder, body_bb);
  LLVMValueRef src_ptr = LLVMBuildGEP2(builder, elem_ty, src_data, &idx_val, 1,
                                       "spectral.copy.src");
  LLVMValueRef dst_ptr = LLVMBuildGEP2(builder, elem_ty, dst_data, &idx_val, 1,
                                       "spectral.copy.dst");
  LLVMValueRef elt =
      LLVMBuildLoad2(builder, elem_ty, src_ptr, "spectral.copy.elt");
  LLVMBuildStore(builder, elt, dst_ptr);
  LLVMValueRef next = LLVMBuildAdd(builder, idx_val, LLVMConstInt(i32_ty, 1, 0),
                                   "spectral.copy.next");
  LLVMBuildStore(builder, next, idx_ptr);
  LLVMBuildBr(builder, loop_bb);

  LLVMPositionBuilderAtEnd(builder, exit_bb);
  (void)arr_ty;
}

static SpectralKernelRecord
compile_spectral_frame_record(Ast *fn_ast, Type *fn_type, TypeEnv *env,
                              DspBuildCtx *enclosing_dsp_ctx, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {
  SpectralKernelRecord rec = {0};
  if (!fn_ast || !fn_type || fn_ast->tag != AST_LAMBDA) {
    return rec;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);
  LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef complex_ty =
      LLVMStructType((LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMTypeRef array_ty = codegen_array_type(complex_ty);

  char *name = malloc(64);
  if (!name) {
    return rec;
  }

  char frame_name[64];
  char init_name[64];
  int kernel_id = spectral_kernel_counter++;
  snprintf(name, 64, "spectral.kernel.%d", kernel_id);
  snprintf(frame_name, sizeof(frame_name), "spectral.kernel.frame.%d",
           kernel_id);
  snprintf(init_name, sizeof(init_name), "spectral.kernel.init.%d", kernel_id);

  int num_params = fn_ast->data.AST_LAMBDA.len;
  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)(num_params + 3));
  if (!frame_param_tys) {
    free(name);
    return rec;
  }
  frame_param_tys[0] = i8_ptr_ty;
  frame_param_tys[1] = i8_ptr_ty;
  frame_param_tys[2] = LLVMPointerType(array_ty, 0);

  Type *walk_type = fn_type;
  for (int i = 0; i < num_params; i++) {
    Type *param_type =
        walk_type && walk_type->kind == T_FN ? walk_type->data.T_FN.from : NULL;
    frame_param_tys[i + 3] = type_to_llvm_type(param_type, ctx, module);
    walk_type =
        walk_type && walk_type->kind == T_FN ? walk_type->data.T_FN.to : NULL;
  }
  LLVMTypeRef frame_ty =
      LLVMFunctionType(LLVMVoidType(), frame_param_tys, num_params + 3, 0);
  free(frame_param_tys);

  LLVMValueRef frame_fn = LLVMAddFunction(module, frame_name, frame_ty);
  LLVMTypeRef init_ty =
      LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){i8_ptr_ty}, 1, 0);
  LLVMValueRef init_fn = LLVMAddFunction(module, init_name, init_ty);

  LLVMBasicBlockRef frame_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, frame_fn, "entry");
  LLVMBasicBlockRef init_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, init_fn, "entry");
  LLVMBuilderRef frame_builder = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBuilderRef init_builder = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(frame_builder, frame_bb);
  LLVMPositionBuilderAtEnd(init_builder, init_bb);

  int compile_sample_rate = ctx_sample_rate();
  if (compile_sample_rate <= 0) {
    compile_sample_rate = 48000;
  }
  double compile_spf = ctx_spf();
  if (compile_spf <= 0.0) {
    compile_spf = 1.0 / (double)compile_sample_rate;
  }

  DspBuildCtx frame_ctx = {
      .init_builder = init_builder,
      .perform_builder = frame_builder,
      .perf_fn = frame_fn,
      .sample_rate = compile_sample_rate,
      .spf_scalar = compile_spf,
      .spectral_fft_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_fft_size : 0,
      .spectral_hop_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_hop_size : 0,
      .tmp_alloc = enclosing_dsp_ctx ? enclosing_dsp_ctx->tmp_alloc : NULL,
  };

  frame_ctx.init_state_ptr = LLVMGetParam(init_fn, 0);
  frame_ctx.init_state_base_ptr = frame_ctx.init_state_ptr;
  frame_ctx.init_state_cursor_ptr =
      LLVMBuildAlloca(init_builder, i8_ptr_ty, "init.state_cursor");
  LLVMBuildStore(init_builder, frame_ctx.init_state_ptr,
                 frame_ctx.init_state_cursor_ptr);

  frame_ctx.state_ptr = LLVMGetParam(frame_fn, 0);
  frame_ctx.state_base_ptr = frame_ctx.state_ptr;
  frame_ctx.node_ptr = LLVMGetParam(frame_fn, 1);
  LLVMValueRef frame_out_ptr = LLVMGetParam(frame_fn, 2);
  LLVMValueRef frame_out_arg =
      LLVMBuildLoad2(frame_builder, array_ty, frame_out_ptr, "spectral.out");
  frame_ctx.state_cursor_ptr =
      LLVMBuildAlloca(frame_builder, i8_ptr_ty, "frame.state_cursor");
  LLVMBuildStore(frame_builder, frame_ctx.state_ptr,
                 frame_ctx.state_cursor_ptr);

  LLVMTypeRef spf_fn_ty = LLVMFunctionType(f64_ty, (LLVMTypeRef[]){}, 0, 0);
  LLVMValueRef spf_fn = LLVMGetNamedFunction(module, "ctx_spf");
  if (!spf_fn) {
    spf_fn = LLVMAddFunction(module, "ctx_spf", spf_fn_ty);
    LLVMSetLinkage(spf_fn, LLVMExternalLinkage);
  }
  frame_ctx.spf =
      LLVMBuildCall2(frame_builder, spf_fn_ty, spf_fn, NULL, 0, "ctx_spf");

  JITLangCtx compilation_ctx_storage = *ctx;
  compilation_ctx_storage.env = env;
  JITLangCtx *compilation_ctx = &compilation_ctx_storage;
  STACK_ALLOC_CTX_PUSH(fn_ctx, compilation_ctx)

  int idx = 0;
  Type *bind_type = fn_type;
  for (AstList *p = fn_ast->data.AST_LAMBDA.params; p; p = p->next, idx++) {
    Ast *param_ast = p->ast;
    Type *param_type =
        bind_type && bind_type->kind == T_FN ? bind_type->data.T_FN.from : NULL;
    LLVMValueRef arg_val = LLVMGetParam(frame_fn, idx + 3);

    if (param_ast->tag == AST_TUPLE) {
      int nfields = param_ast->data.AST_LIST.len;
      for (int j = 0; j < nfields; j++) {
        Ast *field_ast = param_ast->data.AST_LIST.items + j;
        Type *field_type = param_type->data.T_CONS.args[j];
        LLVMValueRef field_val = LLVMBuildExtractValue(
            frame_builder, arg_val, (unsigned)j, "tuple.field");
        LLVMTypeRef field_llvm_ty = LLVMTypeOf(field_val);
        JITSymbol *field_sym =
            new_symbol(STYPE_LOCAL_VAR, field_type, field_val, field_llvm_ty);
        const char *field_chars = field_ast->data.AST_IDENTIFIER.value;
        int field_len = field_ast->data.AST_IDENTIFIER.length;
        ht_set_hash(fn_ctx.frame->table, field_chars,
                    hash_string(field_chars, field_len), field_sym);
      }
    } else {
      JITSymbol *sym =
          new_symbol(STYPE_LOCAL_VAR, param_type, arg_val, LLVMTypeOf(arg_val));
      const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
      int id_len = param_ast->data.AST_IDENTIFIER.length;
      ht_set_hash(fn_ctx.frame->table, id_chars, hash_string(id_chars, id_len),
                  sym);
    }
    bind_type =
        bind_type && bind_type->kind == T_FN ? bind_type->data.T_FN.to : NULL;
  }

  DspValue expr = dsp_build_expr(fn_ast->data.AST_LAMBDA.body, &frame_ctx,
                                 &fn_ctx, module, frame_builder);
  Type *ret_type = fn_return_type(fn_type);
  if (ret_type && is_array_type(ret_type)) {
    spectral_build_array_copy(frame_builder, module, expr.scalar, frame_out_arg,
                              complex_ty, "spectral.frame");
  }

  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(frame_builder))) {
    LLVMBuildRetVoid(frame_builder);
  }
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(init_builder))) {
    LLVMBuildRetVoid(init_builder);
  }

  rec.init_fn = init_fn;
  rec.frame_fn = frame_fn;
  rec.state_bytes = (frame_ctx.state_offset + 7) & ~7;

  LLVMDisposeBuilder(frame_builder);
  LLVMDisposeBuilder(init_builder);
  destroy_ctx(&fn_ctx);
  free(name);
  return rec;
}

static LLVMValueRef compile_spectral_kernel_wrapper(Ast *fn_ast,
                                                    SpectralKernelRecord rec,
                                                    int num_ins,
                                                    LLVMModuleRef module,
                                                    LLVMBuilderRef builder) {
  if (!rec.frame_fn) {
    return NULL;
  }

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef complex_ty =
      LLVMStructType((LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMTypeRef array_ty = codegen_array_type(complex_ty);
  LLVMTypeRef array_ptr_ty = LLVMPointerType(array_ty, 0);
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
  LLVMTypeRef wrapper_ty = LLVMFunctionType(
      LLVMVoidType(),
      (LLVMTypeRef[]){i8_ptr_ty, i8_ptr_ty, array_ptr_ty, array_ty}, 4, 0);

  char *name = malloc(64);
  if (!name) {
    return NULL;
  }
  snprintf(name, 64, "spectral.kernel.wrapper.%d", spectral_kernel_counter++);

  START_FUNC(module, name, wrapper_ty)

  LLVMValueRef wrapper_state_ptr = LLVMGetParam(func, 0);
  LLVMValueRef wrapper_node_ptr = LLVMGetParam(func, 1);
  LLVMValueRef inputs_ptr = LLVMGetParam(func, 2);
  LLVMValueRef out_arg = LLVMGetParam(func, 3);
  LLVMTypeRef frame_fn_ty = LLVMGlobalGetValueType(rec.frame_fn);
  unsigned user_argc = LLVMCountParamTypes(frame_fn_ty) - 3;
  LLVMValueRef *frame_args = alloca(sizeof(LLVMValueRef) * (user_argc + 3));
  LLVMValueRef state_cursor_ptr =
      LLVMBuildAlloca(builder, i8_ptr_ty, "spectral.frame.state_cursor");
  LLVMBuildStore(builder, wrapper_state_ptr, state_cursor_ptr);
  (void)state_cursor_ptr;

  LLVMValueRef out_ptr = LLVMBuildAlloca(builder, array_ty, "spectral.out.ptr");
  LLVMBuildStore(builder, out_arg, out_ptr);

  frame_args[0] = wrapper_state_ptr;
  frame_args[1] = wrapper_node_ptr;
  frame_args[2] = out_ptr;
  for (unsigned i = 0; i < user_argc; i++) {
    LLVMValueRef idx = LLVMConstInt(i32_ty, i, 0);
    LLVMValueRef in_ptr = LLVMBuildGEP2(builder, array_ty, inputs_ptr, &idx, 1,
                                        "spectral.in.gep");
    frame_args[i + 3] =
        LLVMBuildLoad2(builder, array_ty, in_ptr, "spectral.in");
  }

  LLVMBuildCall2(builder, frame_fn_ty, rec.frame_fn, frame_args, user_argc + 3,
                 "spectral.frame.call");
  LLVMBuildRetVoid(builder);
  END_FUNC
  return func;
}

DspValue dsp_fft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *fft_size_ast = ast->data.AST_APPLICATION.args + 0;
  Ast *hop_size_ast = ast->data.AST_APPLICATION.args + 1;

  Ast *fn_ast = ast->data.AST_APPLICATION.args + 2;

  Type *generic_type = fn_ast->type;
  Type *ret_type = fn_return_type(fn_ast->type);
  int num_args = fn_ast->data.AST_LAMBDA.len;
  int n = num_args;
  Type *spec_type = ret_type;

  while (n--) {
    spec_type = type_fn(ret_type, spec_type);
  }

  TypeEnv *spectral_fn_compilation_env =
      create_env_for_spectral_fn(ctx->env, generic_type, spec_type);
  Type *fn_type =
      resolve_type_in_env(generic_type, spectral_fn_compilation_env);

  Ast *inputs_ast = ast->data.AST_APPLICATION.args + 3;

  if (fft_size_ast->tag != AST_INT || hop_size_ast->tag != AST_INT) {
    return DSP_NULL;
  }

  if (!(inputs_ast->tag == AST_ARRAY || inputs_ast->tag == AST_LIST)) {
    fprintf(stderr, "fft inputs must be a literal array/list\n");
    return DSP_NULL;
  }

  int fft_size = fft_size_ast->data.AST_INT.value;
  int hop_size = hop_size_ast->data.AST_INT.value;
  int num_ins = inputs_ast->data.AST_LIST.len;
  if (fft_size <= 0 || hop_size <= 0 || num_ins <= 0) {
    return DSP_ZERO;
  }

  DspValue ins[num_ins];
  int max_lanes = 1;
  for (int i = 0; i < num_ins; i++) {
    ins[i] = dsp_build_expr(inputs_ast->data.AST_LIST.items + i, dsp_ctx, ctx,
                            module, builder);
    if (ins[i].lanes > max_lanes) {
      max_lanes = ins[i].lanes;
    }
  }

  int lane_state_bytes = spectral_region_state_bytes(fft_size, num_ins);
  int kernel_state_bytes = 0;
  int per_lane_state_bytes = lane_state_bytes + kernel_state_bytes;
  int total_state_bytes = per_lane_state_bytes * max_lanes;
  int off = spectral_align_up_i32(dsp_ctx->state_offset, 8);
  dsp_ctx->state_offset = off + total_state_bytes;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMValueRef kernel_ptr = LLVMConstNull(i8_ptr_ty);
  SpectralKernelRecord kernel_rec = {0};
  int prev_fft_size = dsp_ctx->spectral_fft_size;
  int prev_hop_size = dsp_ctx->spectral_hop_size;
  dsp_ctx->spectral_fft_size = fft_size;
  dsp_ctx->spectral_hop_size = hop_size;

  if (fn_ast->tag == AST_LAMBDA && fn_type) {
    kernel_rec = compile_spectral_frame_record(fn_ast, fn_type,
                                               spectral_fn_compilation_env,
                                               dsp_ctx, ctx, module, builder);
    kernel_state_bytes = (kernel_rec.state_bytes + 7) & ~7;
    per_lane_state_bytes = lane_state_bytes + kernel_state_bytes;
    total_state_bytes = per_lane_state_bytes * max_lanes;
    dsp_ctx->state_offset = off + total_state_bytes;

    LLVMValueRef wrapper_fn = compile_spectral_kernel_wrapper(
        fn_ast, kernel_rec, num_ins, module, builder);
    if (wrapper_fn) {
      kernel_ptr = LLVMBuildBitCast(builder, wrapper_fn, i8_ptr_ty,
                                    "spectral.kernel.ptr");
    }
  }
  dsp_ctx->spectral_fft_size = prev_fft_size;
  dsp_ctx->spectral_hop_size = prev_hop_size;

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_state_base =
        dsp_consume_init_state(dsp_ctx, dsp_ctx->init_builder,
                               total_state_bytes, 8, "spectral.init.state");
    LLVMTypeRef init_fn_ty = LLVMFunctionType(
        LLVMVoidType(), (LLVMTypeRef[]){i8_ptr_ty, i32_ty, i32_ty, i32_ty}, 4,
        0);
    LLVMValueRef init_fn = LLVMGetNamedFunction(module, "spectral_region_init");
    if (!init_fn) {
      init_fn = LLVMAddFunction(module, "spectral_region_init", init_fn_ty);
      LLVMSetLinkage(init_fn, LLVMExternalLinkage);
    }

    for (int lane = 0; lane < max_lanes; lane++) {
      LLVMValueRef lane_init_state = init_state_base;
      if (lane > 0) {
        LLVMValueRef lane_off =
            LLVMConstInt(i64_ty, (uint64_t)(lane * per_lane_state_bytes), 0);
        lane_init_state =
            LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, init_state_base,
                          &lane_off, 1, "spectral.init.state.lane");
      }
      LLVMBuildCall2(
          dsp_ctx->init_builder, init_fn_ty, init_fn,
          (LLVMValueRef[]){lane_init_state,
                           LLVMConstInt(i32_ty, (uint64_t)fft_size, 0),
                           LLVMConstInt(i32_ty, (uint64_t)hop_size, 0),
                           LLVMConstInt(i32_ty, (uint64_t)num_ins, 0)},
          4, "spectral.init");
      if (kernel_rec.init_fn && kernel_state_bytes > 0) {
        LLVMValueRef kernel_init_state =
            LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, lane_init_state,
                          (LLVMValueRef[]){LLVMConstInt(
                              i64_ty, (uint64_t)lane_state_bytes, 0)},
                          1, "spectral.kernel.init.state");
        LLVMBuildCall2(dsp_ctx->init_builder,
                       LLVMGlobalGetValueType(kernel_rec.init_fn),
                       kernel_rec.init_fn, (LLVMValueRef[]){kernel_init_state},
                       1, "spectral.kernel.init");
      }
    }
  }

  LLVMValueRef state_base = dsp_consume_frame_state(
      dsp_ctx, builder, total_state_bytes, 8, "spectral.state");

  LLVMTypeRef samp_fn_ty = LLVMFunctionType(
      f64_ty, (LLVMTypeRef[]){i8_ptr_ty, f64_ptr_ty, i8_ptr_ty, i8_ptr_ty}, 4,
      0);
  LLVMValueRef samp_fn = LLVMGetNamedFunction(module, "spectral_region_samp");
  if (!samp_fn) {
    samp_fn = LLVMAddFunction(module, "spectral_region_samp", samp_fn_ty);
    LLVMSetLinkage(samp_fn, LLVMExternalLinkage);
  }

  if (max_lanes == 1) {
    LLVMValueRef in_storage = LLVMBuildArrayAlloca(
        builder, f64_ty, LLVMConstInt(i32_ty, (uint64_t)num_ins, 0),
        "spectral.inputs");
    for (int i = 0; i < num_ins; i++) {
      LLVMValueRef idx = LLVMConstInt(i64_ty, (uint64_t)i, 0);
      LLVMValueRef ptr = LLVMBuildGEP2(builder, f64_ty, in_storage, &idx, 1,
                                       "spectral.in.ptr");
      LLVMBuildStore(
          builder,
          spectral_ensure_float(inputs_ast->data.AST_LIST.items[i].type,
                                ins[i].scalar, builder),
          ptr);
    }

    return DSP_SCALAR(LLVMBuildCall2(
        builder, samp_fn_ty, samp_fn,
        (LLVMValueRef[]){
            LLVMBuildBitCast(builder, state_base, i8_ptr_ty,
                             "spectral.state.i8"),
            LLVMBuildBitCast(builder, in_storage, f64_ptr_ty,
                             "spectral.inputs.ptr"),
            kernel_ptr,
            LLVMBuildBitCast(
                builder,
                LLVMBuildGEP2(builder, i8_ty, state_base,
                              (LLVMValueRef[]){LLVMConstInt(
                                  i64_ty, (uint64_t)lane_state_bytes, 0)},
                              1, "spectral.kernel.state"),
                i8_ptr_ty, "spectral.kernel.state.i8")},
        4, "spectral.sample"));
  }

  LLVMValueRef *vals =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)max_lanes, 9);
  if (!vals) {
    return DSP_NULL;
  }

  for (int lane = 0; lane < max_lanes; lane++) {
    LLVMValueRef lane_state = state_base;
    if (lane > 0) {
      LLVMValueRef lane_off =
          LLVMConstInt(i64_ty, (uint64_t)(lane * per_lane_state_bytes), 0);
      lane_state = LLVMBuildGEP2(builder, i8_ty, state_base, &lane_off, 1,
                                 "spectral.state.lane");
    }

    LLVMValueRef in_storage = LLVMBuildArrayAlloca(
        builder, f64_ty, LLVMConstInt(i32_ty, (uint64_t)num_ins, 0),
        "spectral.inputs");
    for (int i = 0; i < num_ins; i++) {
      LLVMValueRef idx = LLVMConstInt(i64_ty, (uint64_t)i, 0);
      LLVMValueRef ptr = LLVMBuildGEP2(builder, f64_ty, in_storage, &idx, 1,
                                       "spectral.in.ptr");
      LLVMValueRef lane_val =
          ins[i].lanes > 1 ? ins[i].vec[lane % ins[i].lanes] : ins[i].scalar;
      LLVMBuildStore(
          builder,
          spectral_ensure_float(inputs_ast->data.AST_LIST.items[i].type,
                                lane_val, builder),
          ptr);
    }

    vals[lane] = LLVMBuildCall2(
        builder, samp_fn_ty, samp_fn,
        (LLVMValueRef[]){
            LLVMBuildBitCast(builder, lane_state, i8_ptr_ty,
                             "spectral.state.i8"),
            LLVMBuildBitCast(builder, in_storage, f64_ptr_ty,
                             "spectral.inputs.ptr"),
            kernel_ptr,
            LLVMBuildBitCast(
                builder,
                LLVMBuildGEP2(builder, i8_ty, lane_state,
                              (LLVMValueRef[]){LLVMConstInt(
                                  i64_ty, (uint64_t)lane_state_bytes, 0)},
                              1, "spectral.kernel.state"),
                i8_ptr_ty, "spectral.kernel.state.i8")},
        4, "spectral.sample");
  }

  return DSP_MULTI(max_lanes, vals);
}

static LLVMValueRef spectral_alloc_hoisted_array_typed(
    Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder, const char *base_name, Type *el_type, int len) {
  if (!dsp_ctx) {
    return NULL;
  }

  if (len <= 0 || len > 2048) {
    fprintf(stderr, "%s requires size in 1..2048\n", base_name);
    print_ast_err(ast);
    return NULL;
  }
  LLVMTypeRef el_llvm_ty = type_to_llvm_type(el_type, ctx, module);
  LLVMTypeRef arr_ty = codegen_array_type(el_llvm_ty);
  LLVMTypeRef arr_ptr_ty = LLVMPointerType(arr_ty, 0);
  LLVMTypeRef el_ptr_ty = LLVMPointerType(el_llvm_ty, 0);
  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();

  LLVMTargetDataRef dl = LLVMGetModuleDataLayout(module);
  int el_size = (int)LLVMABISizeOfType(dl, el_llvm_ty);
  int arr_size = (int)LLVMABISizeOfType(dl, arr_ty);
  int total_bytes = arr_size + (len * el_size);

  dsp_ctx->array_attrs.comptime_size = len;
  dsp_ctx->state_offset = (dsp_ctx->state_offset + 7) & ~7;
  dsp_ctx->state_offset += total_bytes;

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, total_bytes, 8, base_name);
    LLVMValueRef arr_ptr = LLVMBuildBitCast(dsp_ctx->init_builder, init_base,
                                            arr_ptr_ty, "spectral.tmp.ptr");
    LLVMValueRef data_i8 = LLVMBuildGEP2(
        dsp_ctx->init_builder, i8_ty, init_base,
        (LLVMValueRef[]){LLVMConstInt(i64_ty, (uint64_t)arr_size, 0)}, 1,
        "spectral.tmp.data.i8");
    LLVMValueRef data_ptr = LLVMBuildBitCast(dsp_ctx->init_builder, data_i8,
                                             el_ptr_ty, "spectral.tmp.data");
    LLVMValueRef arr_init = LLVMGetUndef(arr_ty);
    arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                    LLVMConstInt(i32_ty, (uint64_t)len, 0), 0,
                                    "spectral.tmp.size");
    arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init, data_ptr,
                                    1, "spectral.tmp.data.ptr");
    LLVMBuildStore(dsp_ctx->init_builder, arr_init, arr_ptr);
    LLVMBuildMemSet(dsp_ctx->init_builder, data_i8,
                    LLVMConstInt(LLVMInt8Type(), 0, 0),
                    LLVMConstInt(i64_ty, (uint64_t)(len * el_size), 0), 8);
  }

  LLVMValueRef run_arr_ptr_i8 =
      dsp_consume_frame_state(dsp_ctx, builder, total_bytes, 8, base_name);
  LLVMValueRef run_arr_ptr =
      LLVMBuildBitCast(builder, run_arr_ptr_i8, arr_ptr_ty, "spectral.tmp.ptr");
  return LLVMBuildLoad2(builder, arr_ty, run_arr_ptr, "spectral.tmp.load");
}

static LLVMValueRef spectral_alloc_hoisted_array(Ast *ast, DspBuildCtx *dsp_ctx,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder,
                                                 const char *base_name) {
  int len = dsp_ctx ? dsp_ctx->spectral_fft_size : 0;
  Type *el_type = ast->type->data.T_CONS.args[0];
  return spectral_alloc_hoisted_array_typed(ast, dsp_ctx, ctx, module, builder,
                                            base_name, el_type, len);
}

static DspValue spectral_call_helper(Ast *ast, DspBuildCtx *dsp_ctx,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder,
                                     const char *fn_name, int argc) {
  if (!dsp_ctx || ast->data.AST_APPLICATION.len < (size_t)argc) {
    return DSP_NULL;
  }

  Ast *args = ast->data.AST_APPLICATION.args;
  LLVMValueRef out_arr =
      spectral_alloc_hoisted_array(ast, dsp_ctx, ctx, module, builder, fn_name);
  if (!out_arr) {
    return DSP_NULL;
  }

  LLVMTypeRef arr_ty = LLVMTypeOf(out_arr);
  LLVMTypeRef arg_tys[3] = {arr_ty, arr_ty, arr_ty};
  LLVMValueRef call_args[3] = {0};
  for (int i = 0; i < argc; i++) {
    call_args[i] =
        dsp_build_expr(args + i, dsp_ctx, ctx, module, builder).scalar;
  }
  call_args[argc] = out_arr;

  LLVMTypeRef fn_ty = LLVMFunctionType(arr_ty, arg_tys, argc + 1, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, fn_name);
  if (!fn) {
    fn = LLVMAddFunction(module, fn_name, fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  out_arr = LLVMBuildCall2(builder, fn_ty, fn, call_args, argc + 1, fn_name);
  return DSP_SCALAR(out_arr);
}

DspValue dsp_spectral_env(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {
  return spectral_call_helper(ast, dsp_ctx, ctx, module, builder,
                              "spectral_env_fill", 1);
}

DspValue dsp_spectral_env_real(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  if (!dsp_ctx || ast->data.AST_APPLICATION.len < 2) {
    return DSP_NULL;
  }

  Ast *args = ast->data.AST_APPLICATION.args;
  Ast *size_ast = args;
  Ast *in_ast = args + 1;
  if (!(ast_is_const(size_ast, ctx) && size_ast->tag == AST_INT)) {
    fprintf(stderr,
            "pv_env_real requires a constant integer size as its first argument\n");
    print_ast_err(ast);
    return DSP_NULL;
  }

  int len = size_ast->data.AST_INT.value;
  if (len <= 0 || len > 2048 ||
      (dsp_ctx->spectral_fft_size > 0 && len > dsp_ctx->spectral_fft_size)) {
    fprintf(stderr, "pv_env_real size must be in 1..fft_size (<= 2048)\n");
    print_ast_err(ast);
    return DSP_NULL;
  }

  Type *el_type = &t_num;
  LLVMValueRef out_arr = spectral_alloc_hoisted_array_typed(
      ast, dsp_ctx, ctx, module, builder, "pv_env_real", el_type, len);
  if (!out_arr) {
    return DSP_NULL;
  }

  LLVMValueRef in_arr = dsp_build_expr(in_ast, dsp_ctx, ctx, module, builder).scalar;
  LLVMTypeRef in_arr_ty = LLVMTypeOf(in_arr);
  LLVMTypeRef out_arr_ty = LLVMTypeOf(out_arr);
  LLVMTypeRef fn_ty =
      LLVMFunctionType(out_arr_ty, (LLVMTypeRef[]){in_arr_ty, out_arr_ty}, 2, 0);
  LLVMValueRef fn = LLVMGetNamedFunction(module, "pv_env_real_fill");
  if (!fn) {
    fn = LLVMAddFunction(module, "pv_env_real_fill", fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }

  out_arr = LLVMBuildCall2(builder, fn_ty, fn,
                           (LLVMValueRef[]){in_arr, out_arr}, 2,
                           "pv_env_real");
  return DSP_SCALAR(out_arr);
}

DspValue dsp_spectral_scale_mag(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  return spectral_call_helper(ast, dsp_ctx, ctx, module, builder,
                              "spectral_scale_mag", 2);
}

DspValue dsp_spectral_div_mag(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {
  return spectral_call_helper(ast, dsp_ctx, ctx, module, builder,
                              "spectral_div_mag", 2);
}

// Spectral-domain combinators used only while compiling the spectral function.
FDomValue dsp_freq_map(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

FDomValue dsp_freq_zip(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

// Bin-level primitives used inside spectral lambdas.
LLVMValueRef dsp_bin_mag(LLVMValueRef bin, LLVMModuleRef module,
                         LLVMBuilderRef builder) {}

LLVMValueRef dsp_bin_phase(LLVMValueRef bin, LLVMModuleRef module,
                           LLVMBuilderRef builder) {}

LLVMValueRef dsp_bin_polar(LLVMValueRef r, LLVMValueRef theta,
                           LLVMModuleRef module, LLVMBuilderRef builder) {}

// AST-facing wrappers for builtin dispatch.
LLVMValueRef dsp_mag(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                     LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMValueRef dsp_phase(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMValueRef dsp_polar(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

// Optional domain/type checks if you want explicit guarding in build_expr/fn
// application.
bool dsp_is_freq_signal_type(Type *t) {}
bool dsp_is_bin_type(Type *t) {}

static FDomValue spectral_process(Ast *ast, DspBuildCtx *dsp_ctx,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  if (ast->tag == AST_APPLICATION) {
    Ast *f = ast->data.AST_APPLICATION.function;
    // if (is_ident(f, "fft")) {
    //   return dsp_fft_region(ast, dsp_ctx, ctx, module, builder);
    // }

    if (is_ident(f, "freq_map")) {
      return dsp_freq_map(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "freq_zip")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "mag")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "phase")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "polar")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }
  }
}

// static DspValue dsp_ifft(FDomValue) { return DSP_ZERO; }
//
// DspValue dsp_ifft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
//                          LLVMModuleRef module, LLVMBuilderRef builder) {
//
//   FDomValue spectral_val = spectral_process(ast->data.AST_APPLICATION.args,
//                                             dsp_ctx, ctx, module, builder);
//   return dsp_ifft(spectral_val);
// }

LLVMValueRef CompileSpectralFnHandler(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return CompileAudioFnHandler(ast, ctx, module, builder);
}
