#include "./oscillators.h"
#include "./common.h"
#include "./ctx.h"
#include "node.h"
#include "signals.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {0};
void maketable_sq(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SQ_TABSIZE;
  for (int i = 0; i < SQ_TABSIZE; i++) {

    for (int harm = 1; harm < SQ_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      // for (int harm = 1; harm < 100; harm += 2) { // summing odd frequencies
      double val = sin(phase * harm) / harm; // sinewave of different frequency
      sq_table[i] += val;
    }
    phase += phsinc;
  }
}

typedef struct sin_state {
  double phase;
} sin_state;

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }
}

void *sin_perform(Node *node, int nframes, double spf) {
  sin_state *state = get_node_state(node);
  Signal in = *get_node_input(node, 0);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double *freq;

  int out_layout = node->out.layout;
  double *out = node->out.buf;
  while (nframes--) {
    freq = get_val(&in);
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % SIN_TABSIZE];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }
    state->phase = fmod((*freq) * spf + (state->phase), 1.0);
  }
  return (char *)node + node->node_size;
}

Node *sin_node(Signal *input) {
  Node *node = node_new();
  sin_state *state = (sin_state *)state_new(sizeof(sin_state));

  int in_offset = (char *)input - (char *)node;
  *node = (Node){.num_ins = 1,
                 .input_offsets = {in_offset},
                 .node_size = aligned_size(sizeof(Node) + sizeof(sin_state)),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)sin_perform,
                 .next = NULL};

  // sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){0.};

  return node;
}

typedef struct simple_gate_env_state {
  int phase;
  int end_phase;
} simple_gate_env_state;

void *simple_gate_env_perform(Node *node, int nframes, double spf) {

  simple_gate_env_state *state = get_node_state(node);

  int out_layout = node->out.layout;
  double *out = node->out.buf;
  while (nframes--) {
    if (state->phase < state->end_phase) {
      *out = 1.;
    } else {
      *out = 0.;
    }
    out++;
    printf("%f\n", *out);
    state->phase++;
  }
  return (char *)node + node->node_size;
}

NodeRef simple_gate_env_node(double len) {
  Node *node = node_new();
  simple_gate_env_state *state =
      (simple_gate_env_state *)state_new(sizeof(simple_gate_env_state));

  *node = (Node){.num_ins = 0,
                 .input_offsets = {0},
                 .node_size = sizeof(Node) + sizeof(simple_gate_env_state),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)simple_gate_env_perform,
                 .next = NULL};

  *state = (simple_gate_env_state){0., len * ctx_sample_rate()};
  return node;
}
typedef struct adsr_env_state {
  int phase;         // Current phase counter
  double output;     // Current output value
  double prev_input; // Previous input value to detect transitions

  // ADSR parameters (in samples)
  int attack_samples;
  int decay_samples;
  double sustain_level;
  int release_samples;

  // State tracking
  enum { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } current_state;
  int parent_node_offset;
  bool should_free_parent;
} adsr_env_state;

void *adsr_env_perform(Node *node, int nframes, double spf) {
  adsr_env_state *state = get_node_state(node);
  Signal in = *get_node_input(node, 0);

  int out_layout = node->out.layout;
  double *out = node->out.buf;

  while (nframes--) {
    double *input = get_val(&in);
    double current_input = *input;

    // Detect 0->1 transition (note on)
    if (current_input >= 0.5 && state->prev_input < 0.5) {
      state->current_state = ATTACK;
      state->phase = 0;
    }
    // Detect 1->0 transition (note off)
    else if (current_input < 0.5 && state->prev_input >= 0.5) {
      state->current_state = RELEASE;
      state->phase = 0;
    }

    // Process based on current state
    switch (state->current_state) {
    case IDLE:
      state->output = 0.0;
      break;

    case ATTACK:
      // Linear attack from 0 to 1
      state->output = (double)state->phase / state->attack_samples;
      if (state->output >= 1.0) {
        state->output = 1.0;
        state->current_state = DECAY;
        state->phase = 0;
      }
      break;

    case DECAY:
      // Exponential decay from 1 to sustain_level
      {
        double decay_progress = (double)state->phase / state->decay_samples;
        state->output = 1.0 + (state->sustain_level - 1.0) *
                                  (1.0 - exp(-5.0 * decay_progress)) /
                                  (1.0 - exp(-5.0));
        if (state->phase >= state->decay_samples) {
          state->current_state = SUSTAIN;
          state->output = state->sustain_level;
        }
      }
      break;

    case SUSTAIN:
      state->output = state->sustain_level;
      break;

    case RELEASE:
      // Exponential release from current level to 0
      {
        double start_level =
            (state->phase == 0) ? state->output : state->sustain_level;
        double release_progress = (double)state->phase / state->release_samples;
        state->output = start_level * exp(-5.0 * release_progress);

        if (state->phase >= state->release_samples) {
          if (state->parent_node_offset && state->should_free_parent) {
            Node *target = (char *)node - state->parent_node_offset;
            target->can_free = true;
            target->write_to_dac = false;
          }

          state->current_state = IDLE;
          state->output = 0.0;
        }
      }
      break;
    }

    // Output the envelope value
    for (int i = 0; i < out_layout; i++) {
      *out = state->output;
      out++;
    }

    // Update state
    state->prev_input = current_input;
    state->phase++;
  }

  char *ret = (char *)node + node->node_size;
  return ret;
}

NodeRef adsr_env_node(double attack_time, double decay_time,
                      double sustain_level, double release_time, Signal *gate) {

  Node *node = node_new();
  adsr_env_state *state = (adsr_env_state *)state_new(sizeof(adsr_env_state));

  // Set the input signal offset correctly
  int in_offset = (char *)gate - (char *)node;
  *node =
      (Node){.num_ins = 1,
             .input_offsets = {in_offset}, // Will be set in the function call
             .node_size = aligned_size(sizeof(Node) + sizeof(adsr_env_state)),
             .out = {.size = BUF_SIZE,
                     .layout = 1,
                     .buf = malloc(sizeof(double) * BUF_SIZE)},
             .node_perform = (perform_func_t)adsr_env_perform,
             .next = NULL};

  // Convert time parameters to samples
  int sr = ctx_sample_rate();
  *state = (adsr_env_state){.phase = 0,
                            .output = 0.0,
                            .prev_input = 0.0,
                            .attack_samples = (int)(attack_time * sr),
                            .decay_samples = (int)(decay_time * sr),
                            .sustain_level = sustain_level,
                            .release_samples = (int)(release_time * sr),
                            .current_state = ATTACK,
                            .should_free_parent = true};

  if (_current_blob && _current_blob->first_node_offset != -1) {
    state->parent_node_offset =
        ((char *)node - (char *)_current_blob->blob_data) +
        aligned_size(sizeof(Node) + sizeof(blob_state));
  }

  return node;
}
