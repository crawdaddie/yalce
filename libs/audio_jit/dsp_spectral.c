#include "./dsp_spectral.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/type_ser.h"
#include "./common.h"
#include "function.h"
#include <fftw3.h>
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
} SpectralComplex;

SpectralComplex polar(double mag, double theta) {
  return (SpectralComplex){mag * cos(theta), mag * sin(theta)};
}

typedef struct {
  int32_t size;
  SpectralComplex *data;
} SpectralComplexArray;

_Static_assert(sizeof(SpectralComplex) == sizeof(fftw_complex),
               "SpectralComplex must match fftw_complex layout");
_Static_assert(_Alignof(SpectralComplex) == _Alignof(fftw_complex),
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
typedef void (*SpectralKernelFn)(int32_t num_inputs,
                                 const SpectralComplexArray *inputs,
                                 SpectralComplexArray out);

static int spectral_kernel_counter = 0;

static inline double spectral_complex_mag(SpectralComplex z) {
  return sqrt(z.re * z.re + z.im * z.im);
}

static inline SpectralComplex spectral_complex_conjugate(SpectralComplex z) {
  return (SpectralComplex){.re = z.re, .im = -z.im};
}

static inline SpectralComplex spectral_complex_scale(SpectralComplex z,
                                                     double s) {
  return (SpectralComplex){.re = z.re * s, .im = z.im * s};
}

static inline SpectralComplex spectral_complex_normalize_phase(
    SpectralComplex z) {
  double m = spectral_complex_mag(z);
  if (m < 1.0e-12) {
    return (SpectralComplex){.re = 1.0, .im = 0.0};
  }
  return spectral_complex_scale(z, 1.0 / m);
}

static void spectral_smooth_envelope(const SpectralComplexArray in,
                                     double *env_out, int32_t n, int32_t nyq,
                                     int radius) {
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
SpectralComplexArray spectral_vocoder_mod_carrier(SpectralComplexArray mod,
                                                  SpectralComplexArray car,
                                                  SpectralComplexArray out) {
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
    out.data[0] = (SpectralComplex){.re = re, .im = 0.0};
  }

  for (int32_t i = 1; i < nyq; i++) {
    double gain = mod_env[i] / fmax(car_env[i], eps);
    SpectralComplex z = spectral_complex_scale(car.data[i], gain);

    out.data[i] = z;
    out.data[n - i] = spectral_complex_conjugate(z);
  }

  // Nyquist bin must also be purely real for even-length real FFTs.
  {
    double gain = mod_env[nyq] / fmax(car_env[nyq], eps);
    double re = car.data[nyq].re * gain;
    out.data[nyq] = (SpectralComplex){.re = re, .im = 0.0};
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

double spectral_region_samp(void *state_raw, double *inputs, void *kernel_raw) {
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
      SpectralComplexArray input_views[st->num_inputs];
      SpectralComplexArray out_view = {
          .size = st->fft_size,
          .data = (SpectralComplex *)st->out_spec,
      };

      for (int ch = 0; ch < st->num_inputs; ch++) {
        input_views[ch] = (SpectralComplexArray){
            .size = st->fft_size,
            .data = (SpectralComplex *)(spec + ((size_t)ch * st->fft_size)),
        };
      }

      // The compiled spectral lambda runs here. It sees one YLC array per
      // analyzed input spectrum and writes its result directly into the
      // preallocated output spectrum view.
      kernel(st->num_inputs, input_views, out_view);
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

static LLVMValueRef compile_spectral_user_fn(Ast *fn_ast, Type *fn_type,
                                             TypeEnv *env, JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  Ast kernel_ast = *fn_ast;
  char *name = malloc(64);
  if (!name) {
    return NULL;
  }

  snprintf(name, 64, "spectral.kernel.user.%d", spectral_kernel_counter++);
  kernel_ast.type = fn_type;
  kernel_ast.data.AST_LAMBDA.fn_name =
      (ObjString){.chars = name,
                  .length = (int)strlen(name),
                  .hash = hash_string(name, (int)strlen(name))};

  JITLangCtx compilation_ctx = *ctx;
  compilation_ctx.env = env;

  // print_type_env(compilation_ctx.env);
  // printf("codegen spectral function\n");
  // print_ast(&kernel_ast);
  // print_type(fn_type);

  return codegen_fn(&kernel_ast, &compilation_ctx, module, builder);
}

static LLVMValueRef compile_spectral_kernel_wrapper(Ast *fn_ast, Type *fn_type,
                                                    LLVMValueRef user_fn,
                                                    JITLangCtx *ctx,
                                                    LLVMModuleRef module,
                                                    LLVMBuilderRef builder) {
  if (!user_fn || !fn_type) {
    return NULL;
  }

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef complex_ty =
      LLVMStructType((LLVMTypeRef[]){f64_ty, f64_ty}, 2, 0);
  LLVMTypeRef array_ty = codegen_array_type(complex_ty);
  LLVMTypeRef array_ptr_ty = LLVMPointerType(array_ty, 0);
  LLVMTypeRef wrapper_ty = LLVMFunctionType(
      LLVMVoidType(), (LLVMTypeRef[]){i32_ty, array_ptr_ty, array_ty}, 3, 0);

  char *name = malloc(64);
  if (!name) {
    return NULL;
  }
  snprintf(name, 64, "spectral.kernel.wrapper.%d", spectral_kernel_counter++);

  START_FUNC(module, name, wrapper_ty)

  LLVMValueRef inputs_ptr = LLVMGetParam(func, 1);
  LLVMValueRef out_arg = LLVMGetParam(func, 2);
  LLVMTypeRef user_fn_ty = LLVMGlobalGetValueType(user_fn);
  unsigned user_argc = LLVMCountParamTypes(user_fn_ty);
  LLVMValueRef *user_args =
      user_argc ? alloca(sizeof(LLVMValueRef) * user_argc) : NULL;

  for (unsigned i = 0; i < user_argc; i++) {
    if (i + 1 == user_argc && fn_ast->data.AST_LAMBDA.len > 0) {
      user_args[i] = out_arg;
      continue;
    }

    LLVMValueRef idx = LLVMConstInt(i32_ty, i, 0);
    LLVMValueRef in_ptr = LLVMBuildGEP2(builder, array_ty, inputs_ptr, &idx, 1,
                                        "spectral.in.gep");
    user_args[i] = LLVMBuildLoad2(builder, array_ty, in_ptr, "spectral.in");
  }

  Type *ret_type = fn_return_type(fn_type);
  LLVMValueRef ret = LLVMBuildCall2(builder, user_fn_ty, user_fn, user_args,
                                    user_argc, "spectral.user.call");

  if (is_array_type(ret_type)) {
    LLVMValueRef ret_size =
        LLVMBuildExtractValue(builder, ret, 0, "spectral.ret.size");
    LLVMValueRef ret_data =
        LLVMBuildExtractValue(builder, ret, 1, "spectral.ret.data");
    LLVMValueRef out_size =
        LLVMBuildExtractValue(builder, out_arg, 0, "spectral.out.size");
    LLVMValueRef out_data =
        LLVMBuildExtractValue(builder, out_arg, 1, "spectral.out.data");
    LLVMValueRef copy_count = LLVMBuildCall2(
        builder,
        LLVMFunctionType(i32_ty, (LLVMTypeRef[]){i32_ty, i32_ty}, 2, 0),
        LLVMGetNamedFunction(module, "llvm.smin.i32")
            ? LLVMGetNamedFunction(module, "llvm.smin.i32")
            : LLVMAddFunction(module, "llvm.smin.i32",
                              LLVMFunctionType(i32_ty,
                                               (LLVMTypeRef[]){i32_ty, i32_ty},
                                               2, 0)),
        (LLVMValueRef[]){ret_size, out_size}, 2, "spectral.copy.count");

    LLVMValueRef idx_ptr = LLVMBuildAlloca(builder, i32_ty, "spectral.copy.i");
    LLVMBuildStore(builder, LLVMConstInt(i32_ty, 0, 0), idx_ptr);

    LLVMBasicBlockRef loop_bb =
        LLVMAppendBasicBlock(func, "spectral.copy.loop");
    LLVMBasicBlockRef body_bb =
        LLVMAppendBasicBlock(func, "spectral.copy.body");
    LLVMBasicBlockRef exit_bb =
        LLVMAppendBasicBlock(func, "spectral.copy.exit");
    LLVMBuildBr(builder, loop_bb);

    LLVMPositionBuilderAtEnd(builder, loop_bb);
    LLVMValueRef idx_val =
        LLVMBuildLoad2(builder, i32_ty, idx_ptr, "spectral.copy.idx");
    LLVMValueRef keep_going = LLVMBuildICmp(builder, LLVMIntSLT, idx_val,
                                            copy_count, "spectral.copy.lt");
    LLVMBuildCondBr(builder, keep_going, body_bb, exit_bb);

    LLVMPositionBuilderAtEnd(builder, body_bb);
    LLVMValueRef src_ptr = LLVMBuildGEP2(builder, complex_ty, ret_data,
                                         &idx_val, 1, "spectral.copy.src");
    LLVMValueRef dst_ptr = LLVMBuildGEP2(builder, complex_ty, out_data,
                                         &idx_val, 1, "spectral.copy.dst");
    LLVMValueRef elt =
        LLVMBuildLoad2(builder, complex_ty, src_ptr, "spectral.copy.elt");
    LLVMBuildStore(builder, elt, dst_ptr);
    LLVMValueRef next = LLVMBuildAdd(
        builder, idx_val, LLVMConstInt(i32_ty, 1, 0), "spectral.copy.next");
    LLVMBuildStore(builder, next, idx_ptr);
    LLVMBuildBr(builder, loop_bb);

    LLVMPositionBuilderAtEnd(builder, exit_bb);
  }

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
  int total_state_bytes = lane_state_bytes * max_lanes;
  int off = spectral_align_up_i32(dsp_ctx->state_offset, 8);
  dsp_ctx->state_offset = off + total_state_bytes;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMValueRef kernel_ptr = LLVMConstNull(i8_ptr_ty);

  if (fn_ast->tag == AST_LAMBDA && fn_type) {
    LLVMValueRef user_fn = compile_spectral_user_fn(
        fn_ast, fn_type, spectral_fn_compilation_env, ctx, module, builder);
    LLVMDumpValue(user_fn);

    LLVMValueRef wrapper_fn = compile_spectral_kernel_wrapper(
        fn_ast, fn_type, user_fn, ctx, module, builder);
    if (wrapper_fn) {
      kernel_ptr = LLVMBuildBitCast(builder, wrapper_fn, i8_ptr_ty,
                                    "spectral.kernel.ptr");
    }
  }

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
            LLVMConstInt(i64_ty, (uint64_t)(lane * lane_state_bytes), 0);
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
    }
  }

  LLVMValueRef state_base = dsp_consume_frame_state(
      dsp_ctx, builder, total_state_bytes, 8, "spectral.state");

  LLVMTypeRef samp_fn_ty = LLVMFunctionType(
      f64_ty, (LLVMTypeRef[]){i8_ptr_ty, f64_ptr_ty, i8_ptr_ty}, 3, 0);
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
        (LLVMValueRef[]){LLVMBuildBitCast(builder, state_base, i8_ptr_ty,
                                          "spectral.state.i8"),
                         LLVMBuildBitCast(builder, in_storage, f64_ptr_ty,
                                          "spectral.inputs.ptr"),
                         kernel_ptr},
        3, "spectral.sample"));
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
          LLVMConstInt(i64_ty, (uint64_t)(lane * lane_state_bytes), 0);
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
        (LLVMValueRef[]){LLVMBuildBitCast(builder, lane_state, i8_ptr_ty,
                                          "spectral.state.i8"),
                         LLVMBuildBitCast(builder, in_storage, f64_ptr_ty,
                                          "spectral.inputs.ptr"),
                         kernel_ptr},
        3, "spectral.sample");
  }

  return DSP_MULTI(max_lanes, vals);
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
