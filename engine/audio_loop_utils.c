#include "./audio_loop_utils.h"
#include "ctx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void set_out_format(struct SoundIoDevice *device,
                    struct SoundIoOutStream *outstream, void **write_sample) {

  if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
    outstream->format = SoundIoFormatFloat32NE;
    *write_sample = write_sample_float32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {

    outstream->format = SoundIoFormatFloat64NE;
    *write_sample = write_sample_float64ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {

    outstream->format = SoundIoFormatS32NE;
    *write_sample = write_sample_s32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
    outstream->format = SoundIoFormatS16NE;
    *write_sample = write_sample_s16ne;
  } else {
    fprintf(stderr, "No suitable device format available.\n");
  }
}
int get_in_device(char *in_device_id, struct SoundIo *soundio,
                  struct SoundIoDevice **device) {

  bool in_raw = false;
  int in_device_index = -1;

  if (in_device_id) {
    int device_count = soundio_output_device_count(soundio);
    for (int i = 0; i < device_count; i += 1) {
      *device = soundio_get_input_device(soundio, i);

      bool select_this_one = strcmp((*device)->id, in_device_id) == 0 &&
                             (*device)->is_raw == in_raw;

      soundio_device_unref(*device);
      if (select_this_one) {
        in_device_index = i;
        break;
      }
    }
  } else {
    in_device_index = soundio_default_input_device_index(soundio);
  }

  *device = soundio_get_input_device(soundio, in_device_index);

  if (!*device) {
    fprintf(stderr, "could not get input device: out of memory");
    return 1;
  }
  return 0;
}

int get_out_device(char *device_id, struct SoundIo *soundio,
                   struct SoundIoDevice **device) {

  bool raw = false;

  int selected_device_index = -1;
  if (device_id) {
    int device_count = soundio_output_device_count(soundio);
    for (int i = 0; i < device_count; i += 1) {
      *device = soundio_get_output_device(soundio, i);

      bool select_this_one =
          strcmp((*device)->id, device_id) == 0 && (*device)->is_raw == raw;

      soundio_device_unref(*device);
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

  if (!device) {
    printf("out of memory\n");
    return 1;
  }

  *device = soundio_get_output_device(soundio, selected_device_index);

  if ((*device)->probe_error) {
    printf("Cannot probe device: %s\n",
           soundio_strerror((*device)->probe_error));
    return 1;
  }
  return 0;
}

int start_outstream(struct SoundIoOutStream *outstream) {
  int err;

  if ((err = soundio_outstream_open(outstream))) {
    printf("unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  if (outstream->layout_error) {
    printf("unable to set channel layout: %s\n",
           soundio_strerror(outstream->layout_error));
  }

  if ((err = soundio_outstream_start(outstream))) {
    printf("unable to start device: %s\n", soundio_strerror(err));

    return 1;
  }
  return 0;
}

int start_instream(struct SoundIoInStream *instream) {
  int err;

  if ((err = soundio_instream_open(instream))) {
    printf("unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  if (instream->layout_error) {
    printf("unable to set channel layout: %s\n",
           soundio_strerror(instream->layout_error));
  }

  if ((err = soundio_instream_start(instream))) {
    printf("unable to start device: %s\n", soundio_strerror(err));

    return 1;
  }
  return 0;
}

void list_audio_devices(struct SoundIo *soundio) {
  printf("\n=== Available Audio Devices ===\n");

  // List output devices
  int output_count = soundio_output_device_count(soundio);
  printf("\nOutput Devices (%d available):\n", output_count);
  for (int i = 0; i < output_count; i++) {
    struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
    printf("  %d: %s%s%s\n", i, device->name, device->is_raw ? " (raw)" : "",
           i == soundio_default_output_device_index(soundio) ? " (default)"
                                                             : "");
    soundio_device_unref(device);
  }

  // List input devices
  int input_count = soundio_input_device_count(soundio);
  printf("\nInput Devices (%d available):\n", input_count);
  for (int i = 0; i < input_count; i++) {
    struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
    printf("  %d: %s%s%s\n", i, device->name, device->is_raw ? " (raw)" : "",
           i == soundio_default_input_device_index(soundio) ? " (default)"
                                                            : "");
    soundio_device_unref(device);
  }

  printf("\n===============================\n");
}

// Initialize input signals based on routing configuration
int init_input_signals(Ctx *ctx, InputRoutingConfig config, int buffer_size) {

  // Count the number of signals needed
  int signal_count = 0;
  int channel_offset = 0;

  // Count how many signals we need by going through the layout array
  while (channel_offset < config.num_input_channels) {
    if (signal_count >= 8) { // Safety limit
      break;
    }

    int layout = config.input_channel_layouts[signal_count];
    if (layout <= 0) {
      fprintf(stderr, "Invalid layout value %d at position %d\n", layout,
              signal_count);
      return 0;
    }

    channel_offset += layout;
    signal_count++;
  }

  if (channel_offset != config.num_input_channels) {
    fprintf(
        stderr,
        "Warning: Channel count mismatch. Layouts total: %d, Expected: %d\n",
        channel_offset, config.num_input_channels);
  }

  // Allocate signal array
  ctx->input_signals = (Signal *)malloc(signal_count * sizeof(Signal));
  if (!ctx->input_signals) {
    fprintf(stderr, "Failed to allocate signal array\n");
    return 0;
  }

  // Initialize signals
  ctx->num_input_signals = signal_count;
  channel_offset = 0;

  for (int i = 0; i < signal_count; i++) {
    int layout = config.input_channel_layouts[i];

    ctx->input_signals[i].layout = layout;
    ctx->input_signals[i].size = buffer_size;
    ctx->input_signals[i].buf =
        (double *)calloc(layout * buffer_size, sizeof(double));

    // Log what we're creating
    printf("Created signal %d: %d channels (", i, layout);

    for (int ch = 0; ch < layout; ch++) {
      if (channel_offset + ch < config.num_input_channels) {
        printf("hw:%d%s", config.hardware_inputs[channel_offset + ch] + 1,
               (ch < layout - 1) ? "," : "");
      }
    }

    printf("), %d frames\n", buffer_size);

    channel_offset += layout;
  }

  // Store the current routing configuration
  ctx->total_input_channels = config.num_input_channels;

  return 1;
}
