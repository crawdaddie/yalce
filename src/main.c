#include "audio/signal.h"
#include "callback.c"
#include "ctx.h"
#include <math.h>
#include <soundio/soundio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #include "./prog.c" */
#include "graph/graph.h"
#include <stdlib.h>

#include <getopt.h>

#include "bindings.h"
#include "lang/vm.h"
#include "lang_runner.h"
#include <pthread.h>
static int usage(char *exe) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "Options:\n"
          "  [--backend dummy|alsa|pulseaudio|jack|coreaudio|wasapi]\n"
          "  [--device id]\n"
          "  [--raw]\n"
          "  [--name stream_name]\n"
          "  [--latency seconds]\n"
          "  [--sample-rate hz]\n"
          "  [--oscilloscope]\n",
          exe);
  return 1;
}

static int process_opts(int argc, char **argv, char **device_id,
                        int *sample_rate, enum SoundIoBackend *backend,
                        bool *raw, bool *oscilloscope, char **stream_name,
                        double *latency, char **filename) {
  int c;

  while (1) {
    static struct option long_options[] = {
        {"backend", required_argument, 0, 'b'},
        {"device", required_argument, 0, 'd'},
        {"sample-rate", required_argument, 0, 's'},
        {"latency", required_argument, 0, 'l'},
        {"name", required_argument, 0, 'n'},
        {"oscilloscope", no_argument, 0, 'o'},
        {"r", no_argument, 0, 'r'},
        {0, 0, 0, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "b:d:s:l:n:o:r:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;

    case 'b': {

      if (strcmp(optarg, "dummy") == 0) {
        *backend = SoundIoBackendDummy;
      } else if (strcmp(optarg, "alsa") == 0) {
        *backend = SoundIoBackendAlsa;
      } else if (strcmp(optarg, "pulseaudio") == 0) {
        *backend = SoundIoBackendPulseAudio;
      } else if (strcmp(optarg, "jack") == 0) {
        *backend = SoundIoBackendJack;
      } else if (strcmp(optarg, "coreaudio") == 0) {
        *backend = SoundIoBackendCoreAudio;
      } else if (strcmp(optarg, "wasapi") == 0) {
        *backend = SoundIoBackendWasapi;
      } else {
        fprintf(stderr, "Invalid backend: %s\n", optarg);
        return 0;
      }
      break;
    }

    case 'd':
      *device_id = optarg;
      break;

    case 's':
      *sample_rate = atoi(optarg);
      break;

    case 'l':
      *latency = atof(optarg);
      break;

    case 'n':
      *stream_name = optarg;
      break;

    case 'o':
      *oscilloscope = true;
      break;

    case 'r':
      *raw = true;
      break;

    case '?':
      /* getopt_long already printed an error message. */
      break;

    default:
      return 0;
    }
  }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    *filename = argv[argc - 1];
  }
  return 1;
}

int main(int argc, char **argv) {
  char *exe = argv[0];
  enum SoundIoBackend backend = SoundIoBackendNone;
  char *device_id = NULL;
  bool raw = false;
  bool oscilloscope = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  char *filename = NULL;
  if (!process_opts(argc, argv, &device_id, &sample_rate, &backend, &raw,
                    &oscilloscope, &stream_name, &latency, &filename)) {
    usage(exe);
  }
  init_vm();

  struct SoundIo *soundio = soundio_create();
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

  fprintf(stderr, "Backend: %s\n",
          soundio_backend_name(soundio->current_backend));

  soundio_flush_events(soundio);

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

  init_user_ctx();
  outstream->write_callback = write_callback;
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
    fprintf(stderr, "No suitable device format available.\n");
    return 1;
  }

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

  printf("--------------\n");
  if (filename) {
    printf("loading file %s\n", filename);
    run_file(filename);
  }

  char input[2048];
  for (;;) {
    printf("> ");
    soundio_flush_events(soundio);
    fgets(input, 2048, stdin);
    interpret(input);
  }

  soundio_outstream_destroy(outstream);
  soundio_device_unref(device);
  soundio_destroy(soundio);
  return 0;
}
