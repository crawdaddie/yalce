#include "audio_loop.h"
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

static void (*write_sample)(char *ptr, double sample);
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

  double ms_per_frame = 1000.0 / sample_rate;
  uint64_t ms = us_offset(start, end);
  return ((int)(ms / ms_per_frame)) % BUF_SIZE;
}

struct timespec get_block_time() { return block_time; }
struct timespec get_start_time() { return start_time; }

struct SoundIoRingBuffer *ring_buffer = NULL;

static char *preferred_input_device_name = NULL;

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

// Removed old __read_callback - replaced by improved read_callback
static void read_callback(struct SoundIoInStream *instream, int frame_count_min,
                          int frame_count_max) {
  struct SoundIoChannelArea *areas;
  int err;

  if (!ring_buffer)
    return;

  char *write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
  int free_bytes = soundio_ring_buffer_free_count(ring_buffer);
  int bytes_per_frame = total_hardware_inputs * instream->bytes_per_sample;
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
      // Fill with silence on overflow
      memset(write_ptr, 0, frame_count * bytes_per_frame);
      fprintf(stderr, "Dropped %d frames\n", frame_count);
    } else {
      // Copy each frame
      for (int frame = 0; frame < frame_count; frame++) {
        // Copy our selected hardware inputs in order
        for (int i = 0; i < num_input_mappings; i++) {
          int hw_ch = input_mappings[i].hardware_input;

          if (hw_ch < instream->layout.channel_count) {
            memcpy(write_ptr, areas[hw_ch].ptr, instream->bytes_per_sample);
          } else {
            // Zero if hardware channel doesn't exist
            memset(write_ptr, 0, instream->bytes_per_sample);
          }

          write_ptr += instream->bytes_per_sample;
        }
        
        // Advance all area pointers after processing the frame
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

// Removed redundant input_mapping function - integrated into write_callback

// Removed redundant copy_ring_buffer_to_signals function - integrated into write_callback
// Removed debug_input_signals function - debug prints removed from audio callbacks
static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {

  struct SoundIoChannelArea *areas;
  Ctx *ctx = outstream->userdata;

  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;
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

  // Process input data from ring buffer using the new mapping system
  if (read_ptr && ring_buffer) {
    int bytes_per_frame = total_hardware_inputs * outstream->bytes_per_sample;
    int available_frames = fill_bytes / bytes_per_frame;
    int read_frames = min_int(frame_count_max, available_frames);
    
    // Copy data from ring buffer to signal buffers using input mappings
    for (int frame = 0; frame < read_frames; frame++) {
      for (int i = 0; i < num_input_mappings; i++) {
        InputMapping *mapping = &input_mappings[i];
        
        // Calculate position in ring buffer for this hardware input
        char *sample_ptr = read_ptr + (frame * total_hardware_inputs + i) * 
                                         outstream->bytes_per_sample;
        
        // Get the sample value
        float fsample = *(float *)sample_ptr;
        double sample = (double)fsample;
        
        // Store in the correct signal buffer
        Signal *sig = &ctx->input_signals[mapping->signal_index];
        int buffer_index = frame * sig->layout + mapping->signal_channel;
        
        if (buffer_index < BUF_SIZE * sig->layout) {
          sig->buf[buffer_index] = sample;
        }
      }
    }
    
    // Advance ring buffer read pointer
    soundio_ring_buffer_advance_read_ptr(ring_buffer, read_frames * bytes_per_frame);
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

    const struct SoundIoChannelLayout *layout = &outstream->layout;

    set_block_time(&block_time);

    user_ctx_callback(ctx, buffer_start_sample, frame_count, seconds_per_frame);

    int sample_idx;
    double sample;
    for (int channel = 0; channel < 2; channel += 1) {
      for (int frame = 0; frame < frame_count; frame += 1) {

        sample_idx = LAYOUT * frame + channel;
        sample = ctx->output_buf[sample_idx];

        write_sample(areas[channel].ptr, 0.5 * sample);
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
      total_hardware_inputs * outstream->bytes_per_sample;
  fprintf(stderr, "Format: %s\n", soundio_format_string(outstream->format));
  fprintf(stderr, "Sample rate: %d Hz\n", outstream->sample_rate);
  if (ring_buffer) {
    fprintf(stderr, "Buffer size: %.2f ms\n",
            (double)soundio_ring_buffer_capacity(ring_buffer) /
                channel_bytes_per_frame / instream->sample_rate * 1000.0);
  }

  fprintf(stderr, "Channel configuration:\n");
  print_input_mapping();

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
  // Get the built-in stereo layout
  const struct SoundIoChannelLayout *stereo_layout =
      soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);

  // soundio_device_sort_channel_layouts(out_device);
  // // Define a stereo channel layout (Front Left, Front Right)
  //
  // out_layout = soundio_best_matching_channel_layout(
  //     stereo_layout, 1, out_device->layouts, out_device->layout_count);
  //
  // if (!out_layout)
  //   panic("output channel layouts not compatible");

  // *_out_layout = *out_layout;
  *_out_layout = *stereo_layout;
}

// void validate_in_layout(struct SoundIoDevice *in_device,
//                         struct SoundIoChannelLayout *_in_layout, int size,
//                         int *input_map, int *num_in_channels,
//                         int *num_hardware_inputs, int
//                         *requested_input_channels, int *hw_in_to_sig_map) {
//
//   struct SoundIoChannelLayout *in_layout;
//
//   int *im = input_map;
//   int chans = 0;
//   int req_in_counter = 0;
//
//   while (size) {
//     int layout = *im;
//     int l = layout;
//     im++;
//     size--;
//     while (l--) {
//       int req_hw_input = *im;
//       if (req_hw_input >= in_device->current_layout.channel_count) {
//
//         panic("Invalid hardware input channel %d specified. Device only has
//         %d "
//               "channels\n",
//               req_hw_input + 1, in_device->current_layout.channel_count);
//       } else {
//         *num_hardware_inputs = *num_hardware_inputs + 1;
//         requested_input_channels[req_in_counter] = req_hw_input;
//         hw_in_to_sig_map[req_in_counter] = chans;
//         req_in_counter++;
//       }
//       im++;
//       size--;
//     }
//     chans++;
//   }
//
//   soundio_device_sort_channel_layouts(in_device);
//   in_layout = soundio_best_matching_channel_layout(
//       in_device->layouts, in_device->layout_count, in_device->layouts,
//       in_device->layout_count);
//
//   if (!in_layout)
//     panic("input channel layouts not compatible");
//   *_in_layout = *in_layout;
//   *num_in_channels = chans;
// }
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

  // int num_chans;
  // validate_in_layout(in_device, &in_layout, config_size, input_map,
  // &num_chans,
  //                    &num_hardware_inputs, requested_input_channels,
  //                    hw_in_to_sig_map);
  //
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

  // The actual latency may be different from what we requested
  double actual_latency = outstream->software_latency;
  fprintf(stderr, ANSI_COLOR_RED "Actual output latency: %.4f sec\n",
          actual_latency);

  struct SoundIoInStream *instream = NULL;

  if (total_hardware_inputs > 0) {
    // Now create input stream with the same latency as the output stream
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

    // Create a ring buffer for our SELECTED channels, not all channels
    int channel_bytes_per_frame =
        total_hardware_inputs * instream->bytes_per_sample;

    int capacity =
        actual_latency * 24 * instream->sample_rate * channel_bytes_per_frame;

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
  // printf("set input conf %p\n", conf);
  IntLL *l = (IntLL *)conf;

  config_size = 0;
  while (l) {
    config[config_size] = l->data;
    config_size++;
    IntLL *prev = l;
    l = l->next;
  }
}

void set_input_device(_YLC_String name) {
  preferred_input_device_name = name.chars;
  // printf("set input conf %p\n", conf);
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
