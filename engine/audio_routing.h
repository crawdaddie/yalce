#ifndef _ENGINE_AUDIO_ROUTING_H
#define _ENGINE_AUDIO_ROUTING_H

#define MAX_INPUTS 16
#define MAX_SIGNALS 16

typedef struct {
  int signal_index;   // Which signal this maps to (0, 1, 2...)
  int signal_channel; // Which channel within that signal (0 for mono, 0/1 for
                      // stereo)
  int hardware_input; // Which hardware input channel
} InputMapping;

typedef struct {
  int num_channels;   // 1 for mono, 2 for stereo
  int start_hw_index; // Index into hardware input array where this signal's
                      // data starts
} SignalInfo;

extern InputMapping input_mappings[MAX_INPUTS];
extern SignalInfo signal_info[MAX_SIGNALS];
extern int num_input_mappings;
extern int num_signals;
extern int total_hardware_inputs;

void parse_input_config(int *config, int config_size);

void print_input_mapping();
#endif
