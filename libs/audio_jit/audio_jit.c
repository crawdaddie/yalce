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
#include "./dsp_fn_application.h"
#include "./pattern_coroutine.h"
#include "./play_handler.h"

#include <fftw3.h>
#include <llvm-c/Core.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
int STYPE_AUDIO_JIT_INLINE_LAMBDA;
int STYPE_AUDIO_JIT_SYNTH_INLET;
int STYPE_AUDIO_JIT_LOCAL_ARRAY;
int STYPE_AUDIO_JIT_DSP_VALUE;

typedef struct FFTWForwardPlan {
  int fft_size;
  int num_bins;
  double *in;
  fftw_complex *out;
  fftw_plan plan;
} FFTWForwardPlan;

static int pv_debug_init_logged = 0;
static int pv_debug_hop_logged = 0;
static int pv_debug_sample_logged = 0;

static int pitchshift_align_i32(int value, int align) {
  return (value + align - 1) & ~(align - 1);
}

static int pitchshift_next_power_of_two(int value) {
  int out = 1;
  while (out < value) {
    out <<= 1;
  }
  return out;
}

static double pitchshift_rand_unit(void) {
  return (double)rand() / (double)RAND_MAX;
}

static double pitchshift_rand_bipolar(void) {
  return (2.0 * pitchshift_rand_unit()) - 1.0;
}

static int pitchshift_delaybufsize_for(double winsize, int sample_rate) {
  double minimum_winsize = 3.0 / (double)sample_rate;
  if (winsize < minimum_winsize) {
    winsize = minimum_winsize;
  }
  int delaybufsize = (int)ceil(winsize * (double)sample_rate * 3.0 + 3.0);
  if (delaybufsize < 8) {
    delaybufsize = 8;
  }
  return pitchshift_next_power_of_two(delaybufsize);
}

static void pitchshift_configure(PitchShiftRuntimeState *st, double winsize) {
  if (!st || !st->dlybuf || st->sample_rate <= 0 || st->delaybufsize <= 0) {
    return;
  }

  double minimum_winsize = 3.0 / (double)st->sample_rate;
  if (winsize < minimum_winsize) {
    winsize = minimum_winsize;
  }

  memset(st->dlybuf, 0, sizeof(float) * (size_t)st->delaybufsize);
  st->last_winsize = winsize;
  st->mask = st->delaybufsize - 1;
  st->iwrphase = 0;
  st->numoutput = 0;
  st->framesize = (((int)(winsize * (double)st->sample_rate)) + 2) & ~3;
  if (st->framesize < 4) {
    st->framesize = 4;
  }
  st->slope = 2.0 / (double)st->framesize;
  st->stage = 3;
  st->counter = st->framesize >> 2;

  st->ramp[0] = 0.5;
  st->ramp[1] = 1.0;
  st->ramp[2] = 0.5;
  st->ramp[3] = 0.0;

  st->ramp_slope[0] = -st->slope;
  st->ramp_slope[1] = -st->slope;
  st->ramp_slope[2] = st->slope;
  st->ramp_slope[3] = st->slope;

  for (int i = 0; i < 4; i++) {
    st->dsamp[i] = 2.0;
    st->dsamp_slope[i] = 1.0;
  }

  st->dlybuf[st->mask] = 0.0f;
  st->dlybuf[(st->mask - 1) & st->mask] = 0.0f;
  st->dlybuf[(st->mask - 2) & st->mask] = 0.0f;
  st->initialized = 1;
}

static void pv_bufplay_runtime_free(PVBufplayRuntimeState *st) {
  if (!st) {
    return;
  }
  if (st->ifft_plan) {
    fftw_destroy_plan((fftw_plan)st->ifft_plan);
    st->ifft_plan = NULL;
  }
  if (st->ifft_spec) {
    fftw_free(st->ifft_spec);
    st->ifft_spec = NULL;
  }
  if (st->ifft_out) {
    fftw_free(st->ifft_out);
    st->ifft_out = NULL;
  }
  free(st->window);
  st->window = NULL;
  free(st->synth_phase);
  st->synth_phase = NULL;
  free(st->ola);
  st->ola = NULL;
  free(st->output_cache);
  st->output_cache = NULL;
  st->initialized = 0;
}

static int pv_wrap_frame_index(int idx, int num_frames) {
  if (num_frames <= 0) {
    return 0;
  }
  while (idx < 0) {
    idx += num_frames;
  }
  while (idx >= num_frames) {
    idx -= num_frames;
  }
  return idx;
}

static int pv_playable_frame_count(const YLC_SpectralAnalysis *analysis) {
  if (!analysis || analysis->num_frames <= 1) {
    return analysis && analysis->num_frames > 0 ? 1 : 0;
  }
  return analysis->num_frames - 1;
}

static void pv_seed_phase_from_frame(PVBufplayRuntimeState *st,
                                     const YLC_SpectralAnalysis *analysis,
                                     int frame_idx) {
  if (!st || !analysis || !analysis->phase.data || st->num_bins <= 0) {
    return;
  }
  int playable = pv_playable_frame_count(analysis);
  if (playable <= 0) {
    return;
  }
  frame_idx = pv_wrap_frame_index(frame_idx, playable);
  for (int k = 0; k < st->num_bins; k++) {
    int idx = frame_idx * st->num_bins + k;
    st->synth_phase[k] = analysis->phase.data[idx];
  }
}

static int pv_bufplay_runtime_init(PVBufplayRuntimeState *st,
                                   YLC_SpectralAnalysis *analysis) {
  if (!st || !analysis || analysis->fft_size <= 0 || analysis->hop_size <= 0 ||
      analysis->num_bins <= 0 || analysis->num_frames <= 0 ||
      !analysis->mag.data || !analysis->phase.data ||
      !analysis->phase_inc.data) {
    return 0;
  }

  pv_bufplay_runtime_free(st);

  st->fft_size = analysis->fft_size;
  st->hop_size = analysis->hop_size;
  st->num_bins = analysis->num_bins;
  st->num_frames = analysis->num_frames;
  st->playhead_frame = 0.0;
  st->prev_trig = 0.0;
  st->output_index = 0;
  st->output_frames = 0;

  st->window = (double *)calloc((size_t)st->fft_size, sizeof(double));
  st->synth_phase = (double *)calloc((size_t)st->num_bins, sizeof(double));
  st->ifft_spec = fftw_alloc_complex((size_t)st->num_bins);
  st->ifft_out = fftw_alloc_real((size_t)st->fft_size);
  st->ola = (double *)calloc((size_t)st->fft_size, sizeof(double));
  st->output_cache = (double *)calloc((size_t)st->hop_size, sizeof(double));

  if (!st->window || !st->synth_phase || !st->ifft_spec || !st->ifft_out ||
      !st->ola || !st->output_cache) {
    pv_bufplay_runtime_free(st);
    return 0;
  }

  for (int i = 0; i < st->fft_size; i++) {
    st->window[i] =
        0.5 - 0.5 * cos((2.0 * M_PI * (double)i) / (double)(st->fft_size - 1));
  }

  st->ifft_plan = (void *)fftw_plan_dft_c2r_1d(
      st->fft_size, (fftw_complex *)st->ifft_spec, st->ifft_out, FFTW_MEASURE);
  if (!st->ifft_plan) {
    pv_bufplay_runtime_free(st);
    return 0;
  }

  st->initialized = 1;
  if (pv_debug_init_logged < 4) {
    fprintf(stderr,
            "pv init fft=%d hop=%d bins=%d frames=%d mag=%p phase=%p inc=%p\n",
            st->fft_size, st->hop_size, st->num_bins, st->num_frames,
            (void *)analysis->mag.data, (void *)analysis->phase.data,
            (void *)analysis->phase_inc.data);
    pv_debug_init_logged++;
  }
  return 1;
}

static void pv_bufplay_runtime_reset(PVBufplayRuntimeState *st,
                                     YLC_SpectralAnalysis *analysis,
                                     double start_pos) {
  if (!st || !st->initialized || !analysis) {
    return;
  }
  if (start_pos < 0.0) {
    start_pos = 0.0;
  }
  if (start_pos > 1.0) {
    start_pos = 1.0;
  }
  memset(st->ola, 0, sizeof(double) * (size_t)st->fft_size);
  memset(st->output_cache, 0, sizeof(double) * (size_t)st->hop_size);
  st->output_index = 0;
  st->output_frames = 0;
  int start_frame = 0;
  int playable = pv_playable_frame_count(analysis);
  if (playable > 0) {
    start_frame = (int)(start_pos * (double)(playable - 1));
    if (start_frame < 0) {
      start_frame = 0;
    }
    if (start_frame >= playable) {
      start_frame = playable - 1;
    }
    st->playhead_frame = (double)start_frame;
  } else {
    st->playhead_frame = 0.0;
  }
  pv_seed_phase_from_frame(st, analysis, start_frame);
}

static void pv_bufplay_render_hop(PVBufplayRuntimeState *st,
                                  YLC_SpectralAnalysis *analysis,
                                  double pitch_ratio, double stretch_ratio) {
  if (!st || !st->initialized || !analysis || analysis->num_frames <= 0) {
    return;
  }

  if (pitch_ratio <= 0.0) {
    pitch_ratio = 1.0;
  }
  if (stretch_ratio <= 0.0) {
    stretch_ratio = 1.0;
  }

  int playable = pv_playable_frame_count(analysis);
  if (playable <= 0) {
    return;
  }
  while (st->playhead_frame < 0.0) {
    st->playhead_frame += (double)playable;
  }
  while (st->playhead_frame >= (double)playable) {
    st->playhead_frame -= (double)playable;
  }
  int frame0 = (int)floor(st->playhead_frame);
  double frac = st->playhead_frame - (double)frame0;
  int frame1 = pv_wrap_frame_index(frame0 + 1, playable);
  fftw_complex *ifft_spec = (fftw_complex *)st->ifft_spec;

  memset(ifft_spec, 0, sizeof(fftw_complex) * (size_t)st->num_bins);

  for (int k = 0; k < st->num_bins; k++) {
    int idx0 = frame0 * st->num_bins + k;
    int idx1 = frame1 * st->num_bins + k;
    double mag0 = analysis->mag.data[idx0];
    double mag1 = analysis->mag.data[idx1];
    double inc0 = analysis->phase_inc.data[idx0];
    double inc1 = analysis->phase_inc.data[idx1];
    double mag = mag0 + ((mag1 - mag0) * frac);
    double inc = inc0 + ((inc1 - inc0) * frac);

    st->synth_phase[k] += inc;

    double re = mag * cos(st->synth_phase[k]);
    double im = mag * sin(st->synth_phase[k]);
    double dst = (double)k * pitch_ratio;
    int j0 = (int)floor(dst);
    double a = dst - (double)j0;

    if (j0 >= 0 && j0 < st->num_bins) {
      ifft_spec[j0][0] += re * (1.0 - a);
      ifft_spec[j0][1] += im * (1.0 - a);
    }
    int j1 = j0 + 1;
    if (j1 >= 0 && j1 < st->num_bins) {
      ifft_spec[j1][0] += re * a;
      ifft_spec[j1][1] += im * a;
    }
  }

  fftw_execute((fftw_plan)st->ifft_plan);

  for (int i = 0; i < st->fft_size; i++) {
    double sample = (st->ifft_out[i] / (double)st->fft_size) * st->window[i];
    st->ola[i] += sample;
  }

  for (int i = 0; i < st->hop_size; i++) {
    st->output_cache[i] = st->ola[i];
  }

  if (pv_debug_hop_logged < 8) {
    fprintf(stderr,
            "pv hop frame0=%d frac=%f out[0..3]=%f %f %f %f mag0=%f inc0=%f "
            "phase0=%f\n",
            frame0, frac, st->output_cache[0],
            st->hop_size > 1 ? st->output_cache[1] : 0.0,
            st->hop_size > 2 ? st->output_cache[2] : 0.0,
            st->hop_size > 3 ? st->output_cache[3] : 0.0,
            analysis->mag.data[frame0 * st->num_bins],
            analysis->phase_inc.data[frame0 * st->num_bins],
            st->synth_phase[0]);
    pv_debug_hop_logged++;
  }

  memmove(st->ola, st->ola + st->hop_size,
          sizeof(double) * (size_t)(st->fft_size - st->hop_size));
  memset(st->ola + (st->fft_size - st->hop_size), 0,
         sizeof(double) * (size_t)st->hop_size);

  st->output_frames = st->hop_size;
  st->output_index = 0;
  st->playhead_frame += 1.0 / stretch_ratio;
  if (st->playhead_frame >= (double)playable) {
    st->playhead_frame -= (double)playable;
    pv_seed_phase_from_frame(st, analysis, (int)floor(st->playhead_frame));
  }
}

// let array_of_buf = extern fn Ptr -> Array of Double;
// let bufsize = extern fn Ptr -> Int;
_DoubleArray array_of_buf(NodeRef buf) {
  return (_DoubleArray){.data = buf->output.buf, .size = buf->output.size};
}
int bufsize(NodeRef buf) { return buf->output.size; }
void *ylc_get_output_buf(void *node_raw) {
  return ((Node *)node_raw)->output.buf;
}
int64_t ylc_bufsize(void *node_raw) {
  return (int64_t)((Node *)node_raw)->output.size;
}

Node *ylc_create_audio_node(perform_func_t perform, int num_inputs,
                            int output_layout, int state_bytes,
                            const char *meta_name) {
  size_t total = sizeof(Node) + (size_t)state_bytes +
                 ((size_t)BUF_SIZE * (size_t)output_layout * sizeof(double));
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return NULL;
  }

  node->perform = perform;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
  node->meta = (char *)meta_name;
  node->output = (Signal){
      .layout = output_layout,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->next = NULL;

  return node;
}

void ylc_register_synth_ctor(int synth_id, void *ctor) {
  synth_registry_set_ctor_ptr(synth_id, ctor);
}

void *ylc_get_synth_ctor(int synth_id) {
  return synth_registry_get_ctor_ptr(synth_id);
}

int ylc_rand_int(int n) {
  if (n <= 1)
    return 0;
  return rand() % n;
}

void *fftw_plan_forward_new(int fft_size) {
  if (fft_size <= 0) {
    return NULL;
  }

  FFTWForwardPlan *st = calloc(1, sizeof(FFTWForwardPlan));
  if (!st) {
    return NULL;
  }

  st->fft_size = fft_size;
  st->num_bins = fft_size / 2 + 1;
  st->in = fftw_alloc_real((size_t)fft_size);
  st->out = fftw_alloc_complex((size_t)st->num_bins);
  if (!st->in || !st->out) {
    if (st->in) {
      fftw_free(st->in);
    }
    if (st->out) {
      fftw_free(st->out);
    }
    free(st);
    return NULL;
  }

  st->plan = fftw_plan_dft_r2c_1d(fft_size, st->in, st->out, FFTW_MEASURE);
  if (!st->plan) {
    fftw_free(st->in);
    fftw_free(st->out);
    free(st);
    return NULL;
  }

  return (void *)st;
}

void fftw_forward_execute_raw(void *plan_raw, int32_t out_re_size,
                              double *out_re_data, int32_t out_im_size,
                              double *out_im_data, int32_t input_size,
                              double *input_data) {

  FFTWForwardPlan *st = (FFTWForwardPlan *)plan_raw;
  if (!st) {
    return;
  }

  if (input_size < st->fft_size || out_re_size < st->num_bins ||
      out_im_size < st->num_bins || !input_data || !out_re_data ||
      !out_im_data) {
    return;
  }

  memcpy(st->in, input_data, sizeof(double) * (size_t)st->fft_size);
  fftw_execute(st->plan);

  for (int i = 0; i < st->num_bins; i++) {
    out_re_data[i] = st->out[i][0];
    out_im_data[i] = st->out[i][1];
  }
}

void fftw_plan_free(void *plan) {
  if (!plan) {
    return;
  }
  FFTWForwardPlan *st = (FFTWForwardPlan *)plan;
  if (st->plan) {
    fftw_destroy_plan(st->plan);
  }
  if (st->in) {
    fftw_free(st->in);
  }
  if (st->out) {
    fftw_free(st->out);
  }
  free(st);
}

void dsp_pv_bufplay_next_frame(void *state_raw, YLC_SpectralAnalysis *analysis,
                               double pitch_ratio, double stretch_ratio,
                               double start_pos, double trig) {
  PVBufplayRuntimeState *st = (PVBufplayRuntimeState *)state_raw;
  if (!st) {
    return;
  }

  if (!analysis) {
    return;
  }

  if (!st->initialized || st->fft_size != analysis->fft_size ||
      st->hop_size != analysis->hop_size ||
      st->num_bins != analysis->num_bins ||
      st->num_frames != analysis->num_frames) {
    if (!pv_bufplay_runtime_init(st, analysis)) {
      return;
    }
  }

  if (trig >= 0.5 && st->prev_trig < 0.5) {
    pv_bufplay_runtime_reset(st, analysis, start_pos);
  }
  st->prev_trig = trig;

  if (st->output_index >= st->output_frames) {
    pv_bufplay_render_hop(st, analysis, pitch_ratio, stretch_ratio);
  }
}

void dsp_pv_bufplay_state_init(void *state_raw) {
  PVBufplayRuntimeState *st = (PVBufplayRuntimeState *)state_raw;
  if (!st) {
    return;
  }
  memset(st, 0, sizeof(*st));
}

double dsp_pv_bufplay_get_sample(void *state_raw) {
  PVBufplayRuntimeState *st = (PVBufplayRuntimeState *)state_raw;
  if (!st || !st->initialized || !st->output_cache || st->hop_size <= 0) {
    // printf("%p %d %d %d\n", st, st->initialized, st->output_cache,
    //        st->hop_size);
    return 0.0;
  }
  if (st->output_index >= st->output_frames) {
    return 0.0;
  }
  double sample = st->output_cache[st->output_index++];
  if (pv_debug_sample_logged < 16) {
    fprintf(stderr, "pv sample idx=%d val=%f\n", st->output_index - 1, sample);
    pv_debug_sample_logged++;
  }
  return sample;
}

int dsp_pitchshift_state_bytes_for(int sample_rate, double winsize) {
  if (sample_rate <= 0) {
    sample_rate = 48000;
  }
  int delaybufsize = pitchshift_delaybufsize_for(winsize, sample_rate);
  int bytes = pitchshift_align_i32((int)sizeof(PitchShiftRuntimeState), 8);
  bytes += (int)(sizeof(float) * (size_t)delaybufsize);
  return pitchshift_align_i32(bytes, 8);
}

void dsp_pitchshift_state_init(void *state_raw, int sample_rate,
                               double winsize) {
  if (!state_raw) {
    return;
  }

  PitchShiftRuntimeState *st = (PitchShiftRuntimeState *)state_raw;
  memset(st, 0, sizeof(*st));

  if (sample_rate <= 0) {
    sample_rate = 48000;
  }
  st->sample_rate = sample_rate;
  st->delaybufsize = pitchshift_delaybufsize_for(winsize, sample_rate);
  st->dlybuf =
      (float *)((char *)state_raw +
                pitchshift_align_i32((int)sizeof(PitchShiftRuntimeState), 8));
  pitchshift_configure(st, winsize);
}

double dsp_pitchshift_next_sample(void *state_raw, double input,
                                  double pitch_ratio, double pitch_dispersion,
                                  double time_dispersion) {
  PitchShiftRuntimeState *st = (PitchShiftRuntimeState *)state_raw;
  if (!st || !st->initialized || !st->dlybuf || st->delaybufsize <= 0) {
    return 0.0;
  }

  if (pitch_ratio < 0.0) {
    pitch_ratio = 0.0;
  } else if (pitch_ratio > 4.0) {
    pitch_ratio = 4.0;
  }

  if (time_dispersion < 0.0) {
    time_dispersion = 0.0;
  } else if (time_dispersion > st->last_winsize) {
    time_dispersion = st->last_winsize;
  }
  double timedisp_samples = time_dispersion * (double)st->sample_rate;

  if (st->counter <= 0) {
    st->counter = st->framesize >> 2;
    st->stage = (st->stage + 1) & 3;

    double disppchratio = pitch_ratio;
    if (pitch_dispersion != 0.0) {
      disppchratio += pitch_dispersion * pitchshift_rand_bipolar();
    }
    if (disppchratio < 0.0) {
      disppchratio = 0.0;
    } else if (disppchratio > 4.0) {
      disppchratio = 4.0;
    }

    double pchratio1 = disppchratio - 1.0;
    double samp_slope = -pchratio1;
    double startpos =
        pchratio1 < 0.0 ? 2.0 : (double)st->framesize * pchratio1 + 2.0;
    startpos += timedisp_samples * pitchshift_rand_unit();

    switch (st->stage) {
    case 0:
      st->dsamp_slope[0] = samp_slope;
      st->dsamp[0] = startpos;
      st->ramp[0] = 0.0;
      st->ramp_slope[0] = st->slope;
      st->ramp_slope[2] = -st->slope;
      break;
    case 1:
      st->dsamp_slope[1] = samp_slope;
      st->dsamp[1] = startpos;
      st->ramp[1] = 0.0;
      st->ramp_slope[1] = st->slope;
      st->ramp_slope[3] = -st->slope;
      break;
    case 2:
      st->dsamp_slope[2] = samp_slope;
      st->dsamp[2] = startpos;
      st->ramp[2] = 0.0;
      st->ramp_slope[2] = st->slope;
      st->ramp_slope[0] = -st->slope;
      break;
    default:
      st->dsamp_slope[3] = samp_slope;
      st->dsamp[3] = startpos;
      st->ramp[3] = 0.0;
      st->ramp_slope[1] = -st->slope;
      st->ramp_slope[3] = st->slope;
      break;
    }
  }

  st->counter--;
  st->numoutput++;
  st->iwrphase = (st->iwrphase + 1) & st->mask;

  double value = 0.0;
  for (int i = 0; i < 4; i++) {
    st->dsamp[i] += st->dsamp_slope[i];
    int idsamp = (int)st->dsamp[i];
    double frac = st->dsamp[i] - (double)idsamp;
    int irdphase = (st->iwrphase - idsamp) & st->mask;
    int irdphaseb = (irdphase - 1) & st->mask;

    double sample = 0.0;
    if (st->numoutput < st->delaybufsize) {
      if (irdphase > st->iwrphase) {
        sample = 0.0;
      } else if (irdphaseb > st->iwrphase) {
        double d1 = st->dlybuf[irdphase];
        sample = d1 - frac * d1;
      } else {
        double d1 = st->dlybuf[irdphase];
        double d2 = st->dlybuf[irdphaseb];
        sample = d1 + frac * (d2 - d1);
      }
    } else {
      double d1 = st->dlybuf[irdphase];
      double d2 = st->dlybuf[irdphaseb];
      sample = d1 + frac * (d2 - d1);
    }

    value += sample * st->ramp[i];
    st->ramp[i] += st->ramp_slope[i];
  }

  st->dlybuf[st->iwrphase] = (float)input;
  return value * 0.5;
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
  STYPE_AUDIO_JIT_LOCAL_ARRAY = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_DSP_VALUE = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_LIVE_PATTERN = REGISTERED_JIT_SYMBOL_TYPE++;

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "compile_audio_fn", CompileAudioFnHandler);
  register_builtin(stack, "compile_spectral_fn", CompileSpectralFnHandler);
  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");
  printf("sample rates: %d %f\n", ctx_sample_rate(), ctx_spf());

  register_builtin(stack, "pat", pattern_handler);
  register_builtin(stack, "pat_key", pattern_key_handler);
  register_builtin(stack, "pat_key_chars", pattern_key_chars_handler);

  register_builtin(stack, "play_pattern", play_pattern_handler);
  register_builtin(stack, "play", play_module_handler);
}

void *null_synth_ptr() { return NULL; }
