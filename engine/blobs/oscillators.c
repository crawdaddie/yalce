#include "./oscillators.h"
#include "./common.h"
#include "./ctx.h"
#include "alloc.h"
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

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double *in = get_node_input_buf(node, 0);

  int out_layout = node->output_layout;
  double *out = get_node_out_buf(node);
  double freq;
  while (nframes--) {
    freq = *in;
    in++;
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
    state->phase = fmod(freq * spf + (state->phase), 1.0);
  }
  return (char *)node + node->node_size;
}

Node *sin_node(Signal *input) {
  Node *node = (Node *)engine_alloc(sizeof(Node));
  blob_register_node(node);
  sin_state *state = (sin_state *)engine_alloc(sizeof(sin_state));
  double *out = (double *)engine_alloc(sizeof(double) * BUF_SIZE * 1);

  size_t total_size =
      sizeof(Node) + sizeof(sin_state) + sizeof(double) * BUF_SIZE;

  int in_offset = (char *)(input->buf) - (char *)node;
  *node = (Node){.output_buf_offset = (char *)out - (char *)node,
                 .output_size = BUF_SIZE,
                 .output_layout = 1,
                 .num_ins = 1,
                 .input_offsets = {in_offset},
                 .input_sizes = {input->size},
                 .input_layouts = {input->layout},
                 .node_perform = (perform_func_t)sin_perform,
                 .node_size = total_size,
                 .next = NULL};

  *state = (sin_state){0.};

  return node;
}

double sq_sample(double phase) {
  phase = fmod(phase, 1.0);

  double d_index;
  int index;
  double frac, a, b;
  double sample;

  d_index = phase * SQ_TABSIZE;
  index = (int)d_index;
  frac = d_index - index;

  a = sq_table[index];
  b = sq_table[(index + 1) % SQ_TABSIZE];

  sample = (1.0 - frac) * a + (frac * b);
  return sample;
  // *out = sample;
}

typedef struct sq_state {
  double phase;
} sq_state;

char *sq_perform(Node *node, int nframes, double spf) {

  sq_state *state = get_node_state(node);
  double *in = get_node_input_buf(node, 0);

  int out_layout = node->output_layout;
  double *out = get_node_out_buf(node);

  double freq;
  while (nframes--) {

    freq = *in;
    in++;
    *out = sq_sample(state->phase);
    // *out += sq_sample(state->phase * 1.5, *input);
    // *out /= 2.;
    // printf("sq perform *out %f\n", *out);
    state->phase = fmod(freq * spf + (state->phase), 1.0);
    out++;
  }
}

NodeRef sq_node(SignalRef input) {
  Node *node = engine_alloc(sizeof(Node));
  blob_register_node(node);
  sq_state *state = (sq_state *)engine_alloc(sizeof(sq_state));
  double *out = (double *)engine_alloc(sizeof(double) * BUF_SIZE * 1);

  size_t total_size =
      sizeof(Node) + sizeof(sq_state) + sizeof(double) * BUF_SIZE;

  int in_offset = (char *)(input->buf) - (char *)node;
  *node = (Node){.num_ins = 1,
                 .output_buf_offset = (char *)out - (char *)node,
                 .output_size = BUF_SIZE,
                 .output_layout = 1,
                 .input_offsets = {in_offset},
                 .input_sizes = {input->size},
                 .input_layouts = {input->layout},
                 .node_perform = (perform_func_t)sq_perform,
                 .node_size = total_size,
                 .next = NULL};

  *state = (sq_state){0.};

  return node;
}

typedef struct simple_gate_env_state {
  int phase;
  int end_phase;
} simple_gate_env_state;

void *simple_gate_env_perform(Node *node, int nframes, double spf) {

  simple_gate_env_state *state = get_node_state(node);

  int out_layout = node->output_layout;
  double *out = get_node_out_buf(node);
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
  Node *node = engine_alloc(sizeof(Node));
  simple_gate_env_state *state =
      (simple_gate_env_state *)engine_alloc(sizeof(simple_gate_env_state));

  *node = (Node){.num_ins = 0,
                 .input_offsets = {0},
                 .node_size = sizeof(Node) + sizeof(simple_gate_env_state),
                 // .out = {.size = BUF_SIZE,
                 //         .layout = 1,
                 //         .buf = malloc(sizeof(double) * BUF_SIZE)},
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
  enum {
    ADSR_IDLE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE
  } current_state;
  int parent_node_offset;
  bool should_free_parent;
} adsr_env_state;

void *adsr_env_perform(Node *node, int nframes, double spf) {
  // adsr_env_state *state = get_node_state(node);
  // Signal in = *get_node_input(node, 0);
  //
  // int out_layout = node->out.layout;
  // double *out = node->out.buf;
  //
  // while (nframes--) {
  //   double *input = get_val(&in);
  //   double current_input = *input;
  //
  //   // Detect 0->1 transition (note on)
  //   if (current_input >= 0.5 && state->prev_input < 0.5) {
  //     state->current_state = ADSR_ATTACK;
  //     state->phase = 0;
  //   }
  //   // Detect 1->0 transition (note off)
  //   else if (current_input < 0.5 && state->prev_input >= 0.5) {
  //     state->current_state = ADSR_RELEASE;
  //     state->phase = 0;
  //   }
  //
  //   // Process based on current state
  //   switch (state->current_state) {
  //   case ADSR_IDLE:
  //     state->output = 0.0;
  //     break;
  //
  //   case ADSR_ATTACK:
  //     // Linear attack from 0 to 1
  //     state->output = (double)state->phase / state->attack_samples;
  //     if (state->output >= 1.0) {
  //       state->output = 1.0;
  //       state->current_state = ADSR_DECAY;
  //       state->phase = 0;
  //     }
  //     break;
  //
  //   case ADSR_DECAY:
  //     // Exponential decay from 1 to sustain_level
  //     {
  //       double decay_progress = (double)state->phase / state->decay_samples;
  //       state->output = 1.0 + (state->sustain_level - 1.0) *
  //                                 (1.0 - exp(-5.0 * decay_progress)) /
  //                                 (1.0 - exp(-5.0));
  //       if (state->phase >= state->decay_samples) {
  //         state->current_state = ADSR_SUSTAIN;
  //         state->output = state->sustain_level;
  //       }
  //     }
  //     break;
  //
  //   case ADSR_SUSTAIN:
  //     state->output = state->sustain_level;
  //     break;
  //
  //   case ADSR_RELEASE:
  //     // Exponential release from current level to 0
  //     {
  //       double start_level =
  //           (state->phase == 0) ? state->output : state->sustain_level;
  //       double release_progress = (double)state->phase /
  //       state->release_samples; state->output = start_level * exp(-5.0 *
  //       release_progress);
  //
  //       if (state->phase >= state->release_samples) {
  //         if (state->parent_node_offset && state->should_free_parent) {
  //           Node *target = (char *)node - state->parent_node_offset;
  //           target->can_free = true;
  //           target->write_to_dac = false;
  //         }
  //
  //         state->current_state = ADSR_IDLE;
  //         state->output = 0.0;
  //       }
  //     }
  //     break;
  //   }
  //
  //   // Output the envelope value
  //   for (int i = 0; i < out_layout; i++) {
  //     *out = state->output;
  //     out++;
  //   }
  //
  //   // Update state
  //   state->prev_input = current_input;
  //   state->phase++;
  // }
  //
  char *ret = (char *)node + node->node_size;
  return ret;
}

NodeRef adsr_env_node(double attack_time, double decay_time,
                      double sustain_level, double release_time, Signal *gate) {

  // Node *node = engine_alloc(sizeof(Node));
  //
  // blob_register_node(node);
  // adsr_env_state *state =
  //     (adsr_env_state *)engine_alloc(sizeof(adsr_env_state));
  //
  // // Set the input signal offset correctly
  // int in_offset = (char *)gate - (char *)node;
  // *node =
  //     (Node){.num_ins = 1,
  //            .input_offsets = {in_offset}, // Will be set in the function
  //            call .node_size = aligned_size(sizeof(Node) +
  //            sizeof(adsr_env_state)), .out = {.size = BUF_SIZE,
  //                    .layout = 1,
  //                    .buf = malloc(sizeof(double) * BUF_SIZE)},
  //            .node_perform = (perform_func_t)adsr_env_perform,
  //            .next = NULL};
  //
  // // Convert time parameters to samples
  // int sr = ctx_sample_rate();
  // *state = (adsr_env_state){.phase = 0,
  //                           .output = 0.0,
  //                           .prev_input = 0.0,
  //                           .attack_samples = (int)(attack_time * sr),
  //                           .decay_samples = (int)(decay_time * sr),
  //                           .sustain_level = sustain_level,
  //                           .release_samples = (int)(release_time * sr),
  //                           .current_state = ADSR_ATTACK,
  //                           .should_free_parent = true};
  //
  // if (_current_blob && _current_blob->first_node_offset != -1) {
  //   state->parent_node_offset =
  //       ((char *)node - (char *)_current_blob->blob_data) +
  //       aligned_size(sizeof(Node) + sizeof(blob_state));
  // }
  //
  // return node;
}

typedef struct asr_env_state {
  int phase;         // Current phase counter
  double output;     // Current output value
  double prev_input; // Previous input value to detect transitions

  // asr parameters (in samples)
  int attack_samples;
  int release_samples;

  // State tracking
  enum {
    ASR_IDLE,
    ASR_ATTACK,
    ASR_DECAY,
    ASR_SUSTAIN,
    ASR_RELEASE
  } current_state;
  int parent_node_offset;
  bool should_free_parent;
} asr_env_state;

void *asr_env_perform(Node *node, int nframes, double spf) {
  asr_env_state *state = get_node_state(node);
  double *in = get_node_input_buf(node, 0);

  int out_layout = node->output_layout;
  double *out = get_node_out_buf(node);

  while (nframes--) {
    double input = *in;
    in++;
    double current_input = input;

    // Detect 0->1 transition (note on)
    if (current_input >= 0.5 && state->prev_input < 0.5) {
      state->current_state = ASR_ATTACK;
      state->phase = 0;
    }
    // Detect 1->0 transition (note off)
    else if (current_input < 0.5 && state->prev_input >= 0.5) {
      state->current_state = ASR_RELEASE;
      state->phase = 0;
    }

    // Process based on current state
    switch (state->current_state) {
    case ASR_IDLE:
      state->output = 0.0;
      break;

    case ASR_ATTACK:
      // Linear attack from 0 to 1
      state->output = (double)state->phase / state->attack_samples;
      if (state->output >= 1.0) {
        state->output = 1.0;
        state->current_state = ASR_SUSTAIN;
        state->phase = 0;
      }
      break;

    case ASR_SUSTAIN:
      state->output = 1.0;
      break;

    case ASR_RELEASE:
      // Exponential release from current level to 0
      {
        double start_level = (state->phase == 0) ? state->output : 1.0;

        double release_progress = (double)state->phase / state->release_samples;
        state->output = start_level * exp(-5.0 * release_progress);

        if (state->phase >= state->release_samples) {
          if (state->parent_node_offset && state->should_free_parent) {
            Node *target = (char *)node - state->parent_node_offset;
            target->can_free = true;
            target->write_to_dac = false;
          }

          state->current_state = ASR_IDLE;
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

NodeRef asr_env_node(double attack_time, double release_time, Signal *gate) {

  Node *node = engine_alloc(sizeof(Node));
  blob_register_node(node);
  asr_env_state *state = (asr_env_state *)engine_alloc(sizeof(asr_env_state));
  double *out = (double *)engine_alloc(sizeof(double) * BUF_SIZE * 1);

  size_t total_size =
      sizeof(Node) + sizeof(asr_env_state) + sizeof(double) * BUF_SIZE;

  // Set the input signal offset correctly
  int in_offset = (char *)gate->buf - (char *)node;
  *node = (Node){

      .output_buf_offset = (char *)out - (char *)node,
      .output_size = BUF_SIZE,
      .output_layout = 1,
      .num_ins = 1,
      .input_offsets = {in_offset},
      .input_sizes = {gate->size},
      .input_layouts = {gate->layout},
      .node_size = total_size,
      .node_perform = (perform_func_t)asr_env_perform,
      .next = NULL};

  // Convert time parameters to samples
  int sr = ctx_sample_rate();
  *state = (asr_env_state){.phase = 0,
                           .output = 0.0,
                           .prev_input = 0.0,
                           .attack_samples = (int)(attack_time * sr),
                           .release_samples = (int)(release_time * sr),
                           .current_state = ASR_ATTACK,
                           .should_free_parent = true};

  if (_current_blob && _current_blob->first_node_offset != -1) {
    state->parent_node_offset =
        ((char *)node - (char *)_current_blob->blob_data) +
        aligned_size(sizeof(Node) + sizeof(blob_state));
  }

  return node;
}
