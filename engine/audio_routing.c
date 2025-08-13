#include "./audio_routing.h"
#include "common.h"
#include <soundio/soundio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global arrays (replace the current complex mapping)
InputMapping input_mappings[MAX_INPUTS];
SignalInfo signal_info[MAX_SIGNALS];
int num_input_mappings = 0;
int num_signals = 0;
int req_hardware_inputs = 0;

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
// Parse the configuration array into a simple structure
void parse_input_config(int *config, int config_size) {
  num_input_mappings = 0;
  num_signals = 0;
  req_hardware_inputs = 0;

  int *ptr = config;
  int remaining = config_size;
  int signal_index = 0;

  printf("Parsing input config: ");
  for (int i = 0; i < config_size; i++) {
    printf("%d ", config[i]);
  }
  printf("\n");

  while (remaining > 0) {
    int channels = *ptr++;
    remaining--;

    if (remaining < channels) {
      panic("Invalid config: not enough hardware inputs for signal %d",
            signal_index);
    }

    // Store signal info
    signal_info[signal_index].num_channels = channels;
    // signal_info[signal_index].start_hw_index = total_hardware_inputs;

    signal_info[signal_index].start_hw_index = *ptr;
    printf("Signal %d: %d channels, hw inputs: ", signal_index, channels);

    // Parse hardware inputs for this signal
    for (int ch = 0; ch < channels; ch++) {
      int hw_input = *ptr++;
      remaining--;

      printf("%d ", hw_input);

      // Create mapping entry
      input_mappings[num_input_mappings].signal_index = signal_index;
      input_mappings[num_input_mappings].signal_channel = ch;
      input_mappings[num_input_mappings].hardware_input = hw_input;
      num_input_mappings++;
      req_hardware_inputs++;
    }
    printf("\n");

    signal_index++;
  }

  num_signals = signal_index;

  printf("Parsed %d signals, %d total hardware inputs\n", num_signals,
         req_hardware_inputs);
}
// Debug function to print the current mapping
void print_input_mapping() {
  printf("  === Input Mapping ===\n");
  for (int i = 0; i < num_signals; i++) {
    printf("  Signal %d (%s): ", i,
           signal_info[i].num_channels == 1 ? "mono" : "stereo");

    for (int j = 0; j < num_input_mappings; j++) {
      if (input_mappings[j].signal_index == i) {
        printf("hw%d->ch%d ", input_mappings[j].hardware_input,
               input_mappings[j].signal_channel);
      }
    }
    printf("\n");
  }
  printf("  ====================\n\n");
}

void validate_in_layout(struct SoundIoDevice *device,
                        struct SoundIoChannelLayout *in_layout) {

  soundio_device_sort_channel_layouts(device);
  *in_layout = *soundio_best_matching_channel_layout(
      device->layouts, device->layout_count, device->layouts,
      device->layout_count);
}

int num_hardware_outputs = 2;
int output_channels[8] = {0, 1};
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

void print_routing_setup(struct SoundIoOutStream *outstream,
                         struct SoundIoInStream *instream,
                         struct SoundIoRingBuffer *ring_buffer) {

  fprintf(stderr, ANSI_COLOR_BLUE);

  int channel_bytes_per_frame =
      req_hardware_inputs * outstream->bytes_per_sample;
  fprintf(stderr, "Format: %s\n", soundio_format_string(outstream->format));
  fprintf(stderr, "Sample rate: %d Hz\n", outstream->sample_rate);
  if (ring_buffer) {
    fprintf(stderr, "Buffer size: %.2f ms\n",
            (sample_t)soundio_ring_buffer_capacity(ring_buffer) /
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
