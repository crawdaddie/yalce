#include "./osc_kernels.h"
#include "node.h"
#include "ylc_datatypes.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static inline double audio_jit_wrap_index(double index, double size) {
  return index - floor(index / size) * size;
}

static inline double *audio_jit_array_data(int32_t offset, double *data) {
  return data ? data + offset : NULL;
}

static inline double audio_jit_lerp(double a, double b, double t) {
  return a + ((b - a) * t);
}

static inline double audio_jit_rand_range(double lo, double hi) {
  return lo + ((double)rand() / (double)RAND_MAX) * (hi - lo);
}

static inline int audio_jit_rising_edge(double trig, double prev_trig) {
  return prev_trig < 0.5 && trig >= 0.5;
}

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE] = {
#include "./sin_table.csv"
};

__attribute__((always_inline)) double
ylc_audio_sin_osc_kernel(SinOscState *state, double spf, double freq) {
  const int table_mask = SIN_TABSIZE - 1; // Assuming SIN_TABSIZE is power of 2

  double d_index = state->phase * (SIN_TABSIZE);
  int index = (int)d_index;
  double frac = d_index - index;

  double a = sin_table[index & table_mask];
  double b = sin_table[(index + 1) & table_mask];

  double sample = (1.0 - frac) * a + (frac * b);
  state->phase = fmod(state->phase + freq * spf, 1.0);
  return sample;
}

#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {
#include "./sq_table.csv"
};

__attribute__((always_inline)) double
ylc_audio_sq_osc_kernel(SqOscState *state, double spf, double freq) {

  const int table_mask = SQ_TABSIZE - 1;
  double d_index = state->phase * SQ_TABSIZE;
  int index = (int)d_index;
  double frac = d_index - index;
  double a = sq_table[index & table_mask];
  double b = sq_table[(index + 1) & table_mask];

  double sample = (1.0 - frac) * a + (frac * b);
  state->phase = fmod(state->phase + freq * spf, 1.0);
  return sample;
}

#define SAW_TABSIZE (1 << 11)
static double saw_table[SAW_TABSIZE] = {
#include "./saw_table.csv"
};

__attribute__((always_inline)) double
ylc_audio_saw_osc_kernel(SawOscState *state, double spf, double freq) {

  const int table_mask = SAW_TABSIZE - 1;
  double d_index = state->phase * SAW_TABSIZE;
  int index = (int)d_index;
  double frac = d_index - index;
  double a = saw_table[index & table_mask];
  double b = saw_table[(index + 1) & table_mask];

  double sample = (1.0 - frac) * a + (frac * b);
  state->phase = fmod(state->phase + freq * spf, 1.0);
  return sample;
}

__attribute__((always_inline)) double
ylc_audio_pm_osc_kernel(PmOscState *state, double spf, double mod_index,
                        double mod_ratio, double freq) {

  const double table_size = (double)SIN_TABSIZE;
  const int table_mask = SIN_TABSIZE - 1;

  double carrier_freq, modulator_freq;
  double mod_phase_scaled, carrier_phase_scaled;
  int mod_index_int, carrier_index_int;
  double mod_frac, carrier_frac;
  double modulator_value, carrier_value;
  double modulated_phase;

  carrier_freq = freq;

  modulator_freq = carrier_freq * mod_ratio;

  mod_phase_scaled = state->modulator_phase * table_size;
  mod_index_int = (int)mod_phase_scaled;
  mod_frac = mod_phase_scaled - mod_index_int;

  mod_index_int &= table_mask;

  int mod_idx_0 = (mod_index_int - 1) & table_mask;
  int mod_idx_1 = mod_index_int;
  int mod_idx_2 = (mod_index_int + 1) & table_mask;
  int mod_idx_3 = (mod_index_int + 2) & table_mask;

  double mod_y0 = sin_table[mod_idx_0];
  double mod_y1 = sin_table[mod_idx_1];
  double mod_y2 = sin_table[mod_idx_2];
  double mod_y3 = sin_table[mod_idx_3];

  double mod_c0 = mod_y1;
  double mod_c1 = 0.5 * (mod_y2 - mod_y0);
  double mod_c2 = mod_y0 - 2.5 * mod_y1 + 2.0 * mod_y2 - 0.5 * mod_y3;
  double mod_c3 = 0.5 * (mod_y3 - mod_y0) + 1.5 * (mod_y1 - mod_y2);

  modulator_value =
      ((mod_c3 * mod_frac + mod_c2) * mod_frac + mod_c1) * mod_frac + mod_c0;

  modulator_value *= mod_index;

  modulated_phase = state->carrier_phase + modulator_value;
  modulated_phase -= floor(modulated_phase);

  carrier_phase_scaled = modulated_phase * table_size;
  carrier_index_int = (int)carrier_phase_scaled;
  carrier_frac = carrier_phase_scaled - carrier_index_int;

  carrier_index_int &= table_mask;

  int carr_idx_0 = (carrier_index_int - 1) & table_mask;
  int carr_idx_1 = carrier_index_int;
  int carr_idx_2 = (carrier_index_int + 1) & table_mask;
  int carr_idx_3 = (carrier_index_int + 2) & table_mask;

  double carr_y0 = sin_table[carr_idx_0];
  double carr_y1 = sin_table[carr_idx_1];
  double carr_y2 = sin_table[carr_idx_2];
  double carr_y3 = sin_table[carr_idx_3];

  double carr_c0 = carr_y1;
  double carr_c1 = 0.5 * (carr_y2 - carr_y0);
  double carr_c2 = carr_y0 - 2.5 * carr_y1 + 2.0 * carr_y2 - 0.5 * carr_y3;
  double carr_c3 = 0.5 * (carr_y3 - carr_y0) + 1.5 * (carr_y1 - carr_y2);

  carrier_value =
      ((carr_c3 * carrier_frac + carr_c2) * carrier_frac + carr_c1) *
          carrier_frac +
      carr_c0;

  state->modulator_phase += modulator_freq * spf;
  state->modulator_phase -= floor(state->modulator_phase);

  state->carrier_phase += carrier_freq * spf;
  state->carrier_phase -= floor(state->carrier_phase);
  return carrier_value;
}

__attribute__((always_inline)) double
ylc_audio_phasor_kernel(PhasorState *state, double spf, double freq) {
  double phase = state->phase;
  state->phase = fmod(state->phase + freq * spf, 1.0);
  return phase;
}

__attribute__((always_inline)) double
ylc_audio_trig_once_kernel(TrigOnceState *state, double spf, double freq) {
  (void)spf;
  (void)freq;
  if (state->fired) {
    return 0.0;
  }
  state->fired = 1;
  return 1.0;
}

__attribute__((always_inline)) double
ylc_audio_trig_kernel(TrigState *state, double spf, double freq) {
  double phase = state->phase;
  double step = freq * spf;
  double advanced = phase + step;
  double wrapped = step >= 0.0 ? advanced >= 1.0 : advanced < 0.0;
  double out = (!state->initialized || wrapped) ? 1.0 : 0.0;

  state->phase = advanced - floor(advanced);
  state->initialized = 1;
  return out;
}

__attribute__((always_inline)) double
ylc_audio_changed_kernel(ChangedState *state, double spf, double input) {
  (void)spf;
  double out = state->seen && input != state->prev ? 1.0 : 0.0;
  state->prev = input;
  state->seen = 1;
  return out;
}

static inline double audio_jit_tabread_core(int32_t size, int32_t offset,
                                            double *raw_data, double phase,
                                            int phase_is_normalized) {
  double *data = audio_jit_array_data(offset, raw_data);
  int32_t len = size;
  if (!data || len <= 0 || !isfinite(phase)) {
    return 0.0;
  }

  double len_f = (double)len;
  double index = phase_is_normalized ? phase * len_f : phase;
  double wrapped = audio_jit_wrap_index(index, len_f);
  if (wrapped < 0.0) {
    wrapped += len_f;
  }
  if (wrapped >= len_f) {
    wrapped = 0.0;
  }

  double i0_f = floor(wrapped);
  int32_t i0 = (int32_t)i0_f;
  double frac = wrapped - i0_f;
  int32_t i1 = i0 + 1;
  if (i1 >= len) {
    i1 = 0;
  }

  return audio_jit_lerp(data[i0], data[i1], frac);
}

__attribute__((always_inline)) double
ylc_audio_tabread_kernel(void *state, double spf, int32_t size, int32_t offset,
                         double *data, double phase) {
  (void)state;
  (void)spf;
  return audio_jit_tabread_core(size, offset, data, phase, 1);
}

__attribute__((always_inline)) double
ylc_audio_tabread_samp_kernel(void *state, double spf, int32_t size,
                              int32_t offset, double *data,
                              double sample_index) {
  (void)state;
  (void)spf;
  return audio_jit_tabread_core(size, offset, data, sample_index, 0);
}

__attribute__((always_inline)) double
ylc_audio_bufplay_kernel(BufplayState *state, double spf, int32_t size,
                         int32_t offset, double *data, double rate,
                         double start_pos, double trig) {
  return 0.0;
}

__attribute__((always_inline)) double
ylc_audio_mbufplay_kernel(BufplayState *state, double spf, int32_t channels,
                          int32_t channel, int32_t size, int32_t offset,
                          double *data, double rate, double start_pos,
                          double trig) {
  return 0.0;
}

__attribute__((always_inline)) double ylc_audio_decay_kernel(DecayState *state,
                                                             double spf,
                                                             double decay_time,
                                                             double trig) {
  double current =
      audio_jit_rising_edge(trig, state->prev_trig) ? 1.0 : state->value;
  double multiplier = decay_time > 0.0 ? exp(-spf / decay_time) : 0.0;
  state->value = current * multiplier;
  state->prev_trig = trig;
  return current;
}

__attribute__((always_inline)) double
ylc_audio_scale_kernel(void *state, double spf, double lo, double hi,
                       double value) {
  (void)state;
  (void)spf;
  return lo + value * (hi - lo);
}

__attribute__((always_inline)) double
ylc_audio_scale_bp_kernel(void *state, double spf, double lo, double hi,
                          double value) {
  (void)state;
  (void)spf;
  return lo + ((value + 1.0) * 0.5) * (hi - lo);
}

__attribute__((always_inline)) _DoubleArray
ylc_audio_array_of_buf(void *node_raw) {
  Node *node = (Node *)node_raw;
  if (!node || !node->output.buf || node->output.size <= 0) {
    return (_DoubleArray){0, 0, NULL};
  }

  int layout = node->output.layout > 0 ? node->output.layout : 1;
  return (_DoubleArray){
      .size = node->output.size * layout,
      .offset = 0,
      .data = node->output.buf,
  };
}

_DoubleArray array_of_buf(void *node_raw) {
  return ylc_audio_array_of_buf(node_raw);
}

__attribute__((always_inline)) _DoubleArray
ylc_audio_array_of_buf_kernel(void *node_raw) {
  return ylc_audio_array_of_buf(node_raw);
}

__attribute__((always_inline)) int32_t ylc_audio_bufsize(void *node_raw) {
  _DoubleArray array = ylc_audio_array_of_buf(node_raw);
  return array.size;
}

int32_t bufsize(void *node_raw) { return ylc_audio_bufsize(node_raw); }

__attribute__((always_inline)) int32_t
ylc_audio_bufsize_kernel(void *node_raw) {
  return ylc_audio_bufsize(node_raw);
}

__attribute__((always_inline)) double
ylc_audio_lfnoise_kernel(LfNoiseState *state, double spf, double freq,
                         double lo, double hi) {
  double trig = ylc_audio_trig_kernel(&state->trig, spf, freq);

  if (!state->initialized) {
    state->value = audio_jit_rand_range(lo, hi);
    state->slope = 0.0;
    state->initialized = 1;
    return state->value;
  }

  if (trig >= 0.5) {
    double target = audio_jit_rand_range(lo, hi);
    state->slope = (target - state->value) * freq * spf;
  }

  double out = state->value;
  state->value = out + state->slope;
  return out;
}

__attribute__((always_inline)) double
ylc_audio_lfnoise0_kernel(LfNoise0State *state, double spf, double freq,
                          double lo, double hi) {
  double trig = ylc_audio_trig_kernel(&state->trig, spf, freq);
  if (!state->initialized || trig >= 0.5) {
    state->value = audio_jit_rand_range(lo, hi);
    state->initialized = 1;
  }
  return state->value;
}

__attribute__((always_inline)) double
ylc_audio_kill_on_end_kernel(KillOnEndState *state, double spf, void *node_raw,
                             double signal) {
  if (state->prev > EPSILON && signal <= EPSILON && node_raw) {
    ((Node *)node_raw)->trig_end = true;
  }
  state->prev = signal;
  return signal;
}

__attribute__((always_inline)) double
ylc_audio_adsr_kernel(AdsrState *state, double spf, double attack, double decay,
                      double sustain, double release, double trig) {
  double value = state->value;
  double phase = state->phase;
  double prev_trig = state->prev_trig;

  int rising = audio_jit_rising_edge(trig, prev_trig);
  int falling = prev_trig >= 0.5 && trig < 0.5;

  if (rising) {
    phase = 1.0;
  } else if (falling && phase == 3.0) {
    phase = 4.0;
  }

  if (phase == 1.0) {
    double rate = attack > 0.0 ? 1.0 / attack : 1e6;
    value += rate * spf;
    if (value >= 1.0) {
      value = 1.0;
      phase = 2.0;
    }
  } else if (phase == 2.0) {
    double rate = decay > 0.0 ? (1.0 - sustain) / decay : 1e6;
    value -= rate * spf;
    if (value <= sustain) {
      value = sustain;
      phase = trig >= 0.5 ? 3.0 : 4.0;
    }
  } else if (phase == 3.0) {
    value = sustain;
  } else if (phase == 4.0) {
    double rate = release > 0.0 ? 1.0 / release : 1e6;
    value -= rate * spf;
    if (value <= 0.0) {
      value = 0.0;
      phase = 0.0;
    }
  } else {
    value = 0.0;
  }

  state->value = value;
  state->phase = phase;
  state->prev_trig = trig;
  return value;
}

__attribute__((always_inline)) double ylc_audio_rect_kernel(RectState *state,
                                                            double spf,
                                                            double duration,
                                                            double trig) {
  if (audio_jit_rising_edge(trig, state->prev_trig)) {
    state->remaining = duration > 0.0 ? duration : 0.0;
  }

  double out = state->remaining > 0.0 ? 1.0 : 0.0;
  if (state->remaining > 0.0) {
    state->remaining -= spf;
    if (state->remaining < 0.0) {
      state->remaining = 0.0;
    }
  }

  state->prev_trig = trig;
  return out;
}
