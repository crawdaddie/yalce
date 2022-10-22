#ifndef _CONFIG
#define _CONFIG
#include <jack/jack.h>
#define PROJECT_NAME "simple-synth"
#define NUM_CHANNELS 2
#define BUF_SIZE 1024
#define INITIAL_BUSNUM 8
static const double PI = 3.14159265358979323846264338328;
typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;

#endif
