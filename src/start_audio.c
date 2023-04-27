#include "start_audio.h"
#include "audio/osc.h"
#include "audio/out.h"
#include "ctx.h"
#include "log.h"
#include "oscilloscope.h"
#include "scheduling.h"
#include "write_sample.h"
#include <math.h>

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
      write_log("unrecoverable stream error: %s\n", soundio_strerror(err));
      exit(1);
    }

    if (!frame_count)
      break;

    const struct SoundIoChannelLayout *layout = &outstream->layout;

    size_t len_msgs = queue_size(ctx->queue);
    // write_log("len msgs %d\n", len_msgs);

    for (int i = 0; i < len_msgs; i++) {
      Msg msg = queue_pop_left(&ctx->queue);

      if (msg.handler != NULL) {
        int block_offset = get_msg_block_offset(msg, *ctx, float_sample_rate);

        msg.handler(ctx, msg, block_offset);
      }
    }

    user_ctx_callback(ctx, frame_count, seconds_per_frame);

    int sample_idx;
    double sample;
    Signal DAC = ctx->DAC;

    for (int channel = 0; channel < layout->channel_count; channel += 1) {
      for (int frame = 0; frame < frame_count; frame += 1) {

        sample_idx = SAMPLE_IDX(DAC, frame, channel);
        sample = DAC.data[sample_idx];

        write_sample(areas[channel].ptr, ctx->main_vol * sample);
        add_osc_scope_buf(frame, sample / OUTPUT_CHANNELS);

        DAC.data[sample_idx] = 0.0; // zero channel buffer after reading from it
        areas[channel].ptr += areas[channel].step;
      }
    }

    ctx->sys_time = ctx->sys_time + (seconds_per_frame * frame_count);
    ctx->block_time = get_time();

    if ((err = soundio_outstream_end_write(outstream))) {
      if (err == SoundIoErrorUnderflow)
        return;
      write_log("unrecoverable stream error: %s\n", soundio_strerror(err));
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
  write_log("underflow %d\n", count++);
}
int setup_audio() {
  logging_setup();
  enum SoundIoBackend backend = SoundIoBackendNone;
  char *device_id = NULL;
  bool raw = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  char *filename = NULL;
  write_log("------------------\n");

  soundio = soundio_create();

  if (!soundio) {
    write_log("out of memory\n");
    return 1;
  }

  int err = (backend == SoundIoBackendNone)
                ? soundio_connect(soundio)
                : soundio_connect_backend(soundio, backend);

  if (err) {
    write_log("Unable to connect to backend: %s\n", soundio_strerror(err));
    return 1;
  }
  write_log(ANSI_COLOR_MAGENTA "Simple Synth" ANSI_COLOR_RESET "\n");

  write_log("Backend: %s\n", soundio_backend_name(soundio->current_backend));

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
    write_log("Output device not found\n");
    return 1;
  }

  struct SoundIoDevice *device =
      soundio_get_output_device(soundio, selected_device_index);
  if (!device) {
    write_log("out of memory\n");
    return 1;
  }

  write_log("Output device: %s\n", device->name);

  if (device->probe_error) {
    write_log("Cannot probe device: %s\n",
              soundio_strerror(device->probe_error));
    return 1;
  }

  struct SoundIoOutStream *outstream = soundio_outstream_create(device);
  if (!outstream) {
    write_log("out of memory\n");
    return 1;
  }

  init_ctx();
  init_scheduling();

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
    write_log("No suitable device format available.\n");
    return 1;
  }

  if ((err = soundio_outstream_open(outstream))) {
    write_log("unable to open device: %s", soundio_strerror(err));
    return 1;
  }

  write_log("Software latency: %f\n", outstream->software_latency);

  if (outstream->layout_error)
    write_log("unable to set channel layout: %s\n",
              soundio_strerror(outstream->layout_error));

  if ((err = soundio_outstream_start(outstream))) {

    write_log("unable to start device: %s\n", soundio_strerror(err));

    return 1;
  }
  /* oscilloscope(); */

  write_log("------------------\n");
  return 0;
}

int stop_audio() {
  free(ctx.head);
  ctx.head = NULL;
  /* soundio_outstream_destroy(outstream); */
  /* soundio_device_unref(device); */
  /* soundio_destroy(soundio); */
  logging_teardown();
  return 0;
}
