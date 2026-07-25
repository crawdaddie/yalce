#ifndef AUDIO_JIT_KERNELS_H
#define AUDIO_JIT_KERNELS_H

typedef struct {
  double phase;
} SinOscState;

// Square wave oscillator
typedef struct SqOscState {
  double phase;
} SqOscState;

typedef struct SawOscState {
  double phase;
} SawOscState;

typedef struct PmOscState {
  double carrier_phase;
  double modulator_phase;
} PmOscState;
#endif
