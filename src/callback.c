#include <soundio/soundio.h>

#include "ctx.h"
#include "sched.c"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./channel.c"

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

static void (*write_sample)(char *ptr, double sample);

static volatile bool want_pause = false;

static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {
  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;

  struct SoundIoChannelArea *areas;
  int err;

  int frames_left = frame_count_max;

  for (;;) {
    int frame_count = frames_left;
    if ((err =
             soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      fprintf(stderr, "unrecoverable stream error: %s\n",
              soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    /* sched_incr_time(seconds_per_frame * frame_count); */

    user_ctx_callback(frame_count, seconds_per_frame);

    for (int frame = 0; frame < frame_count; frame += 1) {

      for (int channel = 0; channel < layout->channel_count; channel += 1) {
        write_sample(areas[channel].ptr,
                     sum_channel_output_destructive(channel, frame));
        areas[channel].ptr += areas[channel].step;
      }
    }

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      fprintf(stderr, "unrecoverable stream error: %s\n",
              soundio_strerror(err));
      exit(1);
    }

    frames_left -= frame_count;
    if (frames_left <= 0)
      break;
  }

  soundio_outstream_pause(outstream, want_pause);
}

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", count++);
}
