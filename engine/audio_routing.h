#ifndef _ENGINE_AUDIO_ROUTING_H
#define _ENGINE_AUDIO_ROUTING_H

#include <soundio/soundio.h>

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
extern int req_hardware_inputs;

void parse_input_config(int *config, int config_size);

void print_input_mapping();

extern int num_hardware_outputs;
extern int output_channels[8];

void validate_in_layout(struct SoundIoDevice *device,
                        struct SoundIoChannelLayout *in_layout);

void validate_out_layout(struct SoundIoDevice *out_device,
                         struct SoundIoChannelLayout *_out_layout);

void print_routing_setup(struct SoundIoOutStream *outstream,
                         struct SoundIoInStream *instream,
                         struct SoundIoRingBuffer *ring_buffer);
#endif
