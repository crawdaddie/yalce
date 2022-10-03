#include "node.h"
#include "synth.c"

#include "util.c"
#include <math.h>
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double seconds_offset = 0.0;
static double pitches[7] = {261.626,         311.127, 349.228, 391.995,
                            415.30469757995, 466.164, 523.251};

static double octaves[4] = {0.25, 0.5, 1, 2.0};
void sleep_millisecs(long msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, &ts);
}
void *modulate_pitch(void *arg) {
  /* Node *graph = (Node *)arg; */

  /* Node *env = graph; */
  /* while (env->name != "env") { */
  /*   env = env->next; */
  /* }; */

  for (;;) {
    int rand_int = rand() % 7;
    double p = pitches[rand_int];
    int rand_octave = rand() % 4;
    p = p * 0.5 * octaves[rand_octave];
    /* set_freq(graph, p); */
    /* if (env) { */
    /*   reset_env(env, seconds_offset * 1000); */
    /* } */

    long msec = 250 * ((long)(rand() % 4) + 1);
    sleep_millisecs(msec);
  }
}

Node *get_graph(struct SoundIoOutStream *outstream) {
  int sample_rate = outstream->sample_rate;
  Node *head = get_synth(outstream);
  Node *tail = head;
  debug_node(tail, NULL);

  tail = node_add_to_tail(
      get_delay_node(tail->out, 750, 1000, 0.3, sample_rate), tail);

  return head;
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
    node_frame_size = frame_count;
    if ((err =
             soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      fprintf(stderr, "unrecoverable stream error: %s\n",
              soundio_strerror(err));
      exit(1);
    }
    if (!frame_count)
      break;

    const struct SoundIoChannelLayout *layout = &outstream->layout;

    Node *outnode =
        perform_graph(graph, frame_count, seconds_per_frame, seconds_offset);
    debug_node(outnode, "cb");

    write_buffer_to_output(outnode->out, frame_count, layout, areas);

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
