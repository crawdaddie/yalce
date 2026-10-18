#ifndef AUDIO_JIT_OTT_KERNELS_H
#define AUDIO_JIT_OTT_KERNELS_H

#include <stdint.h>

typedef struct OttState {
  double low_x1[4];
  double low_x2[4];
  double low_y1[4];
  double low_y2[4];
  double high_x1[4];
  double high_x2[4];
  double high_y1[4];
  double high_y2[4];
  double up_env[3];
  double down_env[3];
  double spf;
  double low_frequency;
  double high_frequency;
} OttState;

double ylc_audio_ott_kernel(OttState *state, double spf, double depth,
                            double upward, double downward, double attack,
                            double release, double input);

double ylc_audio_ott_freq_kernel(OttState *state, double spf, double depth,
                                 double upward, double downward,
                                 double attack, double release,
                                 double low_frequency, double high_frequency,
                                 double input);

double ylc_audio_lr_comp_kernel(OttState *state, double spf, double thresh,
                                double ratio, double attack, double release,
                                double input);

#endif
