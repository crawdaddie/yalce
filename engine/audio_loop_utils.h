#ifndef _ENGINE_AUDIO_LOOP_UTILS_H
#define _ENGINE_AUDIO_LOOP_UTILS_H

#include <soundio/soundio.h>

void write_sample_s16ne(char *ptr, double sample);
void write_sample_s32ne(char *ptr, double sample);
void write_sample_float32ne(char *ptr, double sample);
void add_sample_float32ne_w_offset(char *ptr, int offset, double sample);
void write_sample_float64ne(char *ptr, double sample);

void set_out_format(struct SoundIoDevice *device,
                    struct SoundIoOutStream *outstream, void **write_sample);

void validate_in_layout(struct SoundIoDevice *device,
                        struct SoundIoChannelLayout *in_layout);
#endif
