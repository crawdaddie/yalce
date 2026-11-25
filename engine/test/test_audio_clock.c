#include <assert.h>
#include <math.h>
#include "../audio_clock.c"

int main(void) {
  AudioClock snapshot;
  assert(audio_clock_read(&snapshot) < 0);
  audio_clock_publish(48000, 10.032, 48000);
  assert(audio_clock_read(&snapshot) == 0);
  assert(snapshot.sample == 48000 && snapshot.time == 10.032);
  assert(snapshot.sample_rate == 48000);
  unsigned first_generation = snapshot.generation;

  // A busy reader must not block the audio callback or corrupt its snapshot.
  pthread_mutex_lock(&clock_mutex);
  audio_clock_publish(96000, 11.032, 48000);
  pthread_mutex_unlock(&clock_mutex);
  assert(audio_clock_read(&snapshot) == 0 && snapshot.sample == 48000);

  audio_clock_invalidate();
  assert(audio_clock_read(&snapshot) < 0);
  audio_clock_publish(96000, 11.032, 48000);
  assert(audio_clock_read(&snapshot) == 0);
  assert(snapshot.generation != first_generation);
  audio_clock_publish(0, NAN, 48000);
  assert(audio_clock_read(&snapshot) < 0);
  audio_clock_publish(0, 12., 0);
  assert(audio_clock_read(&snapshot) < 0);
  assert(isfinite(audio_clock_now()));
  return 0;
}
