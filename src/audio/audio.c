#include "util.c"
#include <math.h>
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static const double PI = 3.14159265358979323846264338328;
static double seconds_offset = 0.0;
static double pitch = 440.0;
static double pitches[6] = {261.626, 311.127, 349.228,
                            391.995, 466.164, 523.251};
void sleep_millisecs(long msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, &ts);
}
void *modulate_pitch(void *arg) {
  for (;;) {
    int rand_int = rand() % 6;
    double p = pitches[rand_int];
    pitch = p * (rand() % 2 ? 1.0 : 0.25);
    long msec = 250 * ((long)(rand() % 4) + 1);
    sleep_millisecs(msec);
  }
}

typedef struct Node {
  struct Node *next;
  void (*perform)(double *out, int frame_count, double seconds_per_frame);
} Node;


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

void perform(double *out, int frame_count, double seconds_per_frame) {
  perform_sq_detune(out, frame_count, seconds_per_frame);
  perform_tanh(out, frame_count, seconds_per_frame);

}

Node get_graph() {
  Node tanh_node = {
    .perform = perform_tanh
  };
  Node sq_node = {
    .perform = perform_sq_detune,
    .next = &tanh_node
  };
  return sq_node;
}

void perform_graph(Node *graph, double *out, int frame_count, double seconds_per_frame) {
  graph->perform(out, frame_count, seconds_per_frame);
  if (graph->next) {
    perform_graph(graph->next, out, frame_count, seconds_per_frame);
  };
}

static void (*write_sample)(char *ptr, double sample);
void write_buffer_to_output(double *buffer, int frame_count,
                            const struct SoundIoChannelLayout *layout,
                            struct SoundIoChannelArea *areas) {
  for (int frame = 0; frame < frame_count; frame += 1) {
    double sample = 0.25 * buffer[frame];
    for (int channel = 0; channel < layout->channel_count; channel += 1) {
      write_sample(areas[channel].ptr, sample);
      areas[channel].ptr += areas[channel].step;
    }
  }
}
static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max) {
  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;
  struct SoundIoChannelArea *areas;
  Node *graph = (Node *)outstream->userdata;
  


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

    perform_graph(graph, out, frame_count, seconds_per_frame);

    write_buffer_to_output(out, frame_count, layout, areas);

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
