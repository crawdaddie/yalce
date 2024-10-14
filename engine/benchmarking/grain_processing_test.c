#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_GRAINS 1000
#define BUFFER_SIZE 48000
#define NUM_FRAMES 48000

// Array of Structs (AoS) version
typedef struct {
  int start;
  int length;
  int position;
  double rate;
  double amp;
} Grain;

typedef struct {
  Grain grains[MAX_GRAINS];
  int active_grains;
  int next_free_grain;
} GranulatorAoS;

// Struct of Arrays (SoA) version
typedef struct {
  int starts[MAX_GRAINS];
  int lengths[MAX_GRAINS];
  int positions[MAX_GRAINS];
  double rates[MAX_GRAINS];
  double amps[MAX_GRAINS];
  int active_grains;
  int next_free_grain;
} GranulatorSoA;

// Function prototypes
void process_aos(GranulatorAoS *state, double *buf, int buf_size, double *out,
                 int nframes);
void process_soa(GranulatorSoA *state, double *buf, int buf_size, double *out,
                 int nframes);

// Benchmarking function
double benchmark(void (*func)(void *, double *, int, double *, int),
                 void *state, double *buf, int buf_size, double *out,
                 int nframes) {
  clock_t start, end;
  double cpu_time_used;

  start = clock();
  func(state, buf, buf_size, out, nframes);
  end = clock();

  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  return cpu_time_used;
}

int main() {
  double *buffer = malloc(BUFFER_SIZE * sizeof(double));
  double *output = malloc(NUM_FRAMES * sizeof(double));

  // Initialize buffer with some data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer[i] = sin(2 * M_PI * i / 100.0);
  }

  GranulatorAoS aos_state = {0};
  GranulatorSoA soa_state = {0};

  // Initialize some grains for testing
  for (int i = 0; i < 100; i++) {
    aos_state.grains[i].start = rand() % BUFFER_SIZE;
    aos_state.grains[i].length = 1000;
    aos_state.grains[i].position = 0;
    aos_state.grains[i].rate = 1.0;
    aos_state.grains[i].amp = 1.0;

    soa_state.starts[i] = rand() % BUFFER_SIZE;
    soa_state.lengths[i] = 1000;
    soa_state.positions[i] = 0;
    soa_state.rates[i] = 1.0;
    soa_state.amps[i] = 1.0;
  }
  aos_state.active_grains = soa_state.active_grains = 100;

  double aos_time =
      benchmark((void (*)(void *, double *, int, double *, int))process_aos,
                &aos_state, buffer, BUFFER_SIZE, output, NUM_FRAMES);
  double soa_time =
      benchmark((void (*)(void *, double *, int, double *, int))process_soa,
                &soa_state, buffer, BUFFER_SIZE, output, NUM_FRAMES);

  printf("AoS time: %f seconds\n", aos_time);
  printf("SoA time: %f seconds\n", soa_time);

  free(buffer);
  free(output);

  return 0;
}

void process_aos(GranulatorAoS *state, double *buf, int buf_size, double *out,
                 int nframes) {
  while (nframes--) {
    *out = 0.0;

    for (int i = 0; i < state->active_grains; i++) {
      Grain *grain = &state->grains[i];
      if (grain->length > 0) {
        double d_index = grain->start + grain->position * grain->rate;
        int index = (int)d_index;
        double frac = d_index - index;

        double a = buf[index % buf_size];
        double b = buf[(index + 1) % buf_size];

        double sample = ((1.0 - frac) * a + (frac * b)) * grain->amp;
        *out += sample;

        grain->position++;
        if (grain->position >= grain->length) {
          grain->length = 0;
          state->active_grains--;
        }
      }
    }

    if (state->active_grains > 0) {
      *out /= state->active_grains;
    }

    out++;
  }
}

void process_soa(GranulatorSoA *state, double *buf, int buf_size, double *out,
                 int nframes) {
  while (nframes--) {
    *out = 0.0;

    for (int i = 0; i < state->active_grains; i++) {
      if (state->lengths[i] > 0) {
        double d_index =
            state->starts[i] + state->positions[i] * state->rates[i];
        int index = (int)d_index;
        double frac = d_index - index;

        double a = buf[index % buf_size];
        double b = buf[(index + 1) % buf_size];

        double sample = ((1.0 - frac) * a + (frac * b)) * state->amps[i];
        *out += sample;

        state->positions[i]++;
        if (state->positions[i] >= state->lengths[i]) {
          state->lengths[i] = 0;
          state->active_grains--;
        }
      }
    }

    if (state->active_grains > 0) {
      *out /= state->active_grains;
    }

    out++;
  }
}
