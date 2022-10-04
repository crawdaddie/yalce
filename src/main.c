#include "audio/node.h"
#include "audio/util.c"
#include "cli.c"
#include "oscilloscope.c"
#include "user_ctx.c"
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#define PROJECT_NAME "simple-synth"

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
void cleanup_graph(Node *node, Node *prev) {
  if (node->should_free) {
    prev->next = node->next;
    node->free_node(node);
    return cleanup_graph(prev->next, NULL);
  };
  Node *next = node->next;
  if (next) {
    cleanup_graph(next, node);
  }
}
void *cleanup_nodes_job(void *arg) {
  UserCtx *ctx = (UserCtx *)arg;
  Node *graph = ctx->graph;
  for (;;) {
    /* cleanup_graph(graph, NULL); */
    printf("----------ts %f\n", seconds_offset);
    /* debug_graph(graph); */
    sleep_millisecs(250);
  }
}
void *modulate_pitch(void *arg) {
  int p_index = 0;

  for (;;) {
    struct SoundIoOutStream *outstream = (struct SoundIoOutStream *)arg;
    UserCtx *ctx = (UserCtx *)outstream->userdata;
    int rand_int = rand() % 7;
    double p = pitches[p_index];
    /* int rand_octave = rand() % 4; */
    /* p = p * 0.5 * octaves[rand_octave]; */

    ctx_play_synth(ctx, p);
    /* graph = play_synth(graph, p); */
    /* debug_node(graph, "playing"); */
    /* debug_node(graph->next, "before"); */

    long msec = 500 * ((long)(rand() % 4) + 1);
    sleep_millisecs(msec);
    p_index = (p_index + 1) % 7;
  }
}
static void (*write_sample)(char *ptr, double sample);
void write_buffer_to_output(double *buffer, int frame_count,
                            const struct SoundIoChannelLayout *layout,
                            struct SoundIoChannelArea *areas) {
  if (!buffer) {
    fprintf(stderr, "no buffer found to write to output");
    return;
  };
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

  UserCtx *ctx = (UserCtx *)outstream->userdata;

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
    zero_bus(get_bus(ctx, 0), frame_count, seconds_per_frame, seconds_offset);

    Node *node = perform_graph(ctx->graph, frame_count, seconds_per_frame,
                               seconds_offset);

    write_buffer_to_output(get_bus(ctx, 0), frame_count, layout, areas);

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

int get_output_device_index(char *device_id, struct SoundIo *soundio,
                            bool raw) {
  int selected_device_index = -1;
  if (device_id) {
    int device_count = soundio_output_device_count(soundio);
    for (int i = 0; i < device_count; i += 1) {
      struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
      bool select_this_one =
          strcmp(device->id, device_id) == 0 && device->is_raw == raw;
      soundio_device_unref(device);
      if (select_this_one) {
        selected_device_index = i;
        break;
      }
    }
  } else {
    selected_device_index = soundio_default_output_device_index(soundio);
  };
  return selected_device_index;
}

static void (*write_sample)(char *ptr, double sample);
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

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", count++);
}

int main(int argc, char **argv) {
  enum SoundIoBackend *backend = NULL;
  char *device_id = NULL;
  bool raw = false;
  char *stream_name = NULL;

  ss_setup(argc, argv, stream_name, device_id, backend);

  struct SoundIo *soundio = soundio_create();
  if (!soundio) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, *backend);

  if (err) {
    fprintf(stderr, "Unable to connect to backend: %s\n",
            soundio_strerror(err));
    return 1;
  }

  fprintf(stderr, "Backend: %s\n",
          soundio_backend_name(soundio->current_backend));

  soundio_flush_events(soundio);
  int selected_device_index = get_output_device_index(device_id, soundio, raw);
  if (selected_device_index < 0) {
    fprintf(stderr, "Output device not found\n");
    return 1;
  }

  struct SoundIoDevice *device =
      soundio_get_output_device(soundio, selected_device_index);
  if (!device) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  if (device->probe_error) {
    fprintf(stderr, "Cannot probe device: %s\n",
            soundio_strerror(device->probe_error));
    return 1;
  }

  struct SoundIoOutStream *outstream = soundio_outstream_create(device);
  if (!outstream) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  double latency = 0.0;
  int sample_rate = 0;

  outstream->write_callback = write_callback;
  outstream->underflow_callback = underflow_callback;
  outstream->name = stream_name;
  outstream->software_latency = latency;
  outstream->sample_rate = sample_rate;

  if ((err = set_output_format(outstream, device))) {
    return 1;
  };

  if ((err = soundio_outstream_open(outstream))) {
    fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
    return 1;
  };
  fprintf(stderr, "Software latency: %f\n", outstream->software_latency);

  if (outstream->layout_error)
    fprintf(stderr, "unable to set channel layout: %s\n",
            soundio_strerror(outstream->layout_error));

  if ((err = soundio_outstream_start(outstream))) {
    fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
    return 1;
  };

  UserCtx *ctx = get_user_ctx();
  outstream->userdata = ctx;

  pthread_t thread;
  pthread_create(&thread, NULL, modulate_pitch, (void *)outstream);

  pthread_t cleanup_thread;
  pthread_create(&thread, NULL, cleanup_nodes_job, (void *)ctx);

  pthread_t win_thread;
  pthread_create(&win_thread, NULL, (void *)win, (void *)ctx);

  pthread_exit(NULL);
  soundio_outstream_destroy(outstream);
  soundio_device_unref(device);
  soundio_destroy(soundio);

  return 0;
}
