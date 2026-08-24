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

typedef struct ChangeUpState {
  double prev;
  uint64_t seen;
} ChangeUpState;

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

typedef struct ArrayEnvState {
  double phase;
  double prev_trig;
  int32_t current_segment;
  double start;
  double target;
  double time;
  double curve;
  int32_t active;
  int32_t sustaining;
} ArrayEnvState;

typedef struct RectState {
  double remaining;
  double prev_trig;
} RectState;

typedef struct BufplayState {
  double phase;
  double prev_trig;
  double active;
} BufplayState;

typedef struct GrainOscState {
  int32_t max_grains;
  int32_t active_grains;
  double prev_trig;
  uint8_t storage[];
} GrainOscState;

typedef struct DelayLineState {
  int32_t size;
  int32_t write_pos;
  double storage[];
} DelayLineState;

typedef struct LagState {
  int initialized;
  double y1;
  double b1;
  double lag_secs;
} LagState;

typedef struct ArrayChooseState {
  int initialized;
  double value;
  double prev_trig;
} ArrayChooseState;

typedef struct ArraySeqState {
  double initialized;
  double value;
  double prev_trig;
  int32_t counter;
} ArraySeqState;

typedef struct SahState {
  int initialized;
  double prev_trig;
  double value;
} SahState;

/* disperser: cascade of identical 2nd-order allpass sections. Each section
   holds two input taps (x1, x2) and two output taps (y1, y2). Up to
   DISPERSER_MAX_STAGES sections are reserved; the active count is chosen per
   sample from the `amount` parameter. See ylc_audio_disperser_kernel. */
#define DISPERSER_MAX_STAGES 64

typedef struct DisperserState {
  double x1[DISPERSER_MAX_STAGES];
  double x2[DISPERSER_MAX_STAGES];
  double y1[DISPERSER_MAX_STAGES];
  double y2[DISPERSER_MAX_STAGES];
} DisperserState;

/* pan: distribute a mono signal across N output channels (equal-power).
   `out` is a buffer of `n` doubles written by the kernel. See
   ylc_audio_pan_kernel in osc_kernels.c. */
void ylc_audio_pan_kernel(double *out, int n, double pos, double signal);

#endif
