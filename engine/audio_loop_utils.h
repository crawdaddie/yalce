#ifndef _ENGINE_AUDIO_LOOP_UTILS_H
#define _ENGINE_AUDIO_LOOP_UTILS_H

#include <soundio/soundio.h>

typedef void (*write_sample_func_t)(char *ptr, double sample);

void write_sample_s16ne(char *ptr, double sample);
void write_sample_s32ne(char *ptr, double sample);
void write_sample_float32ne(char *ptr, double sample);
void add_sample_float32ne_w_offset(char *ptr, int offset, double sample);
void write_sample_float64ne(char *ptr, double sample);

void set_out_format(struct SoundIoDevice *device,
                    struct SoundIoOutStream *outstream,
                    write_sample_func_t *write_sample);

#endif
