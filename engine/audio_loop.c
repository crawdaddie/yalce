#ifndef _ENGINE_AUDIO_LOOP_H
#define _ENGINE_AUDIO_LOOP_H
#include "./audio_loop.h"
#include "./ctx.h"
#include "./midi.h"
#include "./node_gc.h"
#include "./osc.h"
#include "./scheduling.h"
#include <errno.h>
#include <pthread.h>
#include <soundio/soundio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define INDENT(indent) write_log("%.*s", indent, "  ")

static struct SoundIo *soundio = NULL;
static struct SoundIoDevice *device = NULL;
static struct SoundIoOutStream *outstream = NULL;

static void (*write_sample)(char *ptr, double sample);
static volatile bool want_pause = false;

void write_sample_s16ne(char *ptr, double sample) {
  int16_t *buf = (int16_t *)ptr;
  double range = (double)INT16_MAX - (double)INT16_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

void write_sample_s32ne(char *ptr, double sample) {
  int32_t *buf = (int32_t *)ptr;
  double range = (double)INT32_MAX - (double)INT32_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

void write_sample_float32ne(char *ptr, double sample) {
  float *buf = (float *)ptr;
  *buf = sample;
}

void add_sample_float32ne_w_offset(char *ptr, int offset, double sample) {
  float *buf = (float *)(ptr + offset);
  *buf += sample;
}

void write_sample_float64ne(char *ptr, double sample) {
  double *buf = (double *)ptr;
  *buf = sample;
}

// static long block_time;

// static time_t loc_time;
static struct timespec start_time;
static struct timespec block_time;

void set_block_time(struct timespec *to_set) {
  clock_gettime(CLOCK_MONOTONIC_RAW, to_set);
}

uint64_t us_offset(struct timespec start, struct timespec end) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  // uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 +
  //                     (end.tv_nsec - start.tv_nsec) / 1000.0;
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000 +
                      (end.tv_nsec - start.tv_nsec) / 1000000;
  return delta_us;
}

int get_block_frame_offset(struct timespec start, struct timespec end,
                           int sample_rate) {

  double ms_per_frame = 1000.0 / sample_rate;
  uint64_t ms = us_offset(start, end);
  return ((int)(ms / ms_per_frame)) % 512;
}

struct timespec get_block_time() { return block_time; }
struct timespec get_start_time() { return start_time; }

static void _write_callback(struct SoundIoOutStream *outstream,
                            int frame_count_min, int frame_count_max) {
  // printf("frame count %d %d\n", frame_count_min, frame_count_max);

  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;

  struct SoundIoChannelArea *areas;
  Ctx *ctx = outstream->userdata;
  int err;

  int frames_left = frame_count_max;

  for (;;) {
    int frame_count = frames_left;
    if ((err =
             soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      printf("unrecoverable stream error: %s\n", soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;

    const struct SoundIoChannelLayout *layout = &outstream->layout;

    // size_t len_msgs = queue_size(ctx->queue);

    // for (int i = 0; i < len_msgs; i++) {
    //   Msg msg = queue_pop_left(&ctx->queue);
    //
    //   if (msg.handler != NULL) {
    //     int block_offset = get_msg_block_offset(msg, *ctx,
    //     float_sample_rate);
    //
    //     msg.handler(ctx, msg, block_offset);
    //   }
    // }

    set_block_time(&block_time);

    user_ctx_callback(ctx, frame_count, seconds_per_frame);

    int sample_idx;
    double sample;
    // Signal DAC = ctx->dac_buffer;

    for (int channel = 0; channel < layout->channel_count; channel += 1) {
      for (int frame = 0; frame < frame_count; frame += 1) {

        sample_idx = LAYOUT * frame + channel;
        sample = ctx->output_buf[sample_idx];

        write_sample(areas[channel].ptr, 0.0625 * sample);
        areas[channel].ptr += areas[channel].step;
      }
    }

    // ctx->block_time = get_time();

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      printf("unrecoverable stream error: %s\n", soundio_strerror(err));
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
  printf("underflow %d\n", count++);
}

int start_audio() {
  enum SoundIoBackend backend = SoundIoBackendNone;
  char *device_id = NULL;
  // char *device_id = "BuiltInSpeakerDevice";

  bool raw = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  char *filename = NULL;
  printf("------------------\n");

  soundio = soundio_create();

  if (!soundio) {
    printf("out of memory\n");
    return 1;
  }

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, backend);

  if (err) {
    printf("Unable to connect to backend: %s\n", soundio_strerror(err));
    return 1;
  }
  printf(ANSI_COLOR_MAGENTA "Simple Synth" ANSI_COLOR_RESET "\n");
  printf("Backend:           %s\n",
         soundio_backend_name(soundio->current_backend));

  soundio_flush_events(soundio);

  int selected_device_index = -1;
  if (device_id) {
    int device_count = soundio_output_device_count(soundio);
    for (int i = 0; i < device_count; i += 1) {
      device = soundio_get_output_device(soundio, i);
      bool select_this_one =
          strcmp(device->id, device_id) == 0 && device->is_raw == raw;

      soundio_device_unref(device);
      if (select_this_one) {
        selected_device_index = i;
        break;
      }
    }
  } else {
    selected_device_index = soundio_default_output_device_index(soundio);
  }

  if (selected_device_index < 0) {
    printf("Output device not found\n");
    return 1;
  }

  struct SoundIoDevice *device =
      soundio_get_output_device(soundio, selected_device_index);
  if (!device) {
    printf("out of memory\n");
    return 1;
  }

  printf("Output device:     %s\n", device->name);
  // printf("Output device ID:  %s\n", device->id);

  if (device->probe_error) {
    printf("Cannot probe device: %s\n", soundio_strerror(device->probe_error));
    return 1;
  }

  struct SoundIoOutStream *outstream = soundio_outstream_create(device);
  if (!outstream) {
    printf("out of memory\n");
    return 1;
  }

  init_ctx();

  outstream->userdata = &ctx;
  outstream->write_callback = _write_callback;

  outstream->underflow_callback = underflow_callback;
  outstream->name = stream_name;
  outstream->software_latency = latency;
  outstream->sample_rate = sample_rate;

  if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
    outstream->format = SoundIoFormatFloat32NE;
    write_sample = write_sample_float32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {

    outstream->format = SoundIoFormatFloat64NE;
    write_sample = write_sample_float64ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {

    outstream->format = SoundIoFormatS32NE;
    write_sample = write_sample_s32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {

    outstream->format = SoundIoFormatS16NE;
    write_sample = write_sample_s16ne;
  } else {
    printf("No suitable device format available.\n");
    return 1;
  }

  if ((err = soundio_outstream_open(outstream))) {
    printf("unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  if (outstream->layout_error)
    printf("unable to set channel layout: %s\n",
           soundio_strerror(outstream->layout_error));

  if ((err = soundio_outstream_start(outstream))) {

    printf("unable to start device: %s\n", soundio_strerror(err));

    return 1;
  }
  printf("Software latency:  %f\n", outstream->software_latency);
  printf("Sample rate:       %d\n", outstream->sample_rate);
  // ctx.sample_rate = ctx.sample_rate;
  // ctx.spf = 1.0 / ctx.sample_rate;
  ctx.sample_rate = outstream->sample_rate;
  ctx.spf = 1.0 / outstream->sample_rate;

  printf("------------------\n");

  set_block_time(&start_time);
  return 0;
}

static int msleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
    res = nanosleep(&ts, NULL);
  } while (res && errno == EINTR);

  return res;
}

int init_audio() {
  printf("init audio\n");
  maketable_sq();
  maketable_sin();
  maketable_saw();
  start_audio();
  scheduler_event_loop();
  gc_loop(get_audio_ctx());
  midi_setup();

  return 0;
}

#endif
