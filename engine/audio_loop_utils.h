#ifndef _ENGINE_AUDIO_LOOP_UTILS_H
#define _ENGINE_AUDIO_LOOP_UTILS_H

#include "ctx.h"
#include <soundio/soundio.h>

void write_sample_s16ne(char *ptr, double sample);
void write_sample_s32ne(char *ptr, double sample);
void write_sample_float32ne(char *ptr, double sample);
void add_sample_float32ne_w_offset(char *ptr, int offset, double sample);
void write_sample_float64ne(char *ptr, double sample);

void set_out_format(struct SoundIoDevice *device,
                    struct SoundIoOutStream *outstream, void **write_sample);

int get_out_device(char *device_id, struct SoundIo *soundio,
                   struct SoundIoDevice **device);

int get_in_device(char *in_device_id, struct SoundIo *soundio,
                  struct SoundIoDevice **device);
int start_outstream(struct SoundIoOutStream *outstream);

void list_audio_devices(struct SoundIo *soundio);

// Input routing configuration
typedef struct InputRoutingConfig {
  int num_input_channels;
  int *hardware_inputs;
  int *input_channel_layouts;
} InputRoutingConfig;

int init_input_signals(Ctx *ctx, InputRoutingConfig config, int buffer_size);
#endif
