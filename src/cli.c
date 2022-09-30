#include "cli.h"

static int asimp_usage(char *exe) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "Options:\n"
          "  [--backend dummy|alsa|pulseaudio|jack|coreaudio|wasapi]\n"
          "  [--device id]\n"
          "  [--raw]\n"
          "  [--name stream_name]\n"
          "  [--latency seconds]\n"
          "  [--sample-rate hz]\n",
          exe);
  return 1;
}

int asimp_setup(int argc, char **argv, char *stream_name, char *device_id,
                enum SoundIoBackend *backend) {
  enableRawMode();
  char *exe = argv[0];
  double latency = 0.0;
  int sample_rate = 0;
  int raw = 0;
  for (int i = 1; i < argc; i += 1) {
    char *arg = argv[i];
    if (arg[0] == '-' && arg[1] == '-') {
      if (strcmp(arg, "--raw") == 0) {
        raw = true;
      } else {
        i += 1;
        if (i >= argc) {
          return asimp_usage(exe);
        } else if (strcmp(arg, "--backend") == 0) {
          if (strcmp(argv[i], "dummy") == 0) {
            *backend = SoundIoBackendDummy;
          } else if (strcmp(argv[i], "alsa") == 0) {
            *backend = SoundIoBackendAlsa;
          } else if (strcmp(argv[i], "pulseaudio") == 0) {
            *backend = SoundIoBackendPulseAudio;
          } else if (strcmp(argv[i], "jack") == 0) {
            *backend = SoundIoBackendJack;

          } else if (strcmp(argv[i], "coreaudio") == 0) {
            *backend = SoundIoBackendCoreAudio;

          } else if (strcmp(argv[i], "wasapi") == 0) {
            *backend = SoundIoBackendWasapi;
            return 0;
          } else {
            fprintf(stderr, "Invalid backend: %s\n", argv[i]);
            return 1;
          }
        } else if (strcmp(arg, "--device") == 0) {
          device_id = argv[i];
        } else if (strcmp(arg, "--name") == 0) {
          stream_name = argv[i];
        } else if (strcmp(arg, "--latency") == 0) {
          latency = atof(argv[i]);
        } else if (strcmp(arg, "--sample-rate") == 0) {
          sample_rate = atoi(argv[i]);
        } else {
          return asimp_usage(exe);
        }
      };
    } else {
      return asimp_usage(exe);
    }
  }
}

struct termios orig_termios;
void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
