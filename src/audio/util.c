#ifndef _AUDIO_UTIL
#define _AUDIO_UTIL
#include <math.h>
#include <soundio/soundio.h>
#include <stdint.h>
#include <stdlib.h>

static void write_sample_s16ne(char *ptr, double sample) {
  int16_t *buf = (int16_t *)ptr;
  double range = (double)INT16_MAX - (double)INT16_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

static void write_sample_s32ne(char *ptr, double sample) {
  int32_t *buf = (int32_t *)ptr;
  double range = (double)INT32_MAX - (double)INT32_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

static void write_sample_float32ne(char *ptr, double sample) {
  float *buf = (float *)ptr;
  *buf = sample;
}

static void write_sample_float64ne(char *ptr, double sample) {
  double *buf = (double *)ptr;
  *buf = sample;
}

double get_sample_interp(double read_ptr, double *buf, int max_frames) {
  double r = read_ptr;
  if (r >= max_frames) {
    return 0.0;
  }
  if (r <= 0) {
    r = max_frames + r;
  }
  int frame = ((int)r);
  double fraction = r - frame;
  double result = buf[frame] * fraction;
  result += buf[frame + 1] * (1.0 - fraction);
  return result;
}
#endif
