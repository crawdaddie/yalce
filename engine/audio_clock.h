#ifndef ENGINE_AUDIO_CLOCK_H
#define ENGINE_AUDIO_CLOCK_H
#include <stdint.h>

// Monotonic presentation time of the next frame written to the output.
typedef struct {
  uint64_t sample;
  double time;
  int sample_rate;
  unsigned generation;
} AudioClock;

double audio_clock_now(void);
void audio_clock_publish(uint64_t sample, double time, int sample_rate);
void audio_clock_invalidate(void);
int audio_clock_read(AudioClock *snapshot);
#endif
