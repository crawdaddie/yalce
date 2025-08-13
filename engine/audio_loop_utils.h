#ifndef _ENGINE_AUDIO_LOOP_UTILS_H
#define _ENGINE_AUDIO_LOOP_UTILS_H

#include "./common.h"
#include <soundio/soundio.h>

void write_sample_s16ne(char *ptr, sample_t sample);
void write_sample_s32ne(char *ptr, sample_t sample);
void write_sample_float32ne(char *ptr, sample_t sample);
void add_sample_float32ne_w_offset(char *ptr, int offset, sample_t sample);
void write_sample_float64ne(char *ptr, sample_t sample);

void set_out_format(struct SoundIoDevice *device,
                    struct SoundIoOutStream *outstream, void **write_sample);

#endif
