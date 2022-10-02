#include "audio/audio.c"
#include "cli.c"
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#define PROJECT_NAME "simple-synth"

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

  sq_data data = {.freq = 220.0, ._prev_freq = 220.0};
  tanh_data tanh_data = {.gain = 9.0};
  lp_data lp_data = {.cutoff = 1000.0, .resonance = 0.5};
  add_graph_to_stream(outstream, &data, &tanh_data, &lp_data);

  outstream->write_callback = write_callback;
  outstream->underflow_callback = underflow_callback;
  outstream->name = stream_name;
  outstream->software_latency = latency;
  outstream->sample_rate = sample_rate;

  pthread_t thread;
  pthread_create(&thread, NULL, modulate_pitch, (void *)&data);

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

  pthread_exit(NULL);

  soundio_outstream_destroy(outstream);
  soundio_device_unref(device);
  soundio_destroy(soundio);

  return 0;
}
