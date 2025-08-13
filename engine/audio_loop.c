#include "audio_loop.h"
#include "../lang/ylc_datatypes.h"
#include "audio_loop_utils.h"
#include "audio_routing.h"
#include "ctx.h"
#include "node_gc.h"
#include "osc.h"
#include "scheduling.h"
#include <soundio/soundio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define RING_BUFFER_CAPACITY_SCALING 10

static void (*write_sample)(char *ptr, sample_t sample);
int scheduler_event_loop();

uint64_t get_frame_offset() {
  struct timespec t;
  struct timespec btime = get_block_time();
  uint64_t frame = get_current_sample();
  set_block_time(&t);
  int frame_offset = get_block_frame_offset(btime, t, 48000);
  return frame + frame_offset;
}

static struct timespec start_time;
static struct timespec block_time;

void set_block_time(struct timespec *to_set) {
  clock_gettime(CLOCK_MONOTONIC_RAW, to_set);
}

uint64_t us_offset(struct timespec start, struct timespec end) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000 +
                      (end.tv_nsec - start.tv_nsec) / 1000000;
  return delta_us;
}

int get_block_frame_offset(struct timespec start, struct timespec end,
                           int sample_rate) {

  sample_t ms_per_frame = 1000.0 / sample_rate;
  uint64_t ms = us_offset(start, end);
  return ((int)(ms / ms_per_frame)) % BUF_SIZE;
}

struct timespec get_block_time() { return block_time; }
struct timespec get_start_time() { return start_time; }

struct SoundIoRingBuffer *ring_buffer = NULL;

static char *preferred_input_device_name = NULL;

static enum SoundIoFormat prioritized_formats[] = {
    SoundIoFormatFloat32NE, SoundIoFormatFloat32FE, SoundIoFormatS32NE,
    SoundIoFormatS32FE,     SoundIoFormatS24NE,     SoundIoFormatS24FE,
    SoundIoFormatS16NE,     SoundIoFormatS16FE,     SoundIoFormatFloat64NE,
    SoundIoFormatFloat64FE, SoundIoFormatU32NE,     SoundIoFormatU32FE,
    SoundIoFormatU24NE,     SoundIoFormatU24FE,     SoundIoFormatU16NE,
    SoundIoFormatU16FE,     SoundIoFormatS8,        SoundIoFormatU8,
    SoundIoFormatInvalid,
};

static int prioritized_sample_rates[] = {
    48000, 44100, 96000, 24000, 0,
};

__attribute__((cold)) __attribute__((noreturn))
__attribute__((format(printf, 1, 2))) static void
panic(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  abort();
}

static int min_int(int a, int b) { return (a < b) ? a : b; }

static void read_callback(struct SoundIoInStream *instream, int frame_count_min,
                          int frame_count_max) {
  struct SoundIoChannelArea *areas;
  int err;

  if (!ring_buffer)
    return;

  sample_t *write_ptr = (sample_t *)soundio_ring_buffer_write_ptr(ring_buffer);
  int free_bytes = soundio_ring_buffer_free_count(ring_buffer);
  int bytes_per_frame = req_hardware_inputs * sizeof(sample_t);
  int free_frames = free_bytes / bytes_per_frame;

  if (frame_count_min > free_frames) {
    panic("Ring buffer overflow");
  }

  int write_frames = min_int(free_frames, frame_count_max);
  int frames_left = write_frames;

  while (frames_left > 0) {
    int frame_count = frames_left;

    if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
      panic("Begin read error: %s", soundio_strerror(err));
    }

    if (!frame_count)
      break;

    if (!areas) {
      memset(write_ptr, 0, frame_count * bytes_per_frame);
      fprintf(stderr, "Dropped %d frames\n", frame_count);
      write_ptr += frame_count * req_hardware_inputs;
    } else {
      for (int frame = 0; frame < frame_count; frame++) {
        for (int i = 0; i < num_input_mappings; i++) {
          int hw_ch = input_mappings[i].hardware_input;

          if (hw_ch < instream->layout.channel_count) {
            // Direct copy - no conversion needed if input is already float
            *write_ptr = *(sample_t *)areas[hw_ch].ptr;
          } else {
            *write_ptr = 0.0f;
          }
          write_ptr++;
        }

        for (int ch = 0; ch < instream->layout.channel_count; ch++) {
          areas[ch].ptr += areas[ch].step;
        }
      }
    }

    if ((err = soundio_instream_end_read(instream))) {
      panic("End read error: %s", soundio_strerror(err));
    }

    frames_left -= frame_count;
  }

  soundio_ring_buffer_advance_write_ptr(ring_buffer,
                                        write_frames * bytes_per_frame);
}

static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {

  struct SoundIoChannelArea *areas;
  Ctx *ctx = outstream->userdata;

  sample_t seconds_per_frame = 1.0f / (sample_t)outstream->sample_rate;
  int frames_left;
  int frame_count;
  int err;

  sample_t *read_ptr = NULL;
  int fill_bytes = 0;

  if (ring_buffer) {
    read_ptr = (sample_t *)soundio_ring_buffer_read_ptr(ring_buffer);
    fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
  }

  // Process input data from ring buffer - simplified
  if (read_ptr && ring_buffer) {
    int bytes_per_frame = req_hardware_inputs * sizeof(sample_t);
    int available_frames = fill_bytes / bytes_per_frame;
    int read_frames = min_int(frame_count_max, available_frames);

    // Direct copy from ring buffer to signal buffers
    for (int frame = 0; frame < read_frames; frame++) {
      for (int i = 0; i < num_input_mappings; i++) {
        InputMapping *mapping = &input_mappings[i];

        // Direct access - no casting needed
        sample_t sample = read_ptr[frame * req_hardware_inputs + i];

        // Store in the correct signal buffer
        Signal *sig = &ctx->input_signals[mapping->signal_index];
        int buffer_index = frame * sig->layout + mapping->signal_channel;

        if (buffer_index < BUF_SIZE * sig->layout) {
          sig->buf[buffer_index] = sample;
        }
      }
    }

    // Advance ring buffer read pointer
    soundio_ring_buffer_advance_read_ptr(ring_buffer,
                                         read_frames * bytes_per_frame);
  }

  frames_left = frame_count_max;

  for (;;) {
    int frame_count = frames_left;
    uint64_t buffer_start_sample = atomic_load(&global_sample_position);

    if ((err =
             soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      printf("unrecoverable stream error begin write: %s\n",
             soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;

    set_block_time(&block_time);
    user_ctx_callback(ctx, buffer_start_sample, frame_count, seconds_per_frame);

    // Simplified output - direct sample_t access
    for (int channel = 0; channel < 2; channel++) {
      for (int frame = 0; frame < frame_count; frame++) {
        int sample_idx = LAYOUT * frame + channel;
        sample_t sample = ctx->output_buf[sample_idx] * ctx->main_vol;

        // Direct write - assuming areas are already sample_t*
        *(sample_t *)areas[channel].ptr = sample;
        areas[channel].ptr += areas[channel].step;
      }
    }

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      printf("unrecoverable stream error end write: %s\n",
             soundio_strerror(err));
      exit(1);
    }

    frames_left -= frame_count;
    atomic_fetch_add(&global_sample_position, frame_count);

    if (frames_left <= 0)
      break;
  }
}

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", ++count);
}

struct SoundIoDevice *get_input_device(struct SoundIo *soundio,
                                       char *in_device_id) {

  if (preferred_input_device_name) {

    for (int i = 0; i < soundio_input_device_count(soundio); i += 1) {
      struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
      if (strcmp(device->name, preferred_input_device_name) == 0) {
        return device;
      }
    }
    panic("Preferred input device %s not found\n", preferred_input_device_name);
  }
  bool in_raw = false;
  int default_in_device_index = soundio_default_input_device_index(soundio);
  if (default_in_device_index < 0) {
    panic("no input device found");
  }

  // Find the input device
  int in_device_index = default_in_device_index;
  if (in_device_id) {
    bool found = false;
    for (int i = 0; i < soundio_input_device_count(soundio); i += 1) {

      struct SoundIoDevice *device = soundio_get_input_device(soundio, i);

      if (device->is_raw == in_raw && strcmp(device->id, in_device_id) == 0) {
        in_device_index = i;
        found = true;
        soundio_device_unref(device);
        break;
      }
      soundio_device_unref(device);
    }

    if (!found)
      panic("invalid input device id: %s", in_device_id);
  }

  struct SoundIoDevice *in_device =
      soundio_get_input_device(soundio, in_device_index);
  if (!in_device)
    panic("could not get input device: out of memory");
  return in_device;
}

struct SoundIoDevice *get_output_device(struct SoundIo *soundio,
                                        char *out_device_id) {
  bool out_raw = false;

  int default_out_device_index = soundio_default_output_device_index(soundio);
  if (default_out_device_index < 0)
    panic("no output device found");

  int out_device_index = default_out_device_index;
  if (out_device_id) {
    bool found = false;
    for (int i = 0; i < soundio_output_device_count(soundio); i += 1) {
      struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
      if (device->is_raw == out_raw && strcmp(device->id, out_device_id) == 0) {
        out_device_index = i;
        found = true;
        soundio_device_unref(device);
        break;
      }
      soundio_device_unref(device);
    }
    if (!found)
      panic("invalid output device id: %s", out_device_id);
  }

  struct SoundIoDevice *out_device =
      soundio_get_output_device(soundio, out_device_index);
  if (!out_device)
    panic("could not get output device: out of memory");
  return out_device;
}

void print_device_info(struct SoundIoDevice *in_device,
                       struct SoundIoDevice *out_device) {
  fprintf(stderr, ANSI_COLOR_RED);

  fprintf(stderr, "Input device: %s\n", in_device->name);
  fprintf(stderr, "Output device: %s\n", out_device->name);

  fprintf(stderr, "Input device has %d channels: ",
          in_device->current_layout.channel_count);
  for (int i = 0; i < in_device->current_layout.channel_count; i++) {
    fprintf(stderr, "%d%s", i + 1,
            (i < in_device->current_layout.channel_count - 1) ? "," : "");
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "Output device has %d channels: ",
          out_device->current_layout.channel_count);
  for (int i = 0; i < out_device->current_layout.channel_count; i++) {
    fprintf(stderr, "%d%s", i + 1,
            (i < out_device->current_layout.channel_count - 1) ? "," : "");
  }
  fprintf(stderr, "\n");

  fprintf(stderr,
          "Input device software latency - min: %.4f sec, max: %.4f sec, "
          "current: %.4f sec\n",
          in_device->software_latency_min, in_device->software_latency_max,
          in_device->software_latency_current);
  fprintf(stderr,
          "Output device software latency - min: %.4f sec, max: %.4f sec, "
          "current: %.4f sec\n",
          out_device->software_latency_min, out_device->software_latency_max,
          out_device->software_latency_current);

  fprintf(stderr, ANSI_COLOR_RESET);
}

void get_sample_rate(struct SoundIoDevice *in_device,
                     struct SoundIoDevice *out_device, int *sr) {

  int *sample_rate;
  for (sample_rate = prioritized_sample_rates; *sample_rate; sample_rate += 1) {
    if (soundio_device_supports_sample_rate(in_device, *sample_rate) &&
        soundio_device_supports_sample_rate(out_device, *sample_rate)) {
      break;
    }
  }
  if (!*sample_rate)
    panic("incompatible sample rates");
  *sr = *sample_rate;
}

void get_format(struct SoundIoDevice *in_device,
                struct SoundIoDevice *out_device, enum SoundIoFormat *f) {
  enum SoundIoFormat *fmt;
  for (fmt = prioritized_formats; *fmt != SoundIoFormatInvalid; fmt += 1) {
    if (soundio_device_supports_format(in_device, *fmt) &&
        soundio_device_supports_format(out_device, *fmt)) {
      break;
    }
  }
  if (*fmt == SoundIoFormatInvalid)
    panic("incompatible sample formats");
  *f = *fmt;
}
void print_available_devices(struct SoundIo *soundio) {
  fprintf(stderr, ANSI_COLOR_GREEN);

  fprintf(stderr, "Input devices:\n");
  for (int i = 0; i < soundio_input_device_count(soundio); i++) {
    struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
    fprintf(stderr, "  %d: %s%s%s\n", i, device->name,
            device->is_raw ? " (raw)" : "",
            i == soundio_default_input_device_index(soundio) ? " (default)"
                                                             : "");
    soundio_device_unref(device);
  }

  fprintf(stderr, "Output devices:\n");
  for (int i = 0; i < soundio_output_device_count(soundio); i++) {
    struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
    fprintf(stderr, "  %d: %s%s%s\n", i, device->name,
            device->is_raw ? " (raw)" : "",
            i == soundio_default_output_device_index(soundio) ? " (default)"
                                                              : "");
    soundio_device_unref(device);
  }

  fprintf(stderr, ANSI_COLOR_RESET);
}

int start_audio() {

  enum SoundIoBackend backend = SoundIoBackendNone;
  char *in_device_id = NULL;
  char *out_device_id = NULL;
  bool in_raw = false;
  bool out_raw = false;

  struct SoundIo *soundio = soundio_create();
  if (!soundio)
    panic("out of memory");

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, backend);
  if (err)
    panic("error connecting: %s", soundio_strerror(err));

  soundio_flush_events(soundio);
  print_available_devices(soundio);

  struct SoundIoDevice *in_device = get_input_device(soundio, in_device_id);
  struct SoundIoDevice *out_device = get_output_device(soundio, out_device_id);

  print_device_info(in_device, out_device);

  struct SoundIoChannelLayout out_layout;
  struct SoundIoChannelLayout in_layout;

  validate_in_layout(in_device, &in_layout);
  validate_out_layout(out_device, &out_layout);

  int sample_rate;
  get_sample_rate(in_device, out_device, &sample_rate);

  enum SoundIoFormat fmt;
  get_format(in_device, out_device, &fmt);

  struct SoundIoOutStream *outstream = soundio_outstream_create(out_device);
  if (!outstream)
    panic("out of memory");

  init_ctx();
  outstream->userdata = &ctx;
  outstream->format = fmt;
  outstream->sample_rate = sample_rate;
  outstream->layout = out_layout;
  outstream->write_callback = write_callback;
  outstream->underflow_callback = underflow_callback;

  set_out_format(out_device, outstream, &write_sample);

  if ((err = soundio_outstream_open(outstream))) {
    fprintf(stderr, "unable to open output stream: %s", soundio_strerror(err));
    return 1;
  }

  sample_t actual_latency = outstream->software_latency;
  fprintf(stderr, ANSI_COLOR_RED "Actual output latency: %.4f sec\n",
          actual_latency);

  struct SoundIoInStream *instream = NULL;

  if (req_hardware_inputs > 0) {
    instream = soundio_instream_create(in_device);
    if (!instream) {
      panic("out of memory");
    }
    instream->format = fmt;
    instream->sample_rate = sample_rate;
    instream->layout = in_layout;
    instream->software_latency = outstream->software_latency;
    instream->read_callback = read_callback;

    if ((err = soundio_instream_open(instream))) {
      fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
      return 1;
    }

    int channel_bytes_per_frame =
        req_hardware_inputs * instream->bytes_per_sample;

    int capacity = actual_latency * RING_BUFFER_CAPACITY_SCALING *
                   instream->sample_rate * channel_bytes_per_frame;

    ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!ring_buffer)
      panic("unable to create ring buffer: out of memory");

    char *buf = soundio_ring_buffer_write_ptr(ring_buffer);
    int fill_count =
        actual_latency * outstream->sample_rate * channel_bytes_per_frame;
    memset(buf, 0, fill_count);
    soundio_ring_buffer_advance_write_ptr(ring_buffer, fill_count);
  }

  print_routing_setup(outstream, instream, ring_buffer);

  fprintf(stderr, "\nStarting streams...\n");

  if (instream && (err = soundio_instream_start(instream)))
    panic("unable to start input device: %s", soundio_strerror(err));

  if ((err = soundio_outstream_start(outstream)))
    panic("unable to start output device: %s", soundio_strerror(err));

  ctx.sample_rate = outstream->sample_rate;
  ctx.spf = 1.0 / outstream->sample_rate;

  set_block_time(&start_time);
  return 0;
}

static int config[16];
static int32_t config_size = 0;
typedef struct IntLL {
  int32_t data;
  struct IntLL *next;
} IntLL;

void set_input_conf(char *conf) {
  IntLL *l = (IntLL *)conf;

  config_size = 0;
  while (l) {
    config[config_size] = l->data;
    config_size++;
    IntLL *prev = l;
    l = l->next;
  }
}

void set_input_device(_String name) {
  preferred_input_device_name = name.chars;
}

int init_audio() {
  maketable_sq();
  maketable_sin();
  maketable_saw();
  maketable_saw();
  maketable_grain_window();

  if (config_size) {
    parse_input_config(config, config_size);
  }
  start_audio();

  scheduler_event_loop();
  gc_loop(get_audio_ctx());
  // midi_setup();

  return 0;
}
