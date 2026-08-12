#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "../../engine/node.h"
#include "../../lang/backend_llvm/common.h"
#include "../../lang/mir/mir.h"
#include "../../lang/ylc_datatypes.h"

#include <stdbool.h>
#include <stddef.h>

LLVMValueRef ensure_float(Type *in_type, LLVMValueRef val,
                          LLVMBuilderRef builder);

void ylc_register_synth_ctor(int synth_id, void *ctor);
void *ylc_get_synth_ctor(int synth_id);
Node *ylc_create_audio_frame_node(frame_perform_func_t frame_perform,
                                  int num_inputs, int output_layout,
                                  int state_bytes, const char *meta_name);
void ylc_audio_node_set_state_init(void *node_raw, void *init_raw);
int ylc_rand_int(int n);

typedef enum ylc_audio_jit_runtime_arg_kind {
  YLC_AUDIO_JIT_RUNTIME_ARG_CONST_INT,
  YLC_AUDIO_JIT_RUNTIME_ARG_NUM,
} ylc_audio_jit_runtime_arg_kind_t;

typedef struct ylc_audio_jit_runtime_arg {
  ylc_audio_jit_runtime_arg_kind_t kind;
  size_t source_index;
} ylc_audio_jit_runtime_arg_t;

typedef struct ylc_audio_jit_runtime_builtin_desc {
  const char *name;
  size_t source_argc;
  const char *runtime_symbol;
  const ylc_audio_jit_runtime_arg_t *runtime_args;
  size_t runtime_argc;
} ylc_audio_jit_runtime_builtin_desc_t;

bool ylc_audio_jit_register_runtime_double_builtin(
    const ylc_audio_jit_runtime_builtin_desc_t *desc);
MirValueId ylc_audio_jit_emit_synth_voice_array(MirBuilder *builder, Ast *app,
                                                MirCtx *ctx, Ast *size_ast,
                                                Ast *synth_ast);

extern int STYPE_AUDIO_JIT_SYM;
extern int STYPE_AUDIO_JIT_INLINE_SYM;
extern int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
extern int STYPE_AUDIO_JIT_INLINE_LAMBDA;
extern int STYPE_AUDIO_JIT_SYNTH_INLET;
extern int STYPE_AUDIO_JIT_LOCAL_ARRAY;
extern int STYPE_AUDIO_JIT_DSP_VALUE;

typedef struct {
  int32_t fft_size;
  int32_t hop_size;
  int32_t sample_rate;
  int32_t num_frames;
  int32_t num_bins;
  _DoubleArray mag;
  _DoubleArray phase;
  _DoubleArray phase_inc;
  _DoubleArray transient;
} YLC_SpectralAnalysis;

typedef struct {
  int initialized;
  int fft_size;
  int hop_size;
  int num_bins;
  int num_frames;
  double playhead_frame;
  double prev_trig;
  int output_index;
  int output_frames;
  double *window;
  double *synth_phase;
  void *ifft_spec;
  double *ifft_out;
  double *ola;
  double *output_cache;
  void *ifft_plan;
} PVBufplayRuntimeState;

typedef struct {
  int initialized;
  int sample_rate;
  int delaybufsize;
  int mask;
  int iwrphase;
  int numoutput;
  int framesize;
  int counter;
  int stage;
  double last_winsize;
  double slope;
  double dsamp[4];
  double dsamp_slope[4];
  double ramp[4];
  double ramp_slope[4];
  float *dlybuf;
} PitchShiftRuntimeState;

void *fftw_plan_forward_new(int fft_size);
void fftw_forward_execute_raw(void *plan, int32_t out_re_size,
                              double *out_re_data, int32_t out_im_size,
                              double *out_im_data, int32_t input_size,
                              double *input_data);
void fftw_plan_free(void *plan);
void dsp_pv_bufplay_state_init(void *state_raw);
void dsp_pv_bufplay_next_frame(void *state_raw, YLC_SpectralAnalysis *analysis,
                               double pitch_ratio, double stretch_ratio,
                               double start_pos, double trig);
double dsp_pv_bufplay_get_sample(void *state_raw);
int dsp_pitchshift_state_bytes_for(int sample_rate, double winsize);
void dsp_pitchshift_state_init(void *state_raw, int sample_rate,
                               double winsize);
double dsp_pitchshift_next_sample(void *state_raw, double input,
                                  double pitch_ratio, double pitch_dispersion,
                                  double time_dispersion);
#endif
