#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "../../lang/backend_llvm/common.h"
#include "../../lang/ylc_datatypes.h"

LLVMValueRef ensure_float(Type *in_type, LLVMValueRef val,
                          LLVMBuilderRef builder);

void ylc_register_synth_ctor(int synth_id, void *ctor);
void *ylc_get_synth_ctor(int synth_id);
int ylc_rand_int(int n);

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
