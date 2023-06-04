#include "audio/signal.h"
#include "ctx.h"
#include "write_sample.h"
#include <math.h>
#include <soundio/soundio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>

#include "channel.h"
#include "dbg.h"
#include "log.h"
#include "start_audio.h"
#include <getopt.h>
#include <pthread.h>

#include <stdarg.h>

#include "lang/dsl.h"
#include <dlfcn.h>

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

static void repl_input(char *input, int bufsize, const char *prompt) {
  char prev;
  char c;
  int position = 0;

  printf("%s", prompt);
  while (1) {
    prev = c;
    c = getchar();
    if (c == '\\') {
      input[position] = '\n';
      position++;
      continue;
    }

    if (c == EOF || c == '\n') {
      if (prev == '\\') {

        return repl_input(input + position, bufsize, "  ");
      }

      input[position] = '\0';
      // input[++position] = '\0';
      return;
    }
    if (position == 2048) {
      // TODO: increase size of input buffer
    }

    input[position] = c;
    position++;
  }
}

int main(int argc, char **argv) {
  write_log = write_stdout_log;
  setup_audio();

  setup_lang_ctx();

  char input[2048];
  int length = 0;
  for (;;) {
    repl_input(input, 2048, "\x1b[32m~ \x1b[0m");
    process_input(input, length);
  }

  destroy_audio();

  return 0;
}
