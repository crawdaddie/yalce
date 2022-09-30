#include "node.h"
#include "util.c"
#include <math.h>
#include <soundio/soundio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double seconds_offset = 0.0;

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", count++);
}

void add_to_graph(struct Node *node, struct Node new_node) {
  node->next = &new_node;
}
double randfrom(double min, double max) {
  double range = (max - min);
  double div = RAND_MAX / range;
  return min + (rand() / div);
}

void sin_perform(double *outs, int sample_frames) {
  int n = sample_frames;
  while (n--) {
    *outs++ = randfrom(-1.0, 1.0);
  }
}

void tanh_perform(double **outs, int sample_frames) {
  int n = sample_frames;
  int i = 0;
  for (int i = 0; i < n; i++) {
    double value = **(outs + i);
    **outs++ = tanh(value * 1.1);
  }
}

static void (*write_sample)(char *ptr, double sample);
static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {
  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;
  struct SoundIoChannelArea *areas;

  /* struct Node *node = (struct Node *)outstream->userdata; */

  int err;

  int frames_left = frame_count_max;

  for (;;) {
    int frame_count = frames_left;
    if ((err =
             soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      fprintf(stderr, "unrecoverable stream error: %s\n",
              soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;
    double buffer[frame_count];

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    double *out = &buffer[0];

    sin_perform(out, frame_count);

    for (int frame = 0; frame < frame_count; frame += 1) {
      for (int channel = 0; channel < layout->channel_count; channel += 1) {
        write_sample(areas[channel].ptr, out[frame]);
        areas[channel].ptr += areas[channel].step;
      }
    }
    /* seconds_offset = */
    /*     fmod(seconds_offset + seconds_per_frame * frame_count, 1.0); */

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      fprintf(stderr, "unrecoverable stream error: %s\n",
              soundio_strerror(err));
      exit(1);
    }

    frames_left -= frame_count;
    if (frames_left <= 0)
      break;
  }
}

int set_output_format(struct SoundIoOutStream *outstream,
                      struct SoundIoDevice *device) {
  if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
    outstream->format = SoundIoFormatFloat32NE;
    write_sample = write_sample_float32ne;
    printf("outstream-format: float32ne\n");
  } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
    outstream->format = SoundIoFormatFloat64NE;
    write_sample = write_sample_float64ne;

    printf("outstream-format: float64ne\n");
  } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {
    outstream->format = SoundIoFormatS32NE;
    write_sample = write_sample_s32ne;

    printf("outstream-format: s32ne\n");
  } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
    outstream->format = SoundIoFormatS16NE;
    write_sample = write_sample_s16ne;
    printf("outstream-format: s16ne\n");
  } else {
    fprintf(stderr, "No suitable device format available.\n");
    return 1;
  };
  return 0;
}
