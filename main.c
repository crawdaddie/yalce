#include "audio/audio.c"
#include "audio/node.h"
#include "cli.c"
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#define PROJECT_NAME "asimp"

void input_to_note(char input, struct Node *data) {
  switch (input) {
  case 'z':
    data->args[0] = 261.626;
    break;
  case 's':
    data->args[0] = 277.183;
    break;
  case 'x':
    data->args[0] = 293.665;
    break;
  case 'd':
    data->args[0] = 311.127;
    break;
  case 'c':
    data->args[0] = 329.628;
    break;
  case 'v':
    data->args[0] = 349.228;
    break;
  case 'g':
    data->args[0] = 369.994;
    break;
  case 'b':
    data->args[0] = 391.995;
    break;
  case 'h':
    data->args[0] = 415.305;
    break;
  case 'n':
    data->args[0] = 440.0;
    break;
  case 'j':
    data->args[0] = 466.164;
    break;
  case 'm':
    data->args[0] = 493.883;
    break;
  case ',':
    data->args[0] = 523.251;
    break;
  default:
    data->args[0] = 440.0;
  }
}

void *input_thread(void *arg) {
  struct Node *data = (struct Node *)arg;
  char input;
  while (1) {
    scanf("%c", &input);
    input_to_note(input, data);
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

int main(int argc, char **argv) {
  enum SoundIoBackend *backend = NULL;
  char *device_id = NULL;
  bool raw = false;
  char *stream_name = NULL;

  asimp_setup(argc, argv, stream_name, device_id, backend);

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

  /* double sin_args[2]; */
  /* sin_args[0] = 440.0; */
  /* sin_args[1] = 1.01; */
  /* double *sin_argsptr = &sin_args[0]; */

  /* struct Node sin = get_sin_node_detune(sin_argsptr); */
  struct Node sin = get_sin_node(440.0);
  struct Node tanh = get_tanh_node(1.01);
  sin.next = &tanh;

  pthread_t thread;
  pthread_create(&thread, NULL, input_thread, (void *)&sin);

  outstream->userdata = &sin;
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

  for (;;) {
    soundio_flush_events(soundio);
  };

  pthread_exit(NULL);

  soundio_outstream_destroy(outstream);
  soundio_device_unref(device);
  soundio_destroy(soundio);

  return 0;
}
