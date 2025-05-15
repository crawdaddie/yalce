
#include "audio_loop.h"
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

int scheduler_event_loop();

uint64_t get_frame_offset() {
  struct timespec t;
  struct timespec btime = get_block_time();
  uint64_t frame = get_current_sample();
  set_block_time(&t);
  int frame_offset = get_block_frame_offset(btime, t, 48000);
  // printf("frame offset %d\n", frame_offset);
  return frame + frame_offset;
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
  return ((int)(ms / ms_per_frame)) % BUF_SIZE;
}

struct timespec get_block_time() { return block_time; }
struct timespec get_start_time() { return start_time; }

struct SoundIoRingBuffer *ring_buffer = NULL;

typedef struct InputRoutingConfig {
  int num_input_channels;
  int hardware_inputs[8];
  int input_channel_layouts[8];
} InputRoutingConfig;

typedef struct OutputRoutingConfig {
  int num_output_channels;
  int hardware_outputs[8];
} OutputRoutingConfig;

static int num_hardware_inputs = 3;
static int input_channels[8] = {0, 2, 3, 0, 0, 0, 0, 0};
static int input_layouts[8] = {1, 2};

static int num_hardware_outputs = 2;
static int output_channels[8] = {0, 1};

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
  char *write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
  int free_bytes = soundio_ring_buffer_free_count(ring_buffer);

  // Calculate bytes per frame for our SELECTED channels (not all channels)
  int selected_bytes_per_frame =
      num_hardware_inputs * instream->bytes_per_sample;
  int free_count = free_bytes / selected_bytes_per_frame;

  if (frame_count_min > free_count) {
    panic("ring buffer overflow");
  }

  int write_frames = min_int(free_count, frame_count_max);
  int frames_left = write_frames;

  for (;;) {
    int frame_count = frames_left;

    if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
      panic("begin read error: %s", soundio_strerror(err));

    if (!frame_count)
      break;

    if (!areas) {
      // Due to an overflow there is a hole. Fill the ring buffer with
      // silence for the size of the hole.
      memset(write_ptr, 0, frame_count * selected_bytes_per_frame);
      fprintf(stderr, "Dropped %d frames due to internal overflow\n",
              frame_count);
    } else {
      for (int frame = 0; frame < frame_count; frame += 1) {
        // Only copy the selected input channels, not all of them
        for (int i = 0; i < num_hardware_inputs; i++) {
          int ch = input_channels[i];
          if (ch < instream->layout.channel_count) {
            memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
            write_ptr += instream->bytes_per_sample;
          }
          areas[ch].ptr += areas[ch].step;
        }
      }
    }

    if ((err = soundio_instream_end_read(instream)))
      panic("end read error: %s", soundio_strerror(err));

    frames_left -= frame_count;
    if (frames_left <= 0)
      break;
  }

  int advance_bytes = write_frames * selected_bytes_per_frame;
  soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);
}

static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {
  struct SoundIoChannelArea *areas;
  int frames_left;
  int frame_count;
  int err;

  char *read_ptr;
  int fill_bytes;
  if (ring_buffer) {
    read_ptr = soundio_ring_buffer_read_ptr(ring_buffer);
    fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
  } else {
    read_ptr = NULL;
  }

  // Calculate bytes per frame for our SELECTED channels (not all channels)
  int selected_bytes_per_frame =
      num_hardware_inputs * outstream->bytes_per_sample;
  int fill_count = fill_bytes / selected_bytes_per_frame;

  int read_count = min_int(frame_count_max, fill_count);
  frames_left = read_count;

  while (frames_left > 0) {
    int frame_count = frames_left;

    if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
      panic("begin write error: %s", soundio_strerror(err));

    if (frame_count <= 0)
      break;
    if (!read_ptr) {
    } else {

      for (int frame = 0; frame < frame_count; frame += 1) {
        // First, clear all output channels
        for (int ch = 0; ch < outstream->layout.channel_count; ch++) {
          memset(areas[ch].ptr, 0, outstream->bytes_per_sample);
        }

        // Then, write selected input channels to selected output channels
        for (int i = 0; i < num_hardware_inputs; i++) {
          int out_ch =
              output_channels[i % num_hardware_outputs]; // Cycle through output
                                                         // channels
          if (out_ch < outstream->layout.channel_count) {
            memcpy(areas[out_ch].ptr, read_ptr, outstream->bytes_per_sample);
            read_ptr += outstream->bytes_per_sample;
          }
          areas[out_ch].ptr += areas[out_ch].step;
        }

        // Advance the remaining channel pointers
        for (int ch = 0; ch < outstream->layout.channel_count; ch++) {
          bool is_used = false;
          for (int i = 0; i < num_hardware_inputs && i < num_hardware_outputs;
               i++) {
            if (output_channels[i] == ch) {
              is_used = true;
              break;
            }
          }
          if (!is_used) {
            areas[ch].ptr += areas[ch].step;
          }
        }
      }
    }

    if ((err = soundio_outstream_end_write(outstream)))
      panic("end write error: %s", soundio_strerror(err));

    frames_left -= frame_count;
  }

  if (ring_buffer) {
    soundio_ring_buffer_advance_read_ptr(ring_buffer,
                                         read_count * selected_bytes_per_frame);
  }
}

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", ++count);
}

struct SoundIoDevice *get_input_device(struct SoundIo *soundio,
                                       char *in_device_id) {
  bool in_raw = false;
  int default_in_device_index = soundio_default_input_device_index(soundio);
  if (default_in_device_index < 0)
    panic("no input device found");

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

  // Find the output device
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

  // Print input device channels
  fprintf(stderr, "Input device has %d channels: ",
          in_device->current_layout.channel_count);
  for (int i = 0; i < in_device->current_layout.channel_count; i++) {
    fprintf(stderr, "%d%s", i + 1,
            (i < in_device->current_layout.channel_count - 1) ? "," : "");
  }
  fprintf(stderr, "\n");

  // Print output device channels
  fprintf(stderr, "Output device has %d channels: ",
          out_device->current_layout.channel_count);
  for (int i = 0; i < out_device->current_layout.channel_count; i++) {
    fprintf(stderr, "%d%s", i + 1,
            (i < out_device->current_layout.channel_count - 1) ? "," : "");
  }
  fprintf(stderr, "\n");

  // Get device capabilities info
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
void print_routing_setup(struct SoundIoOutStream *outstream,
                         struct SoundIoInStream *instream,
                         struct SoundIoRingBuffer *ring_buffer) {

  fprintf(stderr, ANSI_COLOR_BLUE);

  int channel_bytes_per_frame =
      num_hardware_inputs * outstream->bytes_per_sample;
  fprintf(stderr, "Format: %s\n", soundio_format_string(outstream->format));
  fprintf(stderr, "Sample rate: %d Hz\n", outstream->sample_rate);
  if (ring_buffer) {
    fprintf(stderr, "Buffer size: %.2f ms\n",
            (double)soundio_ring_buffer_capacity(ring_buffer) /
                channel_bytes_per_frame / instream->sample_rate * 1000.0);
  }

  fprintf(stderr, "Channel configuration:\n");
  fprintf(stderr, "  Input channels: ");
  for (int i = 0; i < num_hardware_inputs; i++) {
    fprintf(stderr, "%d ", input_channels[i] + 1);
  }

  fprintf(stderr, "\n  Output channels: ");
  for (int i = 0; i < num_hardware_outputs; i++) {
    fprintf(stderr, "%d ", output_channels[i] + 1);
  }

  fprintf(stderr, "Press Ctrl+C to exit.\n");

  fprintf(stderr, ANSI_COLOR_RESET);
}
void validate_out_layout(struct SoundIoDevice *out_device,
                         struct SoundIoChannelLayout *_out_layout) {

  struct SoundIoChannelLayout *out_layout;

  // Check if requested output channels are valid
  for (int i = 0; i < 2; i++) {
    if (output_channels[i] >= out_device->current_layout.channel_count) {
      panic("Invalid output channel %d specified. Device only has %d channels "
            "(0-%d).",
            output_channels[i] + 1, out_device->current_layout.channel_count,
            out_device->current_layout.channel_count - 1);
    }
  }

  soundio_device_sort_channel_layouts(out_device);
  out_layout = soundio_best_matching_channel_layout(
      out_device->layouts, out_device->layout_count, out_device->layouts,
      out_device->layout_count);
  if (!out_layout)
    panic("output channel layouts not compatible");

  *_out_layout = *out_layout;
}

void validate_in_layout(struct SoundIoDevice *in_device,
                        struct SoundIoChannelLayout *_in_layout) {

  struct SoundIoChannelLayout *in_layout;
  int n = num_hardware_inputs;
  for (int i = 0; i < n; i++) {
    if (input_channels[i] >= in_device->current_layout.channel_count) {

      fprintf(stderr,
              "Invalid input channel %d specified. Device only has %d channels "
              "(0-%d).",
              input_channels[i] + 1, in_device->current_layout.channel_count,
              in_device->current_layout.channel_count - 1);
      input_channels[i] = 0;
      num_hardware_inputs--;
    }
  }

  soundio_device_sort_channel_layouts(in_device);
  in_layout = soundio_best_matching_channel_layout(
      in_device->layouts, in_device->layout_count, in_device->layouts,
      in_device->layout_count);

  if (!in_layout)
    panic("input channel layouts not compatible");
  *_in_layout = *in_layout;
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

  // Print available devices
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

int start_audio(int num_hw_ins, int *in_channel_map, int *layouts) {
  if (num_hw_ins > 0) {
    num_hardware_inputs = num_hw_ins;
    for (int i = 0; i < 8; i++) {
      input_channels[i] = in_channel_map[i];
      input_layouts[i] = layouts[i];
    }
  } else {
    num_hardware_inputs = 0;
    for (int i = 0; i < 8; i++) {
      input_channels[i] = 0;
      input_layouts[i] = 0;
    }
  }

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
  struct SoundIoDevice *out_device = get_output_device(soundio, in_device_id);
  print_device_info(in_device, out_device);

  struct SoundIoChannelLayout out_layout;
  struct SoundIoChannelLayout in_layout;
  validate_in_layout(in_device, &in_layout);
  validate_out_layout(out_device, &out_layout);

  int sample_rate;
  get_sample_rate(in_device, out_device, &sample_rate);

  enum SoundIoFormat fmt;
  get_format(in_device, out_device, &fmt);

  // Create output stream first, so we can get its actual latency
  struct SoundIoOutStream *outstream = soundio_outstream_create(out_device);
  if (!outstream)
    panic("out of memory");
  outstream->format = fmt;
  outstream->sample_rate = sample_rate;
  outstream->layout = out_layout;
  outstream->write_callback = write_callback;
  outstream->underflow_callback = underflow_callback;

  if ((err = soundio_outstream_open(outstream))) {
    fprintf(stderr, "unable to open output stream: %s", soundio_strerror(err));
    return 1;
  }

  // The actual latency may be different from what we requested
  double actual_latency = outstream->software_latency;
  fprintf(stderr, ANSI_COLOR_RED "Actual output latency: %.4f sec\n",
          actual_latency);

  struct SoundIoInStream *instream = NULL;
  if (num_hardware_inputs > 0) {
    // Now create input stream with the same latency as the output stream
    instream = soundio_instream_create(in_device);
    if (!instream)
      panic("out of memory");
    instream->format = fmt;
    instream->sample_rate = sample_rate;
    instream->layout = in_layout;
    instream->software_latency = outstream->software_latency;
    instream->read_callback = read_callback;

    if ((err = soundio_instream_open(instream))) {
      fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
      return 1;
    }

    // Create a ring buffer for our SELECTED channels, not all channels
    int channel_bytes_per_frame =
        num_hardware_inputs * instream->bytes_per_sample;

    int capacity =
        actual_latency * 3 * instream->sample_rate * channel_bytes_per_frame;

    ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!ring_buffer)
      panic("unable to create ring buffer: out of memory");

    // Pre-fill the ring buffer with silence to avoid initial underflows
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

  fprintf(stderr, "\nPress Ctrl+C to exit.\n");

  return 0;
}

// Helper function to parse comma-separated list of integers
static int parse_int_list(const char *str, int *list, int max_values) {
  int count = 0;
  char *copy = strdup(str);
  char *token = strtok(copy, ",");

  while (token != NULL && count < max_values) {
    list[count++] = atoi(token);
    token = strtok(NULL, ",");
  }

  free(copy);
  return count;
}

int init_audio(int *config) {
  maketable_sq();
  maketable_sin();
  maketable_saw();
  maketable_saw();
  maketable_grain_window();

  if (config) {
    int numins = config[0];
    config++;
    int hw_ins[numins];
    for (int i = 0; i < numins; i++) {
      hw_ins[i] = *config;
      config++;
    }
    start_audio(numins, hw_ins, config);
  } else {
    start_audio(0, NULL, NULL);
  }

  scheduler_event_loop();
  gc_loop(get_audio_ctx());
  // midi_setup();

  return 0;
}
