#include "./audio_loop_utils.h"
#include <stdint.h>
#include <stdio.h>
void write_sample_s16ne(char *ptr, sample_t sample) {
  int16_t *buf = (int16_t *)ptr;
  sample_t range = (sample_t)INT16_MAX - (sample_t)INT16_MIN;
  sample_t val = sample * range / 2.0;
  *buf = val;
}

void write_sample_s32ne(char *ptr, sample_t sample) {
  int32_t *buf = (int32_t *)ptr;
  sample_t range = (sample_t)INT32_MAX - (sample_t)INT32_MIN;
  sample_t val = sample * range / 2.0;
  *buf = val;
}

void write_sample_float32ne(char *ptr, sample_t sample) {
  float *buf = (float *)ptr;
  *buf = sample;
}

void add_sample_float32ne_w_offset(char *ptr, int offset, sample_t sample) {
  float *buf = (float *)(ptr + offset);
  *buf += sample;
}

void write_sample_float64ne(char *ptr, sample_t sample) {
  sample_t *buf = (sample_t *)ptr;
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
