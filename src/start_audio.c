#include "start_audio.h"
#include "audio/sq.h"
#include "audio/sin.h"
#include "audio/out.h"
#include "ctx.h"
#include <math.h>


static void add_squares() {
  Node *sq = sin_node(200);
  Node *out = replace_out(
      NODE_DATA(sq_data, sq)->out,
      ctx.out_chans[0].data
    );

  ctx.head = sq;
  sq->next = out;
}

static struct SoundIo *soundio = NULL;
static struct SoundIoDevice *device = NULL;
static struct SoundIoOutStream *outstream = NULL;

static void _write_sample_float32ne(char *ptr, double sample) {
  float *buf = (float *)ptr;
  *buf = sample;
}
static void (*write_sample)(char *ptr, double sample);
static volatile bool want_pause = false;
static void _write_callback(struct SoundIoOutStream *outstream,
                            int frame_count_min, int frame_count_max) {
  double float_sample_rate = outstream->sample_rate;
  double seconds_per_frame = 1.0 / float_sample_rate;

  struct SoundIoChannelArea *areas;
  Ctx *ctx = outstream->userdata;
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

    user_ctx_callback(ctx, frame_count, seconds_per_frame);

    for (int out_chan = 0; out_chan < OUTPUT_CHANNELS; out_chan++) {
      Channel chan = ctx->out_chans[out_chan];

      if (!chan.mute) {
        for (int frame = 0; frame < frame_count; frame += 1) {
          for (int layout_chan = 0; layout_chan < layout->channel_count;
               layout_chan += 1) {
            write_sample(areas[layout_chan].ptr,
                         ctx->main_vol * chan.data[frame + layout_chan]
                         /* ctx->main_vol * channel_read_destructive(out_chan, layout_chan, frame) */
                         );


           areas[layout_chan].ptr += areas[layout_chan].step;
          }
        }
      }
    }
    ctx->sys_time += seconds_per_frame * frame_count;

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

  soundio_outstream_pause(outstream, want_pause);
}

static void underflow_callback(struct SoundIoOutStream *outstream) {
  static int count = 0;
  fprintf(stderr, "underflow %d\n", count++);
}
int setup_audio() {
  enum SoundIoBackend backend = SoundIoBackendNone;
  char *device_id = NULL;
  bool raw = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  char *filename = NULL;

  soundio = soundio_create();

  if (!soundio) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, backend);

  if (err) {
    fprintf(stderr, "Unable to connect to backend: %s\n",
            soundio_strerror(err));
    return 1;
  }
  printf(ANSI_COLOR_MAGENTA "Simple Synth" ANSI_COLOR_RESET "\n");

  fprintf(stderr, "Backend: %s\n",
          soundio_backend_name(soundio->current_backend));

  soundio_flush_events(soundio);

  int selected_device_index = -1;
  if (device_id) {
    int device_count = soundio_output_device_count(soundio);
    for (int i = 0; i < device_count; i += 1) {
      device = soundio_get_output_device(soundio, i);
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
  }

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

  fprintf(stderr, "Output device: %s\n", device->name);

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

  init_ctx();
  /* add_squares(); */

  outstream->userdata = &ctx;
  outstream->write_callback = _write_callback;

  outstream->underflow_callback = underflow_callback;
  outstream->name = stream_name;
  outstream->software_latency = latency;
  outstream->sample_rate = sample_rate;
  write_sample = _write_sample_float32ne;

  if ((err = soundio_outstream_open(outstream))) {
    fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  fprintf(stderr, "Software latency: %f\n", outstream->software_latency);

  if (outstream->layout_error)
    fprintf(stderr, "unable to set channel layout: %s\n",
            soundio_strerror(outstream->layout_error));

  if ((err = soundio_outstream_start(outstream))) {
    fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
    return 1;
  }
  return 0;
}

int stop_audio() {
  free(ctx.head);
  ctx.head = NULL;
  /* soundio_outstream_destroy(outstream); */
  /* soundio_device_unref(device); */
  /* soundio_destroy(soundio); */
  return 0;
}
