#include "./audio_routing.h"
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
int total_hardware_inputs = 0;

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
  total_hardware_inputs = 0;

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
      total_hardware_inputs++;
    }
    printf("\n");

    signal_index++;
  }

  num_signals = signal_index;

  printf("Parsed %d signals, %d total hardware inputs\n", num_signals,
         total_hardware_inputs);
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
