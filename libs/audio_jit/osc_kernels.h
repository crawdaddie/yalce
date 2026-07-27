#ifndef AUDIO_JIT_KERNELS_H
#define AUDIO_JIT_KERNELS_H

#include <stdint.h>

typedef struct {
  double phase;
} PhasorState;

typedef struct {
  double phase;
  uint64_t initialized;
} TrigState;

typedef struct {
  uint64_t fired;
} TrigOnceState;

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

typedef struct ChangedState {
  double prev;
  uint64_t seen;
} ChangedState;

typedef struct DecayState {
  double value;
  double prev_trig;
} DecayState;

typedef struct LfNoiseState {
  TrigState trig;
  double value;
  double slope;
  uint64_t initialized;
} LfNoiseState;

typedef struct LfNoise0State {
  TrigState trig;
  double value;
  uint64_t initialized;
} LfNoise0State;

typedef struct KillOnEndState {
  double prev;
} KillOnEndState;

typedef struct AdsrState {
  double value;
  double phase;
  double prev_trig;
} AdsrState;

typedef struct RectState {
  double remaining;
  double prev_trig;
} RectState;

typedef struct BufplayState {
  double phase;
  double prev_trig;
  double active;
} BufplayState;
#endif
