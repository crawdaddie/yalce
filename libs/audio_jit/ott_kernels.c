#include "./ott_kernels.h"
#include <math.h>

#define OTT_BAND_COUNT 3

static inline double ott_clamp(double value, double lo, double hi) {
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

static inline double audio_jit_ott_filter(double input, double spf,
                                          double frequency, int high, int stage,
                                          double *x1, double *x2, double *y1,
                                          double *y2) {
  const double sqrt_two = 1.4142135623730951;
  double warp = 1.0 / tan(M_PI * frequency * spf);
  double warp_squared = warp * warp;
  double mult = 1.0 / (1.0 + sqrt_two * warp + warp_squared);
  double b0 = high ? warp_squared * mult : mult;
  double b1 = high ? -2.0 * b0 : 2.0 * mult;
  double b2 = b0;
  double a1 = 2.0 * (1.0 - warp_squared) * mult;
  double a2 = (1.0 - sqrt_two * warp + warp_squared) * mult;
  double output = b0 * input + b1 * *x1 + b2 * *x2 - a1 * *y1 - a2 * *y2;

  *x2 = *x1;
  *x1 = input;
  *y2 = *y1;
  *y1 = output;
  (void)stage;
  return output;
}

static inline double audio_jit_ott_band(double sample, double spf, int band,
                                        double depth, double upward,
                                        double downward, double attack,
                                        double release, OttState *state) {
  static const double lower_thresholds[OTT_BAND_COUNT] = {-35.0, -36.0, -35.0};
  static const double upper_thresholds[OTT_BAND_COUNT] = {-28.0, -25.0, -30.0};
  static const double lower_ratios[OTT_BAND_COUNT] = {0.8, 0.8, 0.8};
  static const double upper_ratios[OTT_BAND_COUNT] = {0.9, 0.857, 1.0};
  static const double attack_ms[OTT_BAND_COUNT] = {2.8, 1.4, 0.7};
  static const double release_ms[OTT_BAND_COUNT] = {40.0, 28.0, 15.0};
  static const double output_gains[OTT_BAND_COUNT] = {16.3, 11.7, 16.3};
  const double min_envelope_samples = 5.0;
  const double milliseconds_per_second = 1000.0;
  const double min_threshold_db = -100.0;
  const double max_threshold_db = 12.0;
  const double min_envelope_power = 1e-24;
  const double max_compressor_gain = 32.0;

  double lower_db =
      ott_clamp(lower_thresholds[band], min_threshold_db, max_threshold_db);
  double upper_db =
      ott_clamp(upper_thresholds[band], min_threshold_db, max_threshold_db);
  double lower_threshold = pow(10.0, lower_db * 0.1);
  double upper_threshold = pow(10.0, upper_db * 0.1);
  double attack_samples = attack_ms[band] * spf * milliseconds_per_second *
                          exp(ott_clamp(attack, 0.0, 1.0) * 8.0 - 4.0);
  double release_samples = release_ms[band] * spf * milliseconds_per_second *
                           exp(ott_clamp(release, 0.0, 1.0) * 8.0 - 4.0);
  if (attack_samples < min_envelope_samples) {
    attack_samples = min_envelope_samples;
  }
  if (release_samples < min_envelope_samples) {
    release_samples = min_envelope_samples;
  }

  double squared = sample * sample;
  double up_samples =
      squared > state->up_env[band] ? attack_samples : release_samples;
  double down_samples =
      squared > state->down_env[band] ? attack_samples : release_samples;
  state->up_env[band] =
      (squared + state->up_env[band] * up_samples) / (up_samples + 1.0);
  state->down_env[band] =
      (squared + state->down_env[band] * down_samples) / (down_samples + 1.0);
  if (state->up_env[band] > lower_threshold) {
    state->up_env[band] = lower_threshold;
  }
  if (state->down_env[band] < upper_threshold) {
    state->down_env[band] = upper_threshold;
  }
  if (state->up_env[band] < min_envelope_power) {
    state->up_env[band] = min_envelope_power;
  }
  if (state->down_env[band] < min_envelope_power) {
    state->down_env[band] = min_envelope_power;
  }

  double up_ratio = lower_ratios[band] * ott_clamp(depth, 0.0, 1.0) *
                    ott_clamp(upward, 0.0, 2.0) * 0.5;
  double down_ratio = upper_ratios[band] * ott_clamp(depth, 0.0, 1.0) *
                      ott_clamp(downward, 0.0, 2.0) * 0.5;
  double gain = pow(lower_threshold / state->up_env[band], up_ratio) *
                pow(upper_threshold / state->down_env[band], down_ratio);
  if (!isfinite(gain) || gain > max_compressor_gain) {
    gain = max_compressor_gain;
  }
  gain *= pow(10.0, output_gains[band] * 0.05);
  return sample * gain;
}

/* Three-band OTT based on Vital's multiband compressor. The fixed crossover
   frequencies and band defaults mirror vitOTTx; controls scale its depth and
   envelope settings without carrying any UI or plugin framework code. */
__attribute__((always_inline)) double
audio_jit_ott_process(OttState *state, double spf, double depth,
                      double upward, double downward, double attack,
                      double release, double low_cross_frequency,
                      double high_cross_frequency, double input) {
  if (!state || spf <= 0.0 || !isfinite(spf) ||
      !isfinite(low_cross_frequency) || !isfinite(high_cross_frequency) ||
      low_cross_frequency <= 0.0 || high_cross_frequency <= low_cross_frequency ||
      high_cross_frequency >= 0.5 / spf) {
    return input;
  }
  if (state->spf != spf || state->low_frequency != low_cross_frequency ||
      state->high_frequency != high_cross_frequency) {
    *state = (OttState){0};
    state->spf = spf;
    state->low_frequency = low_cross_frequency;
    state->high_frequency = high_cross_frequency;
  }

  double x = isfinite(input) ? input : 0.0;
  double low = audio_jit_ott_filter(x, spf, low_cross_frequency, 0, 0,
                                    &state->low_x1[0], &state->low_x2[0],
                                    &state->low_y1[0], &state->low_y2[0]);
  low = audio_jit_ott_filter(low, spf, low_cross_frequency, 0, 1,
                             &state->low_x1[1], &state->low_x2[1],
                             &state->low_y1[1], &state->low_y2[1]);

  double rest = audio_jit_ott_filter(x, spf, low_cross_frequency, 1, 0,
                                     &state->high_x1[0], &state->high_x2[0],
                                     &state->high_y1[0], &state->high_y2[0]);
  rest = audio_jit_ott_filter(rest, spf, low_cross_frequency, 1, 1,
                              &state->high_x1[1], &state->high_x2[1],
                              &state->high_y1[1], &state->high_y2[1]);

  double mid = audio_jit_ott_filter(rest, spf, high_cross_frequency, 0, 0,
                                    &state->low_x1[2], &state->low_x2[2],
                                    &state->low_y1[2], &state->low_y2[2]);
  mid = audio_jit_ott_filter(mid, spf, high_cross_frequency, 0, 1,
                             &state->low_x1[3], &state->low_x2[3],
                             &state->low_y1[3], &state->low_y2[3]);

  double high = audio_jit_ott_filter(rest, spf, high_cross_frequency, 1, 0,
                                     &state->high_x1[2], &state->high_x2[2],
                                     &state->high_y1[2], &state->high_y2[2]);
  high = audio_jit_ott_filter(high, spf, high_cross_frequency, 1, 1,
                              &state->high_x1[3], &state->high_x2[3],
                              &state->high_y1[3], &state->high_y2[3]);

  return audio_jit_ott_band(low, spf, 0, depth, upward, downward, attack,
                            release, state) +
         audio_jit_ott_band(mid, spf, 1, depth, upward, downward, attack,
                            release, state) +
         audio_jit_ott_band(high, spf, 2, depth, upward, downward, attack,
                            release, state);
}

__attribute__((always_inline)) double
ylc_audio_ott_kernel(OttState *state, double spf, double depth, double upward,
                     double downward, double attack, double release,
                     double input) {
  return audio_jit_ott_process(state, spf, depth, upward, downward, attack,
                               release, 120.0, 2500.0, input);
}

__attribute__((always_inline)) double
ylc_audio_ott_freq_kernel(OttState *state, double spf, double depth,
                          double upward, double downward, double attack,
                          double release, double low_frequency,
                          double high_frequency, double input) {
  return audio_jit_ott_process(state, spf, depth, upward, downward, attack,
                               release, low_frequency, high_frequency, input);
}

static inline double audio_jit_lr_band(double sample, double spf, int band,
                                       double thresh, double ratio,
                                       double attack, double release,
                                       OttState *state) {
  static const double attack_ms[OTT_BAND_COUNT] = {2.8, 1.4, 0.7};
  static const double release_ms[OTT_BAND_COUNT] = {40.0, 28.0, 15.0};
  const double min_envelope_samples = 5.0;
  const double min_envelope_power = 1e-24;
  double threshold = pow(10.0, ott_clamp(thresh, -100.0, 12.0) * 0.1);
  double attack_samples = attack_ms[band] * spf * 1000.0 *
                          exp(ott_clamp(attack, 0.0, 1.0) * 8.0 - 4.0);
  double release_samples = release_ms[band] * spf * 1000.0 *
                           exp(ott_clamp(release, 0.0, 1.0) * 8.0 - 4.0);
  if (attack_samples < min_envelope_samples) {
    attack_samples = min_envelope_samples;
  }
  if (release_samples < min_envelope_samples) {
    release_samples = min_envelope_samples;
  }

  double squared = sample * sample;
  double samples =
      squared > state->down_env[band] ? attack_samples : release_samples;
  state->down_env[band] =
      (squared + state->down_env[band] * samples) / (samples + 1.0);
  if (state->down_env[band] < threshold) {
    state->down_env[band] = threshold;
  }
  if (state->down_env[band] < min_envelope_power) {
    state->down_env[band] = min_envelope_power;
  }

  double compression = 1.0 - 1.0 / ott_clamp(ratio, 1.0, 100.0);
  double gain = pow(threshold / state->down_env[band], compression);
  if (!isfinite(gain)) {
    gain = 1.0;
  }
  return sample * gain;
}

/* Three-band downward compressor with Linkwitz-Riley 4th-order crossovers. */
__attribute__((always_inline)) double
ylc_audio_lr_comp_kernel(OttState *state, double spf, double thresh,
                         double ratio, double attack, double release,
                         double input) {
  const double low_cross_frequency = 120.0;
  const double high_cross_frequency = 2500.0;

  if (!state || spf <= 0.0 || !isfinite(spf)) {
    return input;
  }
  if (state->spf != spf) {
    *state = (OttState){0};
    state->spf = spf;
  }

  double x = isfinite(input) ? input : 0.0;
  double low = audio_jit_ott_filter(x, spf, low_cross_frequency, 0, 0,
                                    &state->low_x1[0], &state->low_x2[0],
                                    &state->low_y1[0], &state->low_y2[0]);
  low = audio_jit_ott_filter(low, spf, low_cross_frequency, 0, 1,
                             &state->low_x1[1], &state->low_x2[1],
                             &state->low_y1[1], &state->low_y2[1]);
  double rest = audio_jit_ott_filter(x, spf, low_cross_frequency, 1, 0,
                                     &state->high_x1[0], &state->high_x2[0],
                                     &state->high_y1[0], &state->high_y2[0]);
  rest = audio_jit_ott_filter(rest, spf, low_cross_frequency, 1, 1,
                              &state->high_x1[1], &state->high_x2[1],
                              &state->high_y1[1], &state->high_y2[1]);
  double mid = audio_jit_ott_filter(rest, spf, high_cross_frequency, 0, 0,
                                    &state->low_x1[2], &state->low_x2[2],
                                    &state->low_y1[2], &state->low_y2[2]);
  mid = audio_jit_ott_filter(mid, spf, high_cross_frequency, 0, 1,
                             &state->low_x1[3], &state->low_x2[3],
                             &state->low_y1[3], &state->low_y2[3]);
  double high = audio_jit_ott_filter(rest, spf, high_cross_frequency, 1, 0,
                                     &state->high_x1[2], &state->high_x2[2],
                                     &state->high_y1[2], &state->high_y2[2]);
  high = audio_jit_ott_filter(high, spf, high_cross_frequency, 1, 1,
                              &state->high_x1[3], &state->high_x2[3],
                              &state->high_y1[3], &state->high_y2[3]);

  return audio_jit_lr_band(low, spf, 0, thresh, ratio, attack, release, state) +
         audio_jit_lr_band(mid, spf, 1, thresh, ratio, attack, release, state) +
         audio_jit_lr_band(high, spf, 2, thresh, ratio, attack, release, state);
}
