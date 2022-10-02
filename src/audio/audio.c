#include "node.h"
#include "util.c"
#include <math.h>
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static const double PI = 3.14159265358979323846264338328;
static double pitches[6] = {261.626, 311.127, 349.228,
                            391.995, 466.164, 523.251};

void sleep_millisecs(long msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, &ts);
}
void *modulate_pitch(void *arg) {
  sq_data *data = (sq_data *)arg;
  for (;;) {
    int rand_int = rand() % 6;
    double p = pitches[rand_int];
    p = p * (rand() % 2 ? 1.0 : 0.25);
    data->freq = p;
    debug_sq(data);

    long msec = 250 * ((long)(rand() % 4) + 1);
    sleep_millisecs(msec);
  }
}

Node *get_graph(sq_data *sq_data, tanh_data *tanh_data, lp_data *lp_data) {
  Node *head = get_sq_detune_node(sq_data);
  Node *tanh = get_tanh_node(tanh_data);
  /* Node *lp = get_lp_node(); */
  head->next = tanh;
  /* tanh->next = lp; */
  return head;
}
void add_graph_to_stream(struct SoundIoOutStream *outstream, sq_data *data,
                         tanh_data *tanh_data, lp_data *lp_data) {
  Node *graph = get_graph(data, tanh_data, lp_data);
  outstream->userdata = graph;
}

void perform_graph(Node *graph, double *out, int frame_count,
                   double seconds_per_frame, double seconds_offset) {
  Node *node = graph;

  node->perform(node, out, frame_count, seconds_per_frame, seconds_offset);
  if (node->next) {
    perform_graph(node->next, out, frame_count, seconds_per_frame,
                  seconds_offset);
  }
}
static double seconds_offset = 0.0;
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

    perform_graph(graph, out, frame_count, seconds_per_frame, seconds_offset);

    write_buffer_to_output(out, frame_count, layout, areas);

    seconds_offset = seconds_offset + seconds_per_frame * frame_count;
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
