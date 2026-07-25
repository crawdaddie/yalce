#include "./osc_kernels.h"
#include <math.h>
#include <stdint.h>

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE] = {
#include "../../engine/assets/sin_table.csv"
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
#include "../../engine/assets/sq_table.csv"
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
#include "../../engine/assets/saw_table.csv"
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
