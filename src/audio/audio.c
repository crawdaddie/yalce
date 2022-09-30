#include "util.c"
#include <math.h>
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct Node {
  struct Node *next;
} Node;

static const double PI = 3.14159265358979323846264338328;
static double seconds_offset = 0.0;
static double pitch = 440.0;
static double pitches[6] = {261.626, 311.127, 349.228,
                            391.995, 466.164, 523.251};
void *modulate_pitch(void *arg) {
  for (;;) {
    int rand_int = rand() % 6;
    double p = pitches[rand_int];
    pitch = p * (rand() % 2 ? 1.0 : 0.25);

    struct timespec ts;
    long msec = 250 * ((long)(rand() % 4) + 1);
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
  }
}

void perform_sin(double *out, int frame_count, double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample =
        sin((seconds_offset + i * seconds_per_frame) * radians_per_second);
    out[i] = sample;
  };
}

void perform_sin_detune(double *out, int frame_count,
                        double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample =
        sin((seconds_offset + i * seconds_per_frame) * radians_per_second);
    sample += sin((seconds_offset + i * seconds_per_frame) *
                  radians_per_second * 1.01);
    out[i] = sample * 0.5;
  };
}

void perform_sq(double *out, int frame_count, double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample =
        fmod((seconds_offset + i * seconds_per_frame) * radians_per_second,
             2 * PI) > PI;
    out[i] = (2 * sample - 1);
  };
}

void perform_saw(double *out, int frame_count, double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample = fmod(
        (seconds_offset + i * seconds_per_frame) * radians_per_second, 2 * PI);
    out[i] = 2 * sample - 1;
  };
}

void perform_saw_detune(double *out, int frame_count,
                        double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample = fmod(
        (seconds_offset + i * seconds_per_frame) * radians_per_second, 2 * PI);
    sample += fmod((seconds_offset + i * seconds_per_frame) *
                       radians_per_second * 1.001,
                   2 * PI);

    out[i] = 2 * sample - 1;
  };
}

void perform_sq_detune(double *out, int frame_count, double seconds_per_frame) {
  double radians_per_second = pitch * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample =
        fmod((seconds_offset + i * seconds_per_frame) * radians_per_second,
             2 * PI) > PI;

    sample += fmod((seconds_offset + i * seconds_per_frame) *
                       radians_per_second * 1.02,
                   2 * PI) > PI;

    out[i] = (2 * sample - 1) * 0.5;
  };
}

void perform_tanh(double *out, int frame_count, double seconds_per_frame) {
  for (int i = 0; i < frame_count; i++) {
    double sample = tanh(out[i] * 10.0);
    out[i] = sample;
  };
}

void perform_lp(double *out, int frame_count, double seconds_per_frame) {
  for (int i = 0; i < frame_count; i++) {
    double sample = tanh(out[i] * 10.0);
    out[i] = sample;
  };
}

static void (*write_sample)(char *ptr, double sample);
static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {
  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;
  struct SoundIoChannelArea *areas;
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
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    double buffer[frame_count];
    double *out = &buffer[0];

    perform_sq_detune(out, frame_count, seconds_per_frame);
    perform_tanh(out, frame_count, seconds_per_frame);

    for (int frame = 0; frame < frame_count; frame += 1) {
      double sample = 0.25 * buffer[frame];
      for (int channel = 0; channel < layout->channel_count; channel += 1) {
        write_sample(areas[channel].ptr, sample);
        areas[channel].ptr += areas[channel].step;
      }
    }
    seconds_offset =
        fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);
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
