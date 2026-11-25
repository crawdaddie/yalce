#include "audio_clock.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

static pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
static AudioClock playback;
static atomic_uint generation;

static const double NS_PER_SECOND = 1000000000.;

double audio_clock_now(void) {
  struct timespec time;
  if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
    return NAN;
  }
  return time.tv_sec + time.tv_nsec / NS_PER_SECOND;
}

void audio_clock_invalidate(void) {
  atomic_fetch_add(&generation, 1);
}

void audio_clock_publish(uint64_t sample, double time, int sample_rate) {
  if (!isfinite(time) || time < 0. || sample_rate <= 0) {
    audio_clock_invalidate();
    return;
  }
  // Never wait for a reader on the audio thread; keep the preceding snapshot.
  if (pthread_mutex_trylock(&clock_mutex) != 0) {
    return;
  }
  playback = (AudioClock){sample, time, sample_rate, atomic_load(&generation)};
  pthread_mutex_unlock(&clock_mutex);
}

int audio_clock_read(AudioClock *snapshot) {
  pthread_mutex_lock(&clock_mutex);
  *snapshot = playback;
  pthread_mutex_unlock(&clock_mutex);
  return snapshot->sample_rate > 0 &&
         snapshot->generation == atomic_load(&generation) ? 0 : -1;
}
