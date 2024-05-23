#include "audio_loop.h"
#include "ctx.h"
#include "dbg.h"
#include "gc.h"
#include "log.h"
#include "oscillator.h"
#include "scheduling.h"
#include "write_sample.h"
#include <soundio/soundio.h>
#include <string.h>
#include <time.h>

static struct SoundIo *soundio = NULL;
static struct SoundIoDevice *device = NULL;
static struct SoundIoOutStream *outstream = NULL;

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
      printf("unrecoverable stream error: %s\n", soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;

    const struct SoundIoChannelLayout *layout = &outstream->layout;

    // size_t len_msgs = queue_size(ctx->queue);

    // for (int i = 0; i < len_msgs; i++) {
    //   Msg msg = queue_pop_left(&ctx->queue);
    //
    //   if (msg.handler != NULL) {
    //     int block_offset = get_msg_block_offset(msg, *ctx,
    //     float_sample_rate);
    //
    //     msg.handler(ctx, msg, block_offset);
    //   }
    // }

    set_block_time();
    // handle_scheduler_tick();
    user_ctx_callback(ctx, frame_count, seconds_per_frame);

    int sample_idx;
    double sample;
    Signal DAC = ctx->dac_buffer;

    for (int channel = 0; channel < layout->channel_count; channel += 1) {
      for (int frame = 0; frame < frame_count; frame += 1) {

        sample_idx = DAC.layout * frame + channel;
        sample = DAC.buf[sample_idx];

        write_sample(areas[channel].ptr, ctx->main_vol * sample);
        areas[channel].ptr += areas[channel].step;
      }
    }

    // ctx->block_time = get_time();

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      printf("unrecoverable stream error: %s\n", soundio_strerror(err));
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
  printf("underflow %d\n", count++);
}
int start_audio() {
  enum SoundIoBackend backend = SoundIoBackendNone;
  char *device_id = NULL;
  // char *device_id = "BuiltInSpeakerDevice";

  bool raw = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  char *filename = NULL;
  printf("------------------\n");

  soundio = soundio_create();

  if (!soundio) {
    printf("out of memory\n");
    return 1;
  }

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, backend);

  if (err) {
    printf("Unable to connect to backend: %s\n", soundio_strerror(err));
    return 1;
  }
  printf(ANSI_COLOR_MAGENTA "Simple Synth" ANSI_COLOR_RESET "\n");
  printf("Backend:           %s\n",
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
    printf("Output device not found\n");
    return 1;
  }

  struct SoundIoDevice *device =
      soundio_get_output_device(soundio, selected_device_index);
  if (!device) {
    printf("out of memory\n");
    return 1;
  }

  printf("Output device:     %s\n", device->name);
  // printf("Output device ID:  %s\n", device->id);

  if (device->probe_error) {
    printf("Cannot probe device: %s\n", soundio_strerror(device->probe_error));
    return 1;
  }

  struct SoundIoOutStream *outstream = soundio_outstream_create(device);
  if (!outstream) {
    printf("out of memory\n");
    return 1;
  }

  init_ctx();

  outstream->userdata = &ctx;
  outstream->write_callback = _write_callback;

  outstream->underflow_callback = underflow_callback;
  outstream->name = stream_name;
  outstream->software_latency = latency;
  outstream->sample_rate = sample_rate;

  if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
    outstream->format = SoundIoFormatFloat32NE;
    write_sample = write_sample_float32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {

    outstream->format = SoundIoFormatFloat64NE;
    write_sample = write_sample_float64ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {

    outstream->format = SoundIoFormatS32NE;
    write_sample = write_sample_s32ne;
  } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {

    outstream->format = SoundIoFormatS16NE;
    write_sample = write_sample_s16ne;
  } else {
    printf("No suitable device format available.\n");
    return 1;
  }

  if ((err = soundio_outstream_open(outstream))) {
    printf("unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  if (outstream->layout_error)
    printf("unable to set channel layout: %s\n",
           soundio_strerror(outstream->layout_error));

  if ((err = soundio_outstream_start(outstream))) {

    printf("unable to start device: %s\n", soundio_strerror(err));

    return 1;
  }
  printf("Software latency:  %f\n", outstream->software_latency);
  printf("Sample rate:       %d\n", outstream->sample_rate);
  ctx.sample_rate = outstream->sample_rate;
  printf("------------------\n");

  set_block_time();
  return 0;
}

int stop_audio() {
  // free(ctx.head);
  // ctx.head = NULL;
  soundio_outstream_destroy(outstream);
  soundio_device_unref(device);
  soundio_destroy(soundio);

  // logging_teardown();
  return 0;
}

int init_audio() {
  maketable_sin();
  maketable_sq();
  start_audio();
  init_scheduling();
  cleanup_job();
}
