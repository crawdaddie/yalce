#include "./osc_kernels.h"
#include "node.h"
#include "ylc_datatypes.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
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

static inline double audio_jit_clamp(double value, double lo, double hi) {
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

static inline double audio_jit_curve_interp(double t, double y0, double y1,
                                            double curve) {
  t = audio_jit_clamp(t, 0.0, 1.0);
  if (fabs(curve) < 0.001) {
    return audio_jit_lerp(y0, y1, t);
  }

  double sign = curve > 0.0 ? 1.0 : -1.0;
  double amount = fabs(curve) * 3.0;
  double k = exp(sign * amount);
  double denom = k - 1.0;
  if (fabs(denom) < 1e-12) {
    return audio_jit_lerp(y0, y1, t);
  }

  double curved_t = (exp(sign * amount * t) - 1.0) / denom;
  return audio_jit_lerp(y0, y1, curved_t);
}

static inline void audio_jit_array_env_set_segment(ArrayEnvState *state,
                                                   int32_t segment,
                                                   int32_t size, double *data) {
  int32_t base = segment * 3;
  state->phase = 0.0;
  state->current_segment = segment;
  state->start = data[base];
  state->time = data[base + 1];
  state->curve = data[base + 2];
  state->target = data[base + 3];
  (void)size;
}

static inline double audio_jit_array_env_current_value(ArrayEnvState *state) {
  double duration = state->time > 0.0 ? state->time : 0.0;
  double norm_phase = duration > 0.0 ? state->phase / duration : 1.0;
  return audio_jit_curve_interp(norm_phase, state->start, state->target,
                                state->curve);
}

static inline void
audio_jit_array_env_set_custom_segment(ArrayEnvState *state, int32_t segment,
                                       double start, double target, double time,
                                       double curve) {
  state->phase = 0.0;
  state->current_segment = segment;
  state->start = start;
  state->target = target;
  state->time = time;
  state->curve = curve;
}

static inline int32_t audio_jit_array_env_segment_count(int32_t size) {
  return size >= 4 ? (size - 1) / 3 : 0;
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

/* Estimate frequency from rising zero crossings of an already filtered signal.
 */
__attribute__((always_inline)) double
ylc_audio_zero_xing_freq_kernel(ZeroCrossingState *state, double spf,
                                double input) {
  if (!state || spf <= 0.0 || !isfinite(spf) || !isfinite(input)) {
    return 0.0;
  }

  state->samples++;
  if (state->previous <= 0.0 && input > 0.0) {
    int64_t period = state->samples - state->last_crossing;
    if (state->last_crossing > 0 && period > 0) {
      state->frequency = 1.0 / ((double)period * spf);
    }
    state->last_crossing = state->samples;
  }
  state->previous = input;
  return state->frequency;
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

__attribute__((always_inline)) double ylc_audio_sweep_kernel(SweepState *state,
                                                             double spf,
                                                             double rate,
                                                             double trig) {
  double value =
      audio_jit_rising_edge(trig, state->prev_trig) ? 0.0 : state->value;
  state->value = value + rate * spf;
  state->prev_trig = trig;
  return value;
}

__attribute__((always_inline)) double
ylc_audio_freq_shift_kernel(FreqShiftState *state, double spf, double freq,
                            double phase_offset, double input) {
  static const double hilbert_gamma[] = {
      0.3609, 2.7412, 11.1573, 44.7581, 179.6242, 798.4578,
      1.2524, 5.5671, 22.3423, 89.6271, 364.7914, 2770.1114,
  };

  if (!state->initialized) {
    const double gamma_scale = 15.0 * M_PI * spf;
    for (int i = 0; i < FREQ_SHIFT_HILBERT_STAGES; i++) {
      double gamma = gamma_scale * hilbert_gamma[i];
      state->coefs[i] = (gamma - 1.0) / (gamma + 1.0);
      state->y1[i] = 0.0;
    }
    state->phase = 0.0;
    state->initialized = 1;
  }

  double y = input;
  double cosine = 0.0;
  for (int i = 0; i < FREQ_SHIFT_HILBERT_STAGES / 2; i++) {
    double y0 = y - state->coefs[i] * state->y1[i];
    y = state->coefs[i] * y0 + state->y1[i];
    state->y1[i] = y0;
    cosine = y;
  }

  y = input;
  double sine = 0.0;
  for (int i = FREQ_SHIFT_HILBERT_STAGES / 2; i < FREQ_SHIFT_HILBERT_STAGES;
       i++) {
    double y0 = y - state->coefs[i] * state->y1[i];
    y = state->coefs[i] * y0 + state->y1[i];
    state->y1[i] = y0;
    sine = y;
  }

  double angle = state->phase + phase_offset;
  double output = cosine * cos(angle) + sine * sin(angle);
  state->phase += 2.0 * M_PI * freq * spf;
  state->phase = fmod(state->phase, 2.0 * M_PI);
  return output;
}

__attribute__((always_inline)) double
ylc_audio_rand_trig_kernel(RandTrigState *state, double spf, double lo,
                           double hi, double trig) {
  if (!state->initialized || audio_jit_rising_edge(trig, state->prev_trig)) {
    state->value = audio_jit_rand_range(lo, hi);
    state->initialized = 1;
  }

  state->prev_trig = trig;
  return state->value;
}

__attribute__((always_inline)) double
ylc_audio_fold2_kernel(void *state, double spf, double value, double bound) {
  (void)state;
  (void)spf;

  double lo = -bound;
  double hi = bound;
  if (hi == lo) {
    return lo;
  }

  double range = hi - lo;
  double range2 = range + range;
  double folded = value - lo;
  folded -= range2 * floor(folded / range2);
  if (folded >= range) {
    folded = range2 - folded;
  }
  return folded + lo;
}

__attribute__((always_inline)) double
ylc_audio_phasor_retrig_kernel(PhasorState *state, double spf, double freq,
                               double trig) {
  double phase = state->phase;
  state->phase = fmod(state->phase + freq * spf, 1.0);
  if (trig) {
    state->phase = 0.;
  }
  return phase;
}

__attribute__((always_inline)) double
ylc_audio_trig_once_kernel(TrigOnceState *state, double spf, double freq) {
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
ylc_audio_trig_retrig_kernel(TrigState *state, double spf, double freq,
                             double retrig) {
  double phase = state->phase;
  double step = freq * spf;
  double advanced = phase + step;
  double wrapped = step >= 0.0 ? advanced >= 1.0 : advanced < 0.0;
  double out = (!state->initialized || wrapped) ? 1.0 : 0.0;

  state->phase = advanced - floor(advanced);
  if (retrig) {
    state->phase = 0.;
    out = 1.;
  }
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

__attribute__((always_inline)) double
ylc_audio_change_up_kernel(ChangeUpState *state, double spf, double input) {
  (void)spf;
  double out = state->seen && state->prev == 0.0 && input > 0.0 ? 1.0 : 0.0;
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
                         int32_t offset, double *buf, double rate,
                         double start_pos, double trig) {

  (void)spf;
  int rising = audio_jit_rising_edge(trig, state->prev_trig);
  double phase = rising ? start_pos : state->phase;

  double sample = audio_jit_tabread_core(size, offset, buf, phase, 1);
  double next_phase = fmod(phase + rate / (double)size, 1.0);
  if (next_phase < 0.0) {
    next_phase += 1.0;
  }
  state->phase = next_phase;
  state->prev_trig = trig;

  return sample;
}

__attribute__((always_inline)) double
ylc_audio_mbufplay_kernel(BufplayState *state, double spf, int32_t channels,
                          int32_t channel, int32_t size, int32_t offset,
                          double *data, double rate, double start_pos,
                          double trig) {
  return 0.0;
}

static inline double audio_jit_clamp_feedback(double value) {
  if (!isfinite(value)) {
    return 0.0;
  }
  if (value > 0.999) {
    return 0.999;
  }
  if (value < -0.999) {
    return -0.999;
  }
  return value;
}

static inline int32_t audio_jit_wrap_delay_index(int32_t index, int32_t size) {
  index %= size;
  return index < 0 ? index + size : index;
}

static inline double audio_jit_delay_line_read(DelayLineState *state,
                                               int32_t max_samples,
                                               double delay_secs, double spf) {
  if (!state || max_samples <= 1 || spf <= 0.0 || !isfinite(spf)) {
    return 0.0;
  }

  if (state->size != max_samples) {
    state->size = max_samples;
    state->write_pos = 0;
  }
  if (state->write_pos < 0 || state->write_pos >= state->size) {
    state->write_pos = 0;
  }

  double delay_samples = isfinite(delay_secs) ? delay_secs / spf : 1.0;
  if (delay_samples < 1.0) {
    delay_samples = 1.0;
  }
  if (delay_samples >= (double)state->size) {
    delay_samples = (double)(state->size - 1);
  }

  int32_t delay_i = (int32_t)delay_samples;
  double frac = delay_samples - (double)delay_i;
  int32_t read0 =
      audio_jit_wrap_delay_index(state->write_pos - delay_i, state->size);
  int32_t read1 = audio_jit_wrap_delay_index(read0 - 1, state->size);

  return audio_jit_lerp(state->storage[read0], state->storage[read1], frac);
}

static inline void audio_jit_delay_line_write(DelayLineState *state,
                                              double value) {
  state->storage[state->write_pos] = value;
  state->write_pos++;
  if (state->write_pos >= state->size) {
    state->write_pos = 0;
  }
}

__attribute__((always_inline)) double
ylc_audio_comb_kernel(DelayLineState *state, double spf, int32_t max_samples,
                      double delay_secs, double feedback, double input) {
  if (!state || max_samples <= 1) {
    return input;
  }

  double delayed =
      audio_jit_delay_line_read(state, max_samples, delay_secs, spf);
  double fb = audio_jit_clamp_feedback(feedback);
  double out = input + (fb * delayed);
  audio_jit_delay_line_write(state, out);
  return out;
}

__attribute__((always_inline)) double
ylc_audio_dl_allpass_kernel(DelayLineState *state, double spf,
                            int32_t max_samples, double delay_secs,
                            double feedback, double input) {
  if (!state || max_samples <= 1) {
    return input;
  }

  double delayed =
      audio_jit_delay_line_read(state, max_samples, delay_secs, spf);
  double g = audio_jit_clamp_feedback(feedback);
  double out = delayed - (g * input);
  audio_jit_delay_line_write(state, input + (g * delayed));
  return out;
}

/* Four staggered, linearly interpolated read heads preserve time while
   changing their read slopes to shift pitch. */
__attribute__((always_inline)) double
ylc_audio_pitchshift_kernel(PitchShiftState *state, double spf,
                            int32_t max_samples, double window, double ratio,
                            double pitch_dispersion, double time_dispersion,
                            double input) {
  const double min_window = 3.0 * spf;
  if (!state || max_samples < 8 || spf <= 0.0 || !isfinite(spf)) {
    return input;
  }

  if (!isfinite(window) || window < min_window) {
    window = min_window;
  }

  int32_t framesize = (int32_t)(window / spf) + 2;
  framesize &= ~3;
  if (framesize < 4) {
    framesize = 4;
  }

  if (state->size == 0) {
    state->size = max_samples;
    state->mask = max_samples - 1;
    state->write_pos = 0;
    state->stage = 3;
    state->counter = framesize >> 2;
    state->framesize = framesize;
    state->slope = 2.0 / (double)framesize;
    state->ramp[0] = 0.5;
    state->ramp[1] = 1.0;
    state->ramp[2] = 0.5;
    state->ramp[3] = 0.0;
    state->ramp_slope[0] = -state->slope;
    state->ramp_slope[1] = -state->slope;
    state->ramp_slope[2] = state->slope;
    state->ramp_slope[3] = state->slope;
    state->read_pos[0] = state->read_pos[1] = state->read_pos[2] =
        state->read_pos[3] = 2.0;
    state->read_slope[0] = state->read_slope[1] = state->read_slope[2] =
        state->read_slope[3] = 1.0;
  }

  if (state->framesize != (double)framesize) {
    state->framesize = framesize;
    state->slope = 2.0 / (double)framesize;
  }

  if (state->counter <= 0) {
    double dispersed_ratio = isfinite(ratio) ? ratio : 1.0;
    if (isfinite(pitch_dispersion)) {
      dispersed_ratio += pitch_dispersion * audio_jit_rand_range(-1.0, 1.0);
    }
    dispersed_ratio = audio_jit_clamp(dispersed_ratio, 0.0, 4.0);
    double ratio_delta = dispersed_ratio - 1.0;
    double start =
        ratio_delta < 0.0 ? 2.0 : state->framesize * ratio_delta + 2.0;
    double dispersion =
        isfinite(time_dispersion)
            ? audio_jit_clamp(time_dispersion, 0.0, window) / spf
            : 0.0;
    start += dispersion * audio_jit_rand_range(0.0, 1.0);

    state->counter = framesize >> 2;
    state->stage = (state->stage + 1) & 3;
    int stage = state->stage;
    state->read_slope[stage] = -ratio_delta;
    state->read_pos[stage] = start;
    state->ramp[stage] = 0.0;
    state->ramp_slope[stage] = state->slope;
    state->ramp_slope[(stage + 2) & 3] = -state->slope;
  }

  state->counter--;
  state->write_pos = (state->write_pos + 1) & state->mask;
  double output = 0.0;
  for (int i = 0; i < 4; i++) {
    state->read_pos[i] += state->read_slope[i];
    int32_t sample = (int32_t)state->read_pos[i];
    double frac = state->read_pos[i] - (double)sample;
    int32_t read = (state->write_pos - sample) & state->mask;
    int32_t previous = (read - 1) & state->mask;
    output +=
        audio_jit_lerp(state->storage[read], state->storage[previous], frac) *
        state->ramp[i];
    state->ramp[i] += state->ramp_slope[i];
  }

  state->storage[state->write_pos] = input;
  return output * 0.5;
}

/* Fontana's digital Moog VCF: four cascaded one-pole sections inside a
   feedback loop, with coefficients recomputed when cutoff changes. */
__attribute__((always_inline)) double
ylc_audio_moogff_kernel(MoogFFState *state, double spf, double freq,
                        double gain, double reset, double input) {
  if (!state || spf <= 0.0 || !isfinite(spf)) {
    return input;
  }

  if (!isfinite(freq)) {
    freq = 0.0;
  }
  if (state->freq != freq) {
    double wc = 2.0 * tan(M_PI * freq * spf);
    if (!isfinite(wc) || wc < 0.0) {
      wc = 0.0;
    }
    state->b0 = wc / (wc + 2.0);
    state->a1 = (wc - 2.0) / (wc + 2.0);
    state->freq = freq;
  }

  if (reset > 0.0) {
    state->s1 = 0.0;
    state->s2 = 0.0;
    state->s3 = 0.0;
    state->s4 = 0.0;
  }

  double k = audio_jit_clamp(gain, 0.0, 4.0);
  double b0 = state->b0;
  double a1 = state->a1;
  double b0_2 = b0 * b0;
  double b0_4 = b0_2 * b0_2;
  double output = (b0_4 * input + state->s4 +
                   b0 * (state->s3 + b0 * (state->s2 + b0 * state->s1))) /
                  (1.0 + b0_4 * k);
  double u = input - k * output;

  double past = u;
  double future = b0 * past + state->s1;
  state->s1 = b0 * past - a1 * future;

  past = future;
  future = b0 * past + state->s2;
  state->s2 = b0 * past - a1 * future;

  past = future;
  future = b0 * past + state->s3;
  state->s3 = b0 * past - a1 * future;
  state->s4 = b0 * future - a1 * output;

  return isfinite(output) ? output : 0.0;
}

/* Match SuperCollider's Hasher: hash the 32-bit float representation, then
   reinterpret the result as a float in [2, 3) and shift it to [-1, 0). */
__attribute__((always_inline)) double
ylc_audio_hasher_kernel(void *unused, double spf, double input) {

  union {
    float f;
    uint32_t u;
  } value;
  value.f = (float)input;

  uint32_t hash = value.u;
  hash += ~(hash << 15);
  hash ^= hash >> 10;
  hash += hash << 3;
  hash ^= hash >> 6;
  hash += ~(hash << 11);
  hash ^= hash >> 16;

  value.u = UINT32_C(0x40000000) | (hash >> 9);
  return (double)value.f - 3.0;
}

__attribute__((always_inline)) double ylc_audio_lag_kernel(LagState *state,
                                                           double spf,
                                                           double lag_secs,
                                                           double input) {
  if (!state || !isfinite(input)) {
    return 0.0;
  }

  if (lag_secs < 0.0 || !isfinite(lag_secs)) {
    lag_secs = 0.0;
  }

  if (state->initialized == 0.0) {
    state->initialized = 1.0;
    state->y1 = input;
    state->b1 = 0.0;
    state->lag_secs = -1.0;
  }

  if (lag_secs != state->lag_secs) {
    const double log001 = -6.907755278982137;
    state->b1 = (lag_secs == 0.0 || spf <= 0.0 || !isfinite(spf))
                    ? 0.0
                    : exp(log001 * spf / lag_secs);
    state->lag_secs = lag_secs;
  }

  double y1 = input + state->b1 * (state->y1 - input);
  if (!isfinite(y1)) {
    y1 = input;
  }
  state->y1 = y1;
  return y1;
}

static inline int32_t audio_jit_array_random_index(int32_t size) {
  return size > 0 ? rand() % size : 0;
}

__attribute__((always_inline)) double
ylc_audio_arr_choose_kernel(ArrayChooseState *state, double spf, int32_t size,
                            int32_t offset, double *raw_data, double trig) {
  (void)spf;
  double *data = audio_jit_array_data(offset, raw_data);
  if (!state || !data || size <= 0) {
    return 0.0;
  }

  if (state->initialized == 0.0) {
    state->initialized = 1;
    state->value = data[audio_jit_array_random_index(size)];
    state->prev_trig = trig;
    return state->value;
  }

  if (audio_jit_rising_edge(trig, state->prev_trig)) {
    state->value = data[audio_jit_array_random_index(size)];
  }
  state->prev_trig = trig;
  return state->value;
}

__attribute__((always_inline)) double
ylc_audio_arr_seq_kernel(ArraySeqState *state, double spf, int32_t size,
                         int32_t offset, double *raw_data, double trig) {
  (void)spf;
  double *data = audio_jit_array_data(offset, raw_data);
  if (!state || !data || size <= 0) {
    return 0.0;
  }

  if (state->initialized == 0.0) {
    state->initialized = 1;
    state->value = data[0];
    state->counter = -1;
    state->prev_trig = trig;
    return state->value;
  }

  if (audio_jit_rising_edge(trig, state->prev_trig)) {
    int32_t next = state->counter + 1;
    if (next < 0 || next >= size) {
      next = 0;
    }
    state->counter = next;
    state->value = data[next];
  }
  state->prev_trig = trig;
  return state->value;
}

#define GRAIN_WINDOW_TABSIZE (1 << 9)

static double grain_win[GRAIN_WINDOW_TABSIZE] = {
#include "./grain_win.csv"
};

typedef struct GrainStateArrays {
  double *rates;
  double *phases;
  double *widths;
  double *remaining_secs;
  double *starts;
  int32_t *active;
} GrainStateArrays;

static inline GrainStateArrays
audio_jit_grain_state_arrays(GrainOscState *state, int32_t max_grains) {
  char *mem = (char *)state->storage;
  GrainStateArrays arrays = {0};

  arrays.rates = (double *)mem;
  mem += sizeof(double) * (size_t)max_grains;
  arrays.phases = (double *)mem;
  mem += sizeof(double) * (size_t)max_grains;
  arrays.widths = (double *)mem;
  mem += sizeof(double) * (size_t)max_grains;
  arrays.remaining_secs = (double *)mem;
  mem += sizeof(double) * (size_t)max_grains;
  arrays.starts = (double *)mem;
  mem += sizeof(double) * (size_t)max_grains;
  arrays.active = (int32_t *)mem;

  return arrays;
}

static inline double audio_jit_read_linear(double *data, int32_t size,
                                           double sample_index) {
  if (!data || size <= 0 || !isfinite(sample_index)) {
    return 0.0;
  }

  double len_f = (double)size;
  double wrapped = audio_jit_wrap_index(sample_index, len_f);
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
  if (i1 >= size) {
    i1 = 0;
  }

  return audio_jit_lerp(data[i0], data[i1], frac);
}

static inline double audio_jit_table_read_clamped(double pos, int32_t tabsize,
                                                  double *table) {
  if (!table || tabsize <= 0 || !isfinite(pos)) {
    return 0.0;
  }
  if (tabsize == 1) {
    return table[0];
  }

  double clamped = pos;
  if (clamped < 0.0) {
    clamped = 0.0;
  } else if (clamped > 1.0) {
    clamped = 1.0;
  }

  double table_pos = clamped * (double)(tabsize - 1);
  int32_t i0 = (int32_t)table_pos;
  if (i0 >= tabsize - 1) {
    return table[tabsize - 1];
  }
  double frac = table_pos - (double)i0;
  return audio_jit_lerp(table[i0], table[i0 + 1], frac);
}

static double ylc_audio_pow2table_read(double pos, int tabsize, double *table) {
  if (!table || tabsize <= 0 || !isfinite(pos)) {
    return 0.0;
  }
  int mask = tabsize - 1;

  double env_pos = pos * (mask);
  int env_idx = (int)env_pos;
  double env_frac = env_pos - env_idx;

  // Interpolate between envelope table values
  double env_val = table[env_idx & mask] * (1.0 - env_frac) +
                   table[(env_idx + 1) & mask] * env_frac;
  return env_val;
}
__attribute__((always_inline)) double
ylc_audio_grains_kernel(GrainOscState *state, double spf, int32_t max_grains,
                        int32_t size, int32_t offset, double *data, double rate,
                        double position, double width, double trig) {
  if (!state || max_grains <= 0 || size <= 0 || !data) {
    return 0.0;
  }

  double *buf = data;
  if (!buf) {
    return 0.0;
  }

  GrainStateArrays arrays = audio_jit_grain_state_arrays(state, max_grains);
  double sample = 0.0;
  int rising = audio_jit_rising_edge(trig, state->prev_trig);
  int can_spawn = rising && state->active_grains < max_grains && width > 0.0;

  state->max_grains = max_grains;

  if (can_spawn) {
    for (int32_t i = 0; i < max_grains; i++) {
      if (arrays.active[i] == 0) {
        arrays.rates[i] = rate;
        arrays.phases[i] = 0.0;
        arrays.starts[i] = position * (double)size;
        arrays.widths[i] = width;
        arrays.remaining_secs[i] = width;
        arrays.active[i] = 1;
        state->active_grains++;
        break;
      }
    }
  }

  for (int32_t i = 0; i < max_grains; i++) {
    if (!arrays.active[i]) {
      continue;
    }

    double r = arrays.rates[i];
    double p = arrays.phases[i];
    double s = arrays.starts[i];
    double w = arrays.widths[i];
    double rem = arrays.remaining_secs[i];
    if (w <= 0.0 || rem <= 0.0 || !isfinite(w) || !isfinite(rem)) {
      arrays.active[i] = 0;
      if (state->active_grains > 0) {
        state->active_grains--;
      }
      continue;
    }

    double d_index = s + (p * (double)size);
    double grain_elapsed = 1.0 - (rem / w);
    double env_val = ylc_audio_pow2table_read(grain_elapsed,
                                              GRAIN_WINDOW_TABSIZE, grain_win);

    sample += env_val * audio_jit_read_linear(buf, size, d_index);
    arrays.phases[i] += r / (double)size;

    arrays.remaining_secs[i] -= spf;
    if (arrays.remaining_secs[i] <= 0.0) {
      arrays.active[i] = 0;
      if (state->active_grains > 0) {
        state->active_grains--;
      }
    }
  }

  state->prev_trig = trig;
  return sample;
}

__attribute__((always_inline)) double ylc_audio_grains_env_kernel(
    void *state_raw, double spf, int32_t max_grains, int32_t source_size,
    int32_t source_offset, double *source_data, int32_t envelope_size,
    int32_t envelope_offset, double *envelope_data, double rate,
    double position, double width, double trig) {
  GrainOscState *state = (GrainOscState *)state_raw;
  if (!state || max_grains <= 0 || source_size <= 0 || envelope_size <= 0 ||
      !source_data || !envelope_data) {
    return 0.0;
  }

  double *source = source_data;
  double *envelope = envelope_data;
  if (!source || !envelope) {
    return 0.0;
  }

  GrainStateArrays arrays = audio_jit_grain_state_arrays(state, max_grains);
  double sample = 0.0;
  int rising = audio_jit_rising_edge(trig, state->prev_trig);
  int can_spawn = rising && state->active_grains < max_grains && width > 0.0 &&
                  isfinite(width) && isfinite(rate) && isfinite(position);

  state->max_grains = max_grains;

  if (can_spawn) {
    for (int32_t i = 0; i < max_grains; i++) {
      if (arrays.active[i] == 0) {
        arrays.rates[i] = rate;
        arrays.phases[i] = 0.0;
        arrays.starts[i] = position * (double)source_size;
        arrays.widths[i] = width;
        arrays.remaining_secs[i] = width;
        arrays.active[i] = 1;
        state->active_grains++;
        break;
      }
    }
  }

  for (int32_t i = 0; i < max_grains; i++) {
    if (!arrays.active[i]) {
      continue;
    }

    double r = arrays.rates[i];
    double p = arrays.phases[i];
    double s = arrays.starts[i];
    double w = arrays.widths[i];
    double rem = arrays.remaining_secs[i];
    if (w <= 0.0 || rem <= 0.0 || !isfinite(w) || !isfinite(rem)) {
      arrays.active[i] = 0;
      if (state->active_grains > 0) {
        state->active_grains--;
      }
      continue;
    }

    double d_index = s + (p * (double)source_size);
    double grain_elapsed = 1.0 - (rem / w);
    double env_val =
        audio_jit_table_read_clamped(grain_elapsed, envelope_size, envelope);

    sample += env_val * audio_jit_read_linear(source, source_size, d_index);
    arrays.phases[i] += r / (double)source_size;

    arrays.remaining_secs[i] -= spf;
    if (arrays.remaining_secs[i] <= 0.0) {
      arrays.active[i] = 0;
      if (state->active_grains > 0) {
        state->active_grains--;
      }
    }
  }

  state->prev_trig = trig;
  return sample;
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
  return lo + value * (hi - lo);
}

__attribute__((always_inline)) double
ylc_audio_scale_bp_kernel(void *state, double spf, double lo, double hi,
                          double value) {
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

__attribute__((always_inline)) double
ylc_audio_adsr_array_kernel(AdsrState *state, double spf, int32_t size,
                            int32_t offset, double *data, double trig) {
  double *values = audio_jit_array_data(offset, data);
  if (!values || size < 4) {
    return ylc_audio_adsr_kernel(state, spf, 0.0, 0.0, 0.0, 0.0, trig);
  }

  return ylc_audio_adsr_kernel(state, spf, values[0], values[1], values[2],
                               values[3], trig);
}

__attribute__((always_inline)) double
ylc_audio_array_env_kernel(ArrayEnvState *state, double spf, int32_t size,
                           int32_t offset, double *data, double time_scale,
                           double trig) {
  double *values = audio_jit_array_data(offset, data);
  int32_t segments = audio_jit_array_env_segment_count(size);
  if (!state || !values || segments <= 0 || ((size - 1) % 3) != 0) {
    if (state) {
      state->prev_trig = trig;
      state->active = 0;
    }
    return 0.0;
  }

  if (audio_jit_rising_edge(trig, state->prev_trig)) {
    state->active = 1;
    audio_jit_array_env_set_segment(state, 0, size, values);
  }

  double out = 0.0;
  if (state->active) {
    double duration = state->time > 0.0 ? state->time : 0.0;
    double norm_phase = duration > 0.0 ? state->phase / duration : 1.0;
    out = audio_jit_curve_interp(norm_phase, state->start, state->target,
                                 state->curve);

    double scale = time_scale > 0.0 ? time_scale : 1.0;
    state->phase += spf / scale;

    while (state->active && state->phase >= duration) {
      state->current_segment++;
      if (state->current_segment >= segments) {
        state->active = 0;
        out = state->target;
        break;
      }
      audio_jit_array_env_set_segment(state, state->current_segment, size,
                                      values);
      duration = state->time > 0.0 ? state->time : 0.0;
      if (duration > 0.0) {
        break;
      }
    }
  }

  state->prev_trig = trig;
  return out;
}

__attribute__((always_inline)) double
ylc_audio_gated_array_env_kernel(ArrayEnvState *state, double spf, int32_t size,
                                 int32_t offset, double *data, double gate) {
  double *values = audio_jit_array_data(offset, data);
  int32_t segments = audio_jit_array_env_segment_count(size);
  if (!state || !values || segments <= 0 || ((size - 1) % 3) != 0) {
    if (state) {
      state->prev_trig = gate;
      state->active = 0;
      state->sustaining = 0;
    }
    return 0.0;
  }

  int rising = audio_jit_rising_edge(gate, state->prev_trig);
  int falling = state->prev_trig >= 0.5 && gate < 0.5;
  int release_segment = segments - 1;
  int sustain_point = segments - 1;

  if (rising) {
    state->active = 1;
    state->sustaining = 0;
    audio_jit_array_env_set_segment(state, 0, size, values);
  } else if (falling && state->active && state->current_segment < segments) {
    double current = state->sustaining
                         ? values[sustain_point * 3]
                         : audio_jit_array_env_current_value(state);
    audio_jit_array_env_set_custom_segment(
        state, release_segment, current, values[segments * 3],
        values[release_segment * 3 + 1], values[release_segment * 3 + 2]);
    state->sustaining = 0;
  }

  double out = 0.0;
  if (state->active) {
    if (state->sustaining) {
      out = values[sustain_point * 3];
    } else {
      out = audio_jit_array_env_current_value(state);
      state->phase += spf;

      while (state->active && !state->sustaining &&
             state->phase >= (state->time > 0.0 ? state->time : 0.0)) {
        state->current_segment++;
        if (state->current_segment >= segments) {
          state->active = 0;
          out = state->target;
          break;
        }

        if (state->current_segment == release_segment && gate >= 0.5 &&
            segments > 1) {
          state->sustaining = 1;
          out = values[sustain_point * 3];
          break;
        }

        audio_jit_array_env_set_segment(state, state->current_segment, size,
                                        values);
        if (state->time > 0.0) {
          break;
        }
      }
    }
  }

  state->prev_trig = gate;
  return out;
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

__attribute__((always_inline)) double
ylc_audio_sah_kernel(SahState *state, double spf, double sig, double trig) {
  (void)spf;

  if (audio_jit_rising_edge(trig, state->prev_trig)) {
    state->value = sig;
  }

  if (!state->initialized) {
    state->value = sig;
  }

  state->prev_trig = trig;
  state->initialized = 1;
  return state->value;
}

/* disperser: a cascade of identical 2nd-order allpass sections, all tuned to a
   single cutoff frequency, modeled on the Kilohearts Disperser ("a stack of
   all-pass filters tuned to cause frequency dependent delay").

   Per-sample args:
     freq   - cutoff / center frequency in Hz (clamped to [1, Nyquist])
     amount - normalized [0,1] controlling the allpass order: the number of
              active stages is n = round(amount * DISPERSER_MAX_STAGES).
              amount <= 0 bypasses the input unchanged.
     pinch  - normalized [0,1] controlling the Q of every section. The mapping
              is inverted so that higher pinch is more "chirpy" (low Q, broad
              delay); lower pinch concentrates the group delay around the
              cutoff. Mapped to Q = 0.5 * 100^(1-pinch) (0.5 .. 50).
     input  - signal sample.

   All sections share one set of coefficients, recomputed per sample so freq
   and pinch may be audio-rate. Each section is the RBJ 2nd-order allpass:
       y = a2*x + a1*x1 + x2 - a1*y1 - a2*y2
   with a1 = -2*cos(w0)/(1+alpha), a2 = (1-alpha)/(1+alpha),
   alpha = sin(w0)/(2Q), w0 = 2*pi*freq*spf. Cascading N identical sections
   multiplies the single-section group delay (which peaks at the cutoff),
   concentrating delay there and smearing transients across time without
   changing gain. */
__attribute__((always_inline)) double
ylc_audio_disperser_kernel(DisperserState *state, double spf, double freq,
                           double amount, double pinch, double input) {
  if (!state || spf <= 0.0 || !isfinite(spf)) {
    return input;
  }

  double a = amount;
  if (!isfinite(a) || a < 0.0) {
    a = 0.0;
  } else if (a > 1.0) {
    a = 1.0;
  }
  int n = (int)(a * (double)DISPERSER_MAX_STAGES + 0.5);
  if (n <= 0) {
    return input;
  }
  if (n > DISPERSER_MAX_STAGES) {
    n = DISPERSER_MAX_STAGES;
  }

  double f = freq;
  if (!isfinite(f) || f < 1.0) {
    f = 1.0;
  }
  double nyq = 0.5 / spf;
  if (f > nyq) {
    f = nyq;
  }

  double p = pinch;
  if (!isfinite(p) || p < 0.0) {
    p = 0.0;
  } else if (p > 1.0) {
    p = 1.0;
  }
  double q = 0.5 * pow(100.0, 1.0 - p);

  double w0 = 2.0 * M_PI * f * spf;
  if (w0 > M_PI) {
    w0 = M_PI;
  }
  double cosw = cos(w0);
  double sinw = sin(w0);
  double alpha = sinw / (2.0 * q);
  double a0 = 1.0 + alpha;
  double a1 = -2.0 * cosw / a0;
  double a2 = (1.0 - alpha) / a0;

  double x = input;
  for (int i = 0; i < n; i++) {
    double x1 = state->x1[i];
    double x2 = state->x2[i];
    double y1 = state->y1[i];
    double y2 = state->y2[i];
    double y = a2 * x + a1 * x1 + x2 - a1 * y1 - a2 * y2;
    state->x2[i] = x1;
    state->x1[i] = x;
    state->y2[i] = y1;
    state->y1[i] = y;
    x = y;
  }
  return x;
}

/* glue compressor: an SSL-bus-style "glue" compressor.

   Detection is RMS (one-pole, ~10 ms integration) so it follows the program
   material rather than single peaks, and the gain computer is a classic
   soft-knee curve in the dB domain.

   The glue character comes from two parallel attack/release ballistics
   applied to the gain reduction: a fast pair (attack/4, release/4) and a
   slow pair (attack*4, release*4), combined with max(). A brief transient is
   caught by the fast path, which recovers quickly; sustained material
   engages the slow path, which releases gradually -- the program-dependent
   "breathing" of a bus compressor.

   Per-sample args (all may be audio-rate / multi-lane):
     thresh  - threshold in dB
     ratio   - compression ratio (1 = no compression)
     attack  - attack time in seconds
     release - release time in seconds
     knee    - soft-knee width in dB
     makeup  - output gain in dB
     input   - signal sample. */
__attribute__((always_inline)) double
ylc_audio_glue_kernel(GlueCompState *state, double spf, double thresh,
                      double ratio, double attack, double release, double knee,
                      double makeup, double input) {
  if (!state || spf <= 0.0 || !isfinite(spf)) {
    return input;
  }

  double x = isfinite(input) ? input : 0.0;

  /* --- RMS detector (one-pole in the squared domain) --- */
  const double det_tc = 0.010; /* 10 ms detector integration */
  double det_coef = 1.0 - exp(-spf / det_tc);
  state->rms += (x * x - state->rms) * det_coef;
  if (!isfinite(state->rms) || state->rms < 1e-24) {
    state->rms = 1e-24;
  }
  double det_db = 10.0 * log10(state->rms);

  /* --- soft-knee gain computer (dB domain) --- */
  double r = ratio;
  if (!isfinite(r) || r < 1.0) {
    r = 1.0;
  }
  double slope = 1.0 - (1.0 / r);
  double kw = knee;
  if (!isfinite(kw) || kw < 0.0) {
    kw = 0.0;
  }
  double over = det_db - thresh;
  double gr = 0.0; /* target gain reduction in dB, <= 0 */
  if (over <= -0.5 * kw) {
    gr = 0.0;
  } else if (over >= 0.5 * kw) {
    gr = -(slope * over);
  } else {
    double t = over + 0.5 * kw;
    gr = -(slope * t * t / (2.0 * kw));
  }

  /* --- dual ballistics: fast and slow envelopes, combined with max --- */
  double atk = isfinite(attack) && attack > 0.0 ? attack : 0.0;
  double rel = isfinite(release) && release > 0.0 ? release : 0.0;
  double a_fast = atk > 0.0 ? 1.0 - exp(-spf / (atk * 0.25)) : 1.0;
  double r_fast = rel > 0.0 ? 1.0 - exp(-spf / (rel * 0.25)) : 1.0;
  double a_slow = atk > 0.0 ? 1.0 - exp(-spf / (atk * 4.0)) : 1.0;
  double r_slow = rel > 0.0 ? 1.0 - exp(-spf / (rel * 4.0)) : 1.0;

  state->env_a += (gr - state->env_a) * ((gr > state->env_a) ? a_fast : r_fast);
  state->env_b += (gr - state->env_b) * ((gr > state->env_b) ? a_slow : r_slow);
  if (!isfinite(state->env_a)) {
    state->env_a = 0.0;
  }
  if (!isfinite(state->env_b)) {
    state->env_b = 0.0;
  }

  double gr_env = state->env_a > state->env_b ? state->env_a : state->env_b;

  /* --- apply gain reduction + makeup --- */
  double gain = pow(10.0, (gr_env + makeup) * 0.05);
  if (!isfinite(gain)) {
    gain = 1.0;
  }
  return x * gain;
}

/* Look-ahead limiter modeled on SuperCollider's three-buffer limiter. */
__attribute__((always_inline)) double
ylc_audio_limiter_kernel(LimiterState *state, double spf, int32_t max_samples,
                         double level, double input) {
  if (!state || spf <= 0.0 || max_samples <= 0 || !isfinite(input) ||
      !isfinite(level) || level < 0.0) {
    return input;
  }

  if (state->size != max_samples) {
    state->size = max_samples;
    state->pos = 0;
    state->flips = 0;
    state->slope = 0.0;
    state->level = 1.0;
    state->current_max = 0.0;
    state->previous_max = 0.0;
    for (int32_t i = 0; i < max_samples * 3; i++) {
      state->storage[i] = 0.0;
    }
  }

  int32_t pos = state->pos;
  int32_t input_segment = (2 * state->flips) % 3;
  int32_t output_segment = (2 * state->flips + 2) % 3;
  double *input_buffer = state->storage + input_segment * max_samples;
  double *output_buffer = state->storage + output_segment * max_samples;
  double value = fabs(input);
  input_buffer[pos] = input;

  double output = state->flips >= 2 ? state->level * output_buffer[pos] : 0.0;
  state->level += state->slope;
  if (value > state->current_max) {
    state->current_max = value;
  }

  pos++;
  if (pos >= max_samples) {
    pos = 0;
    double max_value = state->previous_max > state->current_max
                           ? state->previous_max
                           : state->current_max;
    state->previous_max = state->current_max;
    state->current_max = 0.0;

    double next_level = max_value > level ? level / max_value : 1.0;
    state->slope = (next_level - state->level) / (double)max_samples;
    state->flips++;
  }

  state->pos = pos;
  return output;
}

/* pan: distribute a mono signal across N output channels with equal-power
   panning.

   out   - output buffer of N doubles (one per output channel, written here)
   n     - output channel count (compile-time constant at the call site)
   pos   - pan position in [0,1]: 0 = hard first channel, 1 = hard last channel
   signal - the mono (1-lane) input sample

   The position maps to a continuous channel index p = pos*(n-1). The two
   channels bracketing p receive equal-power gains derived from an angle
   theta = p * (pi/2): the lower channel gets cos(theta_local), the upper gets
   sin(theta_local), where theta_local is the fractional part of p scaled to
   [0, pi/2]. All other channels are silent. This generalises stereo
   equal-power panning to any N. */
__attribute__((always_inline)) void
ylc_audio_pan_kernel(double *out, int n, double pos, double signal) {
  if (!out || n <= 0) {
    return;
  }
  if (n == 1) {
    out[0] = signal;
    return;
  }

  double p = pos * (double)(n - 1);
  if (p < 0.0) {
    p = 0.0;
  } else if (p > (double)(n - 1)) {
    p = (double)(n - 1);
  }

  int lo = (int)p;
  if (lo < 0) {
    lo = 0;
  }
  if (lo > n - 2) {
    lo = n - 2;
  }
  double frac = p - (double)lo;

  /* equal-power crossfade between the two bracketing channels */
  double theta = frac * (M_PI / 2.0);
  double gain_lo = cos(theta);
  double gain_hi = sin(theta);

  for (int c = 0; c < n; c++) {
    out[c] = 0.0;
  }
  out[lo] = signal * gain_lo;
  out[lo + 1] = signal * gain_hi;
}
