#include "./osc.h"
#include "./common.h"
#include "./node.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>

double _random_double_range(double min, double max);
int save_table_to_file(double *table, int size, const char *filename) {

  FILE *file = fopen(filename, "w");
  if (!file) {
    return -1; // Error opening file
  }

  for (int i = 0; i < size; i++) {
    fprintf(file, "%.16f", table[i]);
    if (i < size - 1) {
      fprintf(file, ",");
    }
    fprintf(file, "\n");
  }

  fclose(file);
  return 0;
}

// #define READ_WTABLES

#define SQ_TABSIZE (1 << 11)
#ifdef READ_WTABLES
static double sq_table[SQ_TABSIZE] = {
#include "./assets/sq_table.csv"
};
#else
static double sq_table[SQ_TABSIZE];
#endif

void maketable_sq(void) {
#ifndef READ_WTABLES
  double phase = 0.0;
  double phsinc = (2. * PI) / SQ_TABSIZE;
  for (int i = 0; i < SQ_TABSIZE; i++) {

    for (int harm = 1; harm < SQ_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      double val = sin(phase * harm) / harm;
      sq_table[i] += 4. * val / PI;
    }
    phase += phsinc;
  }
  save_table_to_file(sq_table, SQ_TABSIZE, "engine/assets/sq_table.csv");
#endif
}
double *get_sq_table() { return sq_table; }
uint32_t get_sq_tabsize() { return SQ_TABSIZE; }

#define SIN_TABSIZE (1 << 11)
#ifdef READ_WTABLES
static double sin_table[SIN_TABSIZE] = {
#include "./assets/sin_table.csv"
};
#else
static double sin_table[SIN_TABSIZE];
#endif

void maketable_sin(void) {
#ifndef READ_WTABLES

  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }
  save_table_to_file(sin_table, SIN_TABSIZE, "engine/assets/sin_table.csv");
#endif
}

double *get_sin_table() { return sin_table; }
uint32_t get_sin_tabsize() { return SIN_TABSIZE; }

typedef struct sin_state {
  double phase;
} sin_state;

void *sin_perform(Node *node, sin_state *state, Node *inputs[], int nframes,
                  double spf) {

  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffer (frequency control) if connected
  double *in = inputs[0]->output.buf;
  double d_index;
  int index = 0;
  double frac, a, b, sample;
  double freq;
  const int table_mask = SIN_TABSIZE - 1; // Assuming SIN_TABSIZE is power of 2

  while (nframes--) {
    freq = *in;
    in++;

    d_index = state->phase * (SIN_TABSIZE);
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index & table_mask];
    b = sin_table[(index + 1) & table_mask];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *sin_node(Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(sin_state));

  *node = (Node){
      .perform = (perform_func_t)sin_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(sin_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(sin_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sin",
  };

  sin_state *state = (sin_state *)(state_ptr(graph, node));

  *state = (sin_state){.phase = 0.0};

  node->connections[0].source_node_index = input->node_index;

  node->state_ptr = state;
  return node;
}

// Square wave oscillator
typedef struct sq_state {
  double phase;
} sq_state;

void *sq_perform(Node *node, sq_state *state, Node *inputs[], int nframes,
                 double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *in = inputs[0]->output.buf;

  double d_index;
  int index;
  double frac, a, b, sample;
  double freq;

  const int table_mask = SQ_TABSIZE - 1; // Assuming SIN_TABSIZE is power of 2

  while (nframes--) {
    freq = *in;
    in++;

    d_index = state->phase * SQ_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;
    a = sq_table[index & table_mask];
    b = sq_table[(index + 1) & table_mask];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *sq_node(Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(sq_state));

  *node = (Node){
      .perform = (perform_func_t)sq_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(sq_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(sq_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sq",
  };

  sq_state *state =
      (sq_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (sq_state){.phase = 0.0};

  node->connections[0].source_node_index = input->node_index;
  node->state_ptr = state;
  return node;
}
static inline double clamp_range(double input, double a, double b) {
  return input < a ? a : (input > b ? b : input);
}

// Square wave oscillator with PWM
void *sq_pwm_perform(Node *node, sq_state *state, Node *inputs[], int nframes,
                     double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;
  double *pw_in = inputs[1]->output.buf;

  double d_index;
  int index;
  double frac, a, b, sample;
  double freq, pw;

  const int table_mask = SQ_TABSIZE - 1;

  while (nframes--) {
    freq = *freq_in++;
    pw = *pw_in++;

    pw = clamp_range(pw, 0.01, 0.99);

    double adjusted_phase;
    if (state->phase < pw) {
      adjusted_phase = 0.5 * state->phase / pw;
    } else {
      adjusted_phase = 0.5 + 0.5 * (state->phase - pw) / (1.0 - pw);
    }

    d_index = adjusted_phase * SQ_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sq_table[index & table_mask];
    b = sq_table[(index + 1) & table_mask];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out++ = sample;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *sq_pwm_node(Node *pw_input, Node *freq_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(sq_state));

  *node = (Node){
      .perform = (perform_func_t)sq_pwm_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(sq_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(sq_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sq_pwm",
  };

  sq_state *state =
      (sq_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (sq_state){.phase = 0.0};

  node->connections[0].source_node_index = freq_input->node_index;
  node->connections[1].source_node_index = pw_input->node_index;

  node->state_ptr = state;
  return node;
}

// Phasor oscillator
typedef struct phasor_state {
  double phase;
} phasor_state;

void *phasor_perform(Node *node, phasor_state *state, Node *inputs[],
                     int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *in = inputs[0]->output.buf;
  double freq;

  while (nframes--) {
    freq = *in;
    in++;

    for (int i = 0; i < out_layout; i++) {
      *out = state->phase;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *phasor_node(Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(phasor_state));

  *node = (Node){
      .perform = (perform_func_t)phasor_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(phasor_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(phasor_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "phasor",
  };

  phasor_state *state =
      (phasor_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (phasor_state){.phase = 0.0};

  node->connections[0].source_node_index = input->node_index;

  node->state_ptr = state;
  return node;
}

// Raw oscillator (using custom wavetable)
typedef struct raw_osc_state {
  double phase;
} raw_osc_state;

void *raw_osc_perform(Node *node, raw_osc_state *state, Node *inputs[],
                      int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;
  double *table = inputs[1]->output.buf;
  int table_size = inputs[1]->output.size;

  double d_index;
  int index;
  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    d_index = state->phase * table_size;
    index = (int)d_index;
    frac = d_index - index;

    a = 2 * table[index % table_size] - 1.0;
    b = 2 * table[(index + 1) % table_size] - 1.0;

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *raw_osc_node(Node *table, Node *freq) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(raw_osc_state));

  *node = (Node){
      .perform = (perform_func_t)raw_osc_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(raw_osc_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(raw_osc_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "raw_osc",
  };

  raw_osc_state *state = (raw_osc_state *)(state_ptr(graph, node));
  *state = (raw_osc_state){.phase = 0.0};

  node->connections[0].source_node_index = freq->node_index;
  node->connections[1].source_node_index = table->node_index;

  node->state_ptr = state;
  return node;
}

// Oscillator Bank
typedef struct osc_bank_state {
  double phase;
} osc_bank_state;

static double get_freq_scaled_sample(double phase, double multiplier,
                                     double *table, int table_size) {
  double d_index = (phase * multiplier) * table_size;
  int index = (int)d_index;
  double frac = d_index - index;

  double a = table[index % table_size];
  double b = table[(index + 1) % table_size];

  return (1.0 - frac) * a + (frac * b);
}

void *osc_bank_perform(Node *node, osc_bank_state *state, Node *inputs[],
                       int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;
  double *amps = inputs[1]->output.buf;
  int num_amps = inputs[1]->output.size;

  double freq;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    double output = 0.0;
    double norm = 1.0;

    for (int i = 0; i < num_amps; i++) {
      double sample = get_freq_scaled_sample(state->phase, (double)(i + 1),
                                             sin_table, SIN_TABSIZE);
      sample *= amps[i];
      norm += amps[i];
      output += sample;
    }

    output /= norm;

    for (int i = 0; i < out_layout; i++) {
      *out = output;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *osc_bank_node(Node *amps, Node *freq) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(osc_bank_state));

  *node = (Node){
      .perform = (perform_func_t)osc_bank_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(osc_bank_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(osc_bank_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "osc_bank",
  };

  osc_bank_state *state = (osc_bank_state *)(state_ptr(graph, node));
  *state = (osc_bank_state){.phase = 0.0};

  node->connections[0].source_node_index = freq->node_index;
  node->connections[1].source_node_index = amps->node_index;

  node->state_ptr = state;

  return node;
}

typedef struct unison_osc_state {
  int num;
} unison_osc_state;

double get_unison_osc_sample(double phase, int table_size, double *table) {
  double d_index = phase * table_size;
  int index = (int)d_index;
  double frac = d_index - index;

  double a = table[index % table_size];
  double b = table[(index + 1) % table_size];

  return (1.0 - frac) * a + (frac * b);
}

void *unison_osc_perform(Node *node, unison_osc_state *state, Node *inputs[],
                         int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;
  double *table = inputs[1]->output.buf;
  int table_size = inputs[1]->output.size;

  double *phases = (double *)(state + 1);

  double *spread_in = inputs[2]->output.buf;
  double *mix_in = inputs[3]->output.buf; // Mix controls only detuned voices

  double d_index;
  int index;
  double frac, a, b, sample;
  double freq;
  double spread;
  double mix;

  while (nframes--) {

    freq = *freq_in;
    freq_in++;
    spread = *spread_in;
    spread_in++;
    mix = *mix_in;
    mix_in++;

    double output = get_unison_osc_sample(phases[0], table_size, table);

    phases[0] = fmod(phases[0] + freq * spf, 1.0);

    // double output = 0.;
    // Add detuned oscillators scaled by mix
    if (mix > 0.0 && state->num > 0) {
      double detuned_sum = 0.0;
      for (int i = 1; i <= state->num; i++) {
        // Higher frequency oscillator
        detuned_sum += get_unison_osc_sample(phases[i], table_size, table);
        // Higher frequency oscillator
        phases[i] = fmod(phases[i] + freq * (1.0 + spread * i) * spf, 1.0);

        // Lower frequency oscillator
        detuned_sum +=
            get_unison_osc_sample(phases[i + state->num], table_size, table);
        // Lower frequency oscillator
        phases[i + state->num] =
            fmod(phases[i + state->num] + freq * (1.0 - spread * i) * spf, 1.0);
      }

      // Only apply mix parameter to detuned voices
      output += mix * detuned_sum;
    }

    // Apply normalization only when detuned voices are added
    if (mix > 0.0) {
      // Dynamic normalization based on mix parameter:
      // At mix=0: norm = 1 (just fundamental)
      // At mix=1: norm = 1 + 2*num (all voices at full volume)
      double norm = 1.0 + (2.0 * state->num * mix);
      output /= norm;
    }
    // Write output to all channels
    for (int i = 0; i < out_layout; i++) {
      *out++ = output;
    }
  }

  return node->output.buf;
}

NodeRef unison_osc_node(int num, NodeRef spread, NodeRef mix, NodeRef table,
                        NodeRef freq) {

  AudioGraph *graph = _graph;
  int state_size = sizeof(unison_osc_state) + (1 + 2 * num) * sizeof(double);
  state_size = (state_size + 7) & ~7; /* Align to 8-byte boundary */
  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)unison_osc_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = sizeof(unison_osc_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(unison_osc_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "unison_osc",
  };

  unison_osc_state *state = (unison_osc_state *)(state_ptr(graph, node));
  *state = (unison_osc_state){.num = num};
  double *phases = (double *)(state + 1);
  int phases_size = num * 2 + 1;
  for (int i = 1; i < phases_size; i++) {
    phases[i] = (double)rand() / RAND_MAX; // randomize phases
  }

  node->connections[0].source_node_index = freq->node_index;
  node->connections[1].source_node_index = table->node_index;
  node->connections[2].source_node_index = spread->node_index;
  node->connections[3].source_node_index = mix->node_index;

  node->state_ptr = state;

  return node;
}

// Buffer player
typedef struct bufplayer_state {
  double phase;
  double prev_trig;
} bufplayer_state;

void *__bufplayer_perform(Node *node, bufplayer_state *state, Node *inputs[],
                          int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;
  double *rate = inputs[1]->output.buf;

  double d_index, frac, a, b, sample;
  int index;

  while (nframes--) {
    d_index = state->phase * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index % buf_size];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);
    state->phase = fmod(state->phase + (*rate) / buf_size, 1.0);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    rate++;
  }

  return node->output.buf;
}

Node *__bufplayer_node(Node *buf, Node *rate) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  *node = (Node){
      .perform = (perform_func_t)__bufplayer_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(bufplayer_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(bufplayer_state)),
      .output = (Signal){.layout = buf->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, BUF_SIZE * buf->output.layout)},
      .meta = "bufplayer",
  };

  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = rate->node_index;

  node->state_ptr = state;
  return node;
}
// Buffer player with multi-channel support

void *bufplayer_perform(Node *node, bufplayer_state *state, Node *inputs[],
                        int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;
  int buf_layout = inputs[0]->output.layout;

  double *rate = inputs[1]->output.buf;

  for (int frame = 0; frame < nframes; frame++) {
    int buf_frames = buf_size;
    double d_index = state->phase * buf_frames;
    int index = (int)d_index;
    double frac = d_index - index;

    int pos_a = (index % buf_frames) * buf_layout;
    int pos_b = ((index + 1) % buf_frames) * buf_layout;

    for (int ch = 0; ch < out_layout; ch++) {
      int buf_ch = (ch < buf_layout) ? ch : (buf_layout - 1);

      double a = buf[pos_a + buf_ch];
      double b = buf[pos_b + buf_ch];

      double sample = (1.0 - frac) * a + (frac * b);

      out[frame * out_layout + ch] = sample;
    }

    state->phase = fmod(state->phase + rate[frame] / buf_frames, 1.0);
  }

  return node->output.buf;
}

Node *bufplayer_node(Node *buf, Node *rate) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  *node = (Node){
      .perform = (perform_func_t)bufplayer_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(bufplayer_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(bufplayer_state)),
      .output = (Signal){.layout = buf->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, BUF_SIZE * buf->output.layout)},
      .meta = "bufplayer",
  };

  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = rate->node_index;

  node->state_ptr = state;
  return node;
}

// Buffer player with trigger
void *_bufplayer_trig_perform(Node *node, bufplayer_state *state,
                              Node *inputs[], int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;
  double *rate = inputs[1]->output.buf;
  double *trig = inputs[2]->output.buf;
  double *start_pos = inputs[3]->output.buf;

  double d_index, frac, a, b, sample;
  int index;

  while (nframes--) {
    if (*trig > 0.5 && state->prev_trig < 0.5) {
      state->phase = 0;
    }

    d_index = (fmod(state->phase + *start_pos, 1.0)) * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index % buf_size];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);
    state->phase = fmod(state->phase + *rate / buf_size, 1.0);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->prev_trig = *trig;
    rate++;
    trig++;
    start_pos++;
  }

  return node->output.buf;
}

Node *_bufplayer_trig_node(Node *buf, Node *rate, Node *start_pos, Node *trig) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  *node = (Node){
      .perform = (perform_func_t)_bufplayer_trig_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = sizeof(bufplayer_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(bufplayer_state)),
      .output = (Signal){.layout = buf->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, buf->output.layout * BUF_SIZE)},
      .meta = "bufplayer_trig",
  };

  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = rate->node_index;
  node->connections[2].source_node_index = trig->node_index;
  node->connections[3].source_node_index = start_pos->node_index;

  node->state_ptr = state;
  return node;
}
void *bufplayer_trig_perform(Node *node, bufplayer_state *state, Node *inputs[],
                             int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;
  int buf_channels = inputs[0]->output.layout;
  double *rate = inputs[1]->output.buf;
  double *trig = inputs[2]->output.buf;
  double *start_pos = inputs[3]->output.buf;

  double d_index, frac, a, b, sample;
  int index;

  while (nframes--) {
    if (*trig > 0.5 && state->prev_trig < 0.5) {
      state->phase = 0;
    }

    d_index = (fmod(state->phase + *start_pos, 1.0)) * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    for (int i = 0; i < out_layout; i++) {
      int channel = i % buf_channels;

      // Calculate buffer indices for the current channel, accounting for
      // interleaving
      int current_index = (index * buf_channels) + channel;
      int next_index = ((index + 1) * buf_channels) + channel;

      // Make sure we don't go out of bounds
      current_index = current_index % (buf_size * buf_channels);
      next_index = next_index % (buf_size * buf_channels);

      // Get samples from the interleaved buffer
      a = buf[current_index];
      b = buf[next_index];

      // Linear interpolation
      sample = (1.0 - frac) * a + (frac * b);
      *out++ = sample;
    }

    state->phase = fmod(state->phase + *rate / buf_size, 1.0);
    if (state->phase < 0.) {
      state->phase = 1.;
    }
    state->prev_trig = *trig;
    rate++;
    trig++;
    start_pos++;
  }

  return node->output.buf;
}

Node *bufplayer_trig_node(Node *buf, Node *rate, Node *start_pos, Node *trig) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  *node = (Node){
      .perform = (perform_func_t)bufplayer_trig_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = sizeof(bufplayer_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(bufplayer_state)),
      .output = (Signal){.layout = buf->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, buf->output.layout * BUF_SIZE)},
      .meta = "bufplayer_trig",
  };

  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = rate->node_index;
  node->connections[2].source_node_index = trig->node_index;
  node->connections[3].source_node_index = start_pos->node_index;

  node->state_ptr = state;
  return node;
}

// White noise generator
double _random_double_range(double min, double max) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

void *white_noise_perform(Node *node, void *state, Node *inputs[], int nframes,
                          double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  while (nframes--) {
    double sample = _random_double_range(-1.0, 1.0);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }
  }

  return node->output.buf;
}

Node *white_noise_node() {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = (perform_func_t)white_noise_perform,
      .node_index = node->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "white_noise",
  };

  return node;
}

// Brown noise generator
typedef struct brown_noise_state {
  double last;
} brown_noise_state;

void *brown_noise_perform(Node *node, brown_noise_state *state, Node *inputs[],
                          int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double scale = sqrt(spf);

  while (nframes--) {
    double add = scale * _random_double_range(-1.0, 1.0);
    state->last += add;

    if (state->last > 1.0 || state->last < -1.0) {
      state->last = state->last * 0.999;
    }

    for (int i = 0; i < out_layout; i++) {
      *out = state->last;
      out++;
    }
  }

  return node->output.buf;
}

Node *brown_noise_node() {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(brown_noise_state));

  *node = (Node){
      .perform = (perform_func_t)brown_noise_perform,
      .node_index = node->node_index,
      .num_inputs = 0,
      .state_size = sizeof(brown_noise_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(brown_noise_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "brown_noise",
  };

  brown_noise_state *state =
      (brown_noise_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (brown_noise_state){.last = 0.0};

  node->state_ptr = state;
  return node;
}

// lf White noise generator
// LF Noise node with frequency and range inputs
typedef struct lfnoise_state {
  double current_value; // Current output value
  double target_value;  // Target value to ramp to
  double samples_left;  // Samples remaining until next random value
} lfnoise_state;

void *lfnoise_perform(Node *node, lfnoise_state *state, Node *inputs[],
                      int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Input signals
  double *freq_in = inputs[0]->output.buf; // Frequency in Hz
  double *min_in = inputs[1]->output.buf;  // Minimum value
  double *max_in = inputs[2]->output.buf;  // Maximum value

  double freq, min_range, max_range;
  double sample, increment;

  while (nframes--) {
    freq = *freq_in++;
    min_range = *min_in++;
    max_range = *max_in++;

    // If we need a new random value
    if (state->samples_left <= 0) {
      // Generate new target value
      state->target_value = _random_double_range(min_range, max_range);

      // Calculate samples until next change (based on frequency)
      double samples_per_cycle = 1.0 / (freq * spf);
      state->samples_left = samples_per_cycle;

      // Calculate increment per sample to reach target value
      increment =
          (state->target_value - state->current_value) / samples_per_cycle;
    } else {
      // Continue ramping to target
      increment =
          (state->target_value - state->current_value) / state->samples_left;
    }

    // Update current value with increment
    state->current_value += increment;

    // Decrement samples counter
    state->samples_left--;

    // Output current value
    for (int i = 0; i < out_layout; i++) {
      *out++ = state->current_value;
    }
  }

  return node->output.buf;
}

NodeRef lfnoise_node(NodeRef freq_input, NodeRef min_input, NodeRef max_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(lfnoise_state));

  *node = (Node){
      .perform = (perform_func_t)lfnoise_perform,
      .node_index = node->node_index,
      .num_inputs = 3, // frequency, min range, max range
      .state_size = sizeof(lfnoise_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(lfnoise_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lfnoise",
  };

  lfnoise_state *state =
      (lfnoise_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (lfnoise_state){
      .current_value = 0.0,
      .target_value = 0.0,
      .samples_left = 0 // This will trigger immediate generation of a new value
  };

  // Connect inputs
  node->connections[0].source_node_index = freq_input->node_index;
  node->connections[1].source_node_index = min_input->node_index;
  node->connections[2].source_node_index = max_input->node_index;

  node->state_ptr = state;
  return node;
}
typedef struct chirp_state {
  double start_freq;
  double end_freq;
  double current_freq;
  double elapsed_time;
  int trigger_active;
  double prev_trig_value;
} chirp_state;

void *static_chirp_perform(Node *node, chirp_state *state, Node *inputs[],
                           int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double *lag = inputs[1]->output.buf;

  for (int i = 0; i < nframes; i++) {
    double trig_value = trig[i];
    double lag_time = lag[i];

    if (((trig_value == 1.0) && (state->prev_trig_value != 1.0)) ||
        (state->prev_trig_value <= 0.5 && trig_value > 0.5)) {

      state->trigger_active = 1;
      state->current_freq = state->start_freq;
      state->elapsed_time = 0.0;
    }

    state->prev_trig_value = trig_value;

    if (state->trigger_active) {
      double progress = state->elapsed_time / lag_time;
      if (progress > 1.0)
        progress = 1.0;

      state->current_freq = state->start_freq *
                            pow(state->end_freq / state->start_freq, progress);

      state->elapsed_time += spf;

      if (state->elapsed_time >= lag_time) {
        state->trigger_active = 0;
        state->current_freq = state->end_freq;
      }
    }

    for (int ch = 0; ch < out_layout; ch++) {
      out[i * out_layout + ch] = state->current_freq;
    }
  }

  return node->output.buf;
}

Node *static_chirp_node(double start_freq, double end_freq, Node *lag_input,
                        Node *trig_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(chirp_state));

  *node = (Node){
      .perform = (perform_func_t)static_chirp_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(chirp_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(chirp_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "chirp",
  };

  chirp_state *state =
      (chirp_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (chirp_state){.start_freq = start_freq,
                         .end_freq = end_freq,
                         .current_freq = start_freq,
                         .elapsed_time = 0.0,
                         .trigger_active = 0,
                         .prev_trig_value = 0.0};

  node->connections[0].source_node_index = trig_input->node_index;
  node->connections[1].source_node_index = lag_input->node_index;
  node->state_ptr = state;

  return node;
}

void *chirp_perform(Node *node, chirp_state *state, Node *inputs[], int nframes,
                    double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double *lag = inputs[1]->output.buf;
  double *_hi = inputs[2]->output.buf;
  double *_lo = inputs[3]->output.buf;

  for (int i = 0; i < nframes; i++) {
    double trig_value = trig[i];
    double lag_time = lag[i];
    double hi = _hi[i];
    double lo = _lo[i];

    if (((trig_value == 1.0) && (state->prev_trig_value != 1.0)) ||
        (state->prev_trig_value <= 0.5 && trig_value > 0.5)) {

      state->trigger_active = 1;
      state->start_freq = hi;
      state->current_freq = hi;
      state->end_freq = lo;
      state->elapsed_time = 0.0;
    }

    state->prev_trig_value = trig_value;

    if (state->trigger_active) {
      double progress = state->elapsed_time / lag_time;
      if (progress > 1.0)
        progress = 1.0;

      state->current_freq = state->start_freq *
                            pow(state->end_freq / state->start_freq, progress);

      state->elapsed_time += spf;

      if (state->elapsed_time >= lag_time) {
        state->trigger_active = 0;
        state->current_freq = state->end_freq;
      }
    }

    for (int ch = 0; ch < out_layout; ch++) {
      out[i * out_layout + ch] = state->current_freq;
    }
  }

  return node->output.buf;
}

Node *chirp_node(NodeRef start_freq, NodeRef end_freq, Node *lag_input,
                 Node *trig_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(chirp_state));

  *node = (Node){
      .perform = (perform_func_t)chirp_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = sizeof(chirp_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(chirp_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "chirp",
  };

  chirp_state *state =
      (chirp_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (chirp_state){.current_freq = 0.,
                         .elapsed_time = 0.0,
                         .trigger_active = 0,
                         .prev_trig_value = 0.0};

  node->connections[0].source_node_index = trig_input->node_index;
  node->connections[1].source_node_index = lag_input->node_index;
  node->connections[2].source_node_index = start_freq->node_index;
  node->connections[3].source_node_index = end_freq->node_index;

  node->state_ptr = state;

  return node;
}

// Impulse generator (outputs a single sample of 1.0 per cycle)
typedef struct impulse_state {
  double phase;
} impulse_state;

void *impulse_perform(Node *node, impulse_state *state, Node *inputs[],
                      int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;

  double freq;
  double sample;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    if (state->phase >= 1.0) {
      sample = 1.0;
      state->phase = state->phase - 1.0;
    } else {
      sample = 0.0;
    }

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase += freq * spf;
  }

  return node->output.buf;
}

Node *impulse_node(Node *freq) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(impulse_state));

  *node = (Node){
      .perform = (perform_func_t)impulse_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(impulse_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(impulse_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "impulse",
  };

  impulse_state *state =
      (impulse_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (impulse_state){.phase = 1.0};

  node->connections[0].source_node_index = freq->node_index;
  node->state_ptr = state;
  return node;
}

// Ramp generator
typedef struct ramp_state {
  double phase;
} ramp_state;

void *ramp_perform(Node *node, ramp_state *state, Node *inputs[], int nframes,
                   double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;

  double freq;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    for (int i = 0; i < out_layout; i++) {
      *out = state->phase;
      out++;
    }

    state->phase = state->phase + (freq * spf);
    if (state->phase >= 1.0) {
      state->phase -= 1.0;
    }
  }

  return node->output.buf;
}

Node *ramp_node(Node *freq) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(ramp_state));

  *node = (Node){
      .perform = (perform_func_t)ramp_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(ramp_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(ramp_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "ramp",
  };

  ramp_state *state =
      (ramp_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (ramp_state){.phase = 0.0};

  node->connections[0].source_node_index = freq->node_index;

  node->state_ptr = state;
  return node;
}

// Triggered random generator
typedef struct trig_rand_state {
  double value;
} trig_rand_state;

void *trig_rand_perform(Node *node, trig_rand_state *state, Node *inputs[],
                        int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;

  while (nframes--) {
    if (*trig == 1.0) {
      state->value = _random_double_range(0.0, 1.0);
    }

    for (int i = 0; i < out_layout; i++) {
      *out = state->value;
      out++;
    }

    trig++;
  }

  return node->output.buf;
}

Node *trig_rand_node(Node *trig) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(trig_rand_state));

  *node = (Node){
      .perform = (perform_func_t)trig_rand_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(trig_rand_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(trig_rand_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "trig_rand",
  };

  trig_rand_state *state =
      (trig_rand_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (trig_rand_state){.value = _random_double_range(0.0, 1.0)};

  node->connections[0].source_node_index = trig->node_index;
  node->state_ptr = state;

  return node;
}

void *trig_range_perform(Node *node, trig_rand_state *state, Node *inputs[],
                         int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double *low = inputs[1]->output.buf;
  double *high = inputs[2]->output.buf;

  while (nframes--) {

    if (*trig == 1.0) {
      state->value = _random_double_range(*low, *high);
    }
    low++;
    high++;

    *out = state->value;
    out++;

    trig++;
  }

  return node->output.buf;
}
Node *trig_range_node(Node *low, Node *high, Node *trig) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(trig_rand_state));

  *node = (Node){
      .perform = (perform_func_t)trig_range_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(trig_rand_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(trig_rand_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "trig_range",
  };

  trig_rand_state *state =
      (trig_rand_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (trig_rand_state){.value = _random_double_range(0.0, 1.0)};

  node->connections[0].source_node_index = trig->node_index;
  node->connections[1].source_node_index = low->node_index;
  node->connections[2].source_node_index = high->node_index;

  node->state_ptr = state;
  return node;
}

// Triggered selector
typedef struct trig_sel_state {
  double value;
} trig_sel_state;

void *trig_sel_perform(Node *node, trig_sel_state *state, Node *inputs[],
                       int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double *sels = inputs[1]->output.buf;
  int sels_size = inputs[1]->output.size;

  while (nframes--) {
    if (*trig == 1.0) {
      state->value = sels[rand() % sels_size];
    }

    for (int i = 0; i < out_layout; i++) {
      *out = state->value;
      out++;
    }

    trig++;
  }

  return node->output.buf;
}

Node *trig_sel_node(Node *trig, Node *sels) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(trig_sel_state));

  *node = (Node){
      .perform = (perform_func_t)trig_sel_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(trig_sel_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(trig_sel_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "trig_sel",
  };

  trig_sel_state *state =
      (trig_sel_state *)(graph->nodes_state_memory + node->state_offset);

  if (trig) {
    node->connections[0].source_node_index = trig->node_index;
  }
  if (sels) {
    node->connections[1].source_node_index = sels->node_index;
    double *sels_buf = sels->output.buf;
    int sels_size = sels->output.size;
    state->value = sels_buf[rand() % sels_size];
  } else {
    state->value = 0.0;
  }

  node->state_ptr = state;
  return node;
}

typedef struct pm_state {
  double carrier_phase;
  double modulator_phase;
} pm_state;

void *pm_perform_optimized(Node *node, pm_state *state, Node **inputs,
                           int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  double *freq_in = inputs[0]->output.buf;

  double *mod_index_in = inputs[1]->output.buf;
  double *mod_ratio_in = inputs[2]->output.buf;

  const double table_size = (double)SIN_TABSIZE;
  const int table_mask = SIN_TABSIZE - 1;

  double carrier_freq, modulator_freq;
  double mod_index, mod_ratio;
  double mod_phase_scaled, carrier_phase_scaled;
  int mod_index_int, carrier_index_int;
  double mod_frac, carrier_frac;
  double modulator_value, carrier_value;
  double modulated_phase;

  while (nframes--) {
    carrier_freq = *freq_in++;
    mod_index = *mod_index_in++;
    mod_ratio = *mod_ratio_in++;

    modulator_freq = carrier_freq * mod_ratio;

    mod_phase_scaled = state->modulator_phase * table_size;
    mod_index_int = (int)mod_phase_scaled;
    mod_frac = mod_phase_scaled - mod_index_int;

    mod_index_int &= table_mask;

    int mod_idx_0 = (mod_index_int - 1) & table_mask;
    int mod_idx_1 = mod_index_int;
    int mod_idx_2 = (mod_index_int + 1) & table_mask;
    int mod_idx_3 = (mod_index_int + 2) & table_mask;

    double mod_y0 = sin_table[mod_idx_0];
    double mod_y1 = sin_table[mod_idx_1];
    double mod_y2 = sin_table[mod_idx_2];
    double mod_y3 = sin_table[mod_idx_3];

    double mod_c0 = mod_y1;
    double mod_c1 = 0.5 * (mod_y2 - mod_y0);
    double mod_c2 = mod_y0 - 2.5 * mod_y1 + 2.0 * mod_y2 - 0.5 * mod_y3;
    double mod_c3 = 0.5 * (mod_y3 - mod_y0) + 1.5 * (mod_y1 - mod_y2);

    modulator_value =
        ((mod_c3 * mod_frac + mod_c2) * mod_frac + mod_c1) * mod_frac + mod_c0;

    modulator_value *= mod_index;

    modulated_phase = state->carrier_phase + modulator_value;
    modulated_phase -= floor(modulated_phase);

    carrier_phase_scaled = modulated_phase * table_size;
    carrier_index_int = (int)carrier_phase_scaled;
    carrier_frac = carrier_phase_scaled - carrier_index_int;

    carrier_index_int &= table_mask;

    int carr_idx_0 = (carrier_index_int - 1) & table_mask;
    int carr_idx_1 = carrier_index_int;
    int carr_idx_2 = (carrier_index_int + 1) & table_mask;
    int carr_idx_3 = (carrier_index_int + 2) & table_mask;

    double carr_y0 = sin_table[carr_idx_0];
    double carr_y1 = sin_table[carr_idx_1];
    double carr_y2 = sin_table[carr_idx_2];
    double carr_y3 = sin_table[carr_idx_3];

    double carr_c0 = carr_y1;
    double carr_c1 = 0.5 * (carr_y2 - carr_y0);
    double carr_c2 = carr_y0 - 2.5 * carr_y1 + 2.0 * carr_y2 - 0.5 * carr_y3;
    double carr_c3 = 0.5 * (carr_y3 - carr_y0) + 1.5 * (carr_y1 - carr_y2);

    carrier_value =
        ((carr_c3 * carrier_frac + carr_c2) * carrier_frac + carr_c1) *
            carrier_frac +
        carr_c0;

    for (int i = 0; i < out_layout; i++) {
      *out++ = carrier_value;
    }

    state->modulator_phase += modulator_freq * spf;
    state->modulator_phase -= floor(state->modulator_phase);

    state->carrier_phase += carrier_freq * spf;
    state->carrier_phase -= floor(state->carrier_phase);
  }

  return node->output.buf;
}
Node *pm_node(Node *freq_input, Node *mod_index_input, Node *mod_ratio_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(pm_state));

  *node = (Node){
      .perform = (perform_func_t)pm_perform_optimized,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(pm_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(pm_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "fm",
  };

  pm_state *state = (pm_state *)(state_ptr(graph, node));

  *state = (pm_state){
      .carrier_phase = 0.0,
      .modulator_phase = 0.0,
  };

  node->connections[0].source_node_index = freq_input->node_index;
  node->connections[1].source_node_index = mod_index_input->node_index;
  node->connections[2].source_node_index = mod_ratio_input->node_index;
  node->state_ptr = state;
  return node;
}
#define SAW_TABSIZE (1 << 11)
#ifdef READ_WTABLES
static double saw_table[SAW_TABSIZE] = {
#include "./assets/saw_table.csv"
};
#else
static double saw_table[SQ_TABSIZE];
#endif

void maketable_saw(void) {
#ifndef READ_WTABLES
  double phase = 0.0;
  double phsinc = (2. * PI) / SAW_TABSIZE;
  for (int i = 0; i < SAW_TABSIZE; i++) {

    for (int harm = 1; harm < SAW_TABSIZE / 2;
         harm += 1) { // summing n frequencies

      double val = sin(phase * harm) / harm;
      saw_table[i] += (-2. * val) / PI;
    }
    phase += phsinc;
  }
  save_table_to_file(saw_table, SAW_TABSIZE, "engine/assets/saw_table.csv");
#endif
}

double *get_saw_table() { return saw_table; }
uint32_t get_saw_tabsize() { return SAW_TABSIZE; }

typedef struct saw_state {
  double phase;
} saw_state;
void *saw_perform(Node *node, saw_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *in = inputs[0]->output.buf;

  double d_index;
  int index;
  double frac, a, b, sample;
  double freq;

  const int table_mask = SAW_TABSIZE - 1; // Assuming SIN_TABSIZE is power of 2

  while (nframes--) {
    freq = *in;
    in++;

    d_index = state->phase * SAW_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;
    a = saw_table[index & table_mask];
    b = saw_table[(index + 1) & table_mask];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *saw_node(Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(saw_state));

  *node = (Node){
      .perform = (perform_func_t)saw_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(saw_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(saw_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "saw",
  };

  saw_state *state =
      (saw_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (saw_state){.phase = 0.0};

  node->connections[0].source_node_index = input->node_index;

  node->state_ptr = state;
  return node;
}

// Granulator processor
typedef struct {
  int starts[32];    // Start positions for each grain
  int lengths[32];   // Length of each grain
  int positions[32]; // Current position in each grain
  double rates[32];  // Playback rate for each grain
  double amps[32];   // Amplitude of each grain

  int max_concurrent_grains;
  int active_grains;
  int min_grain_length;
  int max_grain_length;
  double overlap;
  int next_free_grain;
} granulator_state;

static void init_grain(granulator_state *state, int index, double pos,
                       int length, double rate) {
  state->starts[index] = (int)pos;
  state->lengths[index] = length;
  state->positions[index] = 0;
  state->rates[index] = rate;
  state->amps[index] = 0.0;
  state->active_grains++;
}

static void process_grain(granulator_state *state, int i, double *out,
                          double *buf, int buf_size) {
  double d_index = state->starts[i] + state->positions[i] * state->rates[i];
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
    if (i < state->next_free_grain) {
      state->next_free_grain = i;
    }
  } else {
    double env_pos = (double)state->positions[i] / state->lengths[i];
    if (env_pos < state->overlap) {
      state->amps[i] = env_pos / state->overlap;
    } else if (env_pos > (1.0 - state->overlap)) {
      state->amps[i] = (1.0 - env_pos) / state->overlap;
    } else {
      state->amps[i] = 1.0;
    }
  }
}

void *granulator_perform(Node *node, granulator_state *state, Node *inputs[],
                         int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;
  double *trig = inputs[1]->output.buf;
  double *pos = inputs[2]->output.buf;
  double *rate = inputs[3]->output.buf;

  while (nframes--) {
    double sample = 0.0;

    if (*trig == 1.0 && state->active_grains < state->max_concurrent_grains) {
      double p = fabs(*pos);
      int start_pos = (int)(p * buf_size);
      int length = state->min_grain_length +
                   rand() % (state->max_grain_length - state->min_grain_length);

      int index = state->next_free_grain;
      init_grain(state, index, start_pos, length, *rate);

      do {
        state->next_free_grain =
            (state->next_free_grain + 1) % state->max_concurrent_grains;
      } while (state->lengths[state->next_free_grain] != 0 &&
               state->next_free_grain != index);
    }

    for (int i = 0; i < state->max_concurrent_grains; i++) {
      if (state->lengths[i] > 0) {
        process_grain(state, i, &sample, buf, buf_size);
      }
    }

    if (state->active_grains > 0) {
      sample /= state->active_grains;
    }

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    trig++;
    pos++;
    rate++;
  }

  return node->output.buf;
}

Node *granulator_node(int max_grains, Node *buf, Node *trig, Node *pos,
                      Node *rate) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(granulator_state));

  if (max_grains <= 0)
    max_grains = 8;
  if (max_grains > 32)
    max_grains = 32;

  size_t state_size = sizeof(granulator_state);

  *node = (Node){
      .perform = (perform_func_t)granulator_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "granulator",
  };

  granulator_state *state =
      (granulator_state *)(graph->nodes_state_memory + node->state_offset);

  state->max_concurrent_grains = max_grains;
  state->active_grains = 0;
  state->min_grain_length = 7000;
  state->max_grain_length = 7001;
  state->overlap = 0.3;
  state->next_free_grain = 0;

  for (int i = 0; i < max_grains; i++) {
    state->starts[i] = 0;
    state->lengths[i] = 0;
    state->positions[i] = 0;
    state->rates[i] = 1.0;
    state->amps[i] = 0.0;
  }

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = trig->node_index;
  node->connections[2].source_node_index = pos->node_index;
  node->connections[3].source_node_index = rate->node_index;

  node->state_ptr = state;
  return node;
}

typedef struct rand_trig_state {
  double val;
} rand_trig_state;

double __rand_double_range(double min, double max) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

void *rand_trig_perform(Node *node, rand_trig_state *state, Node *inputs[],
                        int nframes, double spf) {

  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *in = inputs[0]->output.buf;
  double *low = inputs[1]->output.buf;
  double *high = inputs[2]->output.buf;

  while (nframes--) {

    double lowv = *low;
    low++;

    double highv = *high;
    high++;

    if (*in == 1.0) {
      state->val = __rand_double_range(lowv, highv);
      // printf("trig: %f\n", state->val);
    }
    *out = state->val;
    out++;
    in++;
  }

  return node->output.buf;
}

Node *rand_trig_node(Node *trig_input, Node *low, Node *high) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(rand_trig_state));

  *node = (Node){
      .perform = (perform_func_t)rand_trig_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(rand_trig_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(rand_trig_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "rand_trig",
  };

  rand_trig_state *state =
      (rand_trig_state *)(graph->nodes_state_memory + node->state_offset);

  node->connections[0].source_node_index = trig_input->node_index;
  node->connections[1].source_node_index = low->node_index;
  node->connections[2].source_node_index = high->node_index;

  node->state_ptr = state;
  return node;
}

/**
 * Creates a wavetable containing a Gaussian curve for granular synthesis.
 *
 * @param size The size of the wavetable (typically a power of 2 like 512)
 * @param sigma Controls the width of the Gaussian bell (between 0.1 and 0.5
 * works well)
 * @return Pointer to the allocated wavetable, or NULL if allocation fails
 */
double *create_gaussian_wavetable(double *wavetable, int size, double sigma) {
  double center = (size - 1) / 2.0;

  double scaling = 1.0 / (sigma * sqrt(2.0 * M_PI));

  double variance = sigma * sigma * size * size;

  double max_value = 0.0;
  for (int i = 0; i < size; i++) {
    double x = i - center;
    double exponent = -(x * x) / (2.0 * variance);
    wavetable[i] = scaling * exp(exponent);

    if (wavetable[i] > max_value) {
      max_value = wavetable[i];
    }
  }

  for (int i = 0; i < size; i++) {
    wavetable[i] /= max_value;
  }

  return wavetable;
}

/**
 * Alternative version that creates a Gaussian curve with specified attack and
 * decay times This is useful for asymmetric grain envelopes
 *
 * @param size The size of the wavetable (typically a power of 2 like 512)
 * @param attack_ratio Ratio of attack time to total time (0.0-1.0)
 * @param sigma_attack Sigma parameter for attack portion
 * @param sigma_decay Sigma parameter for decay portion
 * @return Pointer to the allocated wavetable, or NULL if allocation fails
 */
double *create_asymmetric_gaussian_wavetable(double *wavetable, int size,
                                             double attack_ratio,
                                             double sigma_attack,
                                             double sigma_decay) {
  // Validate parameters
  if (attack_ratio < 0.0 || attack_ratio > 1.0) {
    attack_ratio = 0.5; // Default to symmetric
  }

  int attack_samples = (int)(size * attack_ratio);
  int decay_samples = size - attack_samples;

  double max_value = 0.0;

  for (int i = 0; i < attack_samples; i++) {
    double x = i - attack_samples; // Negative values for left side
    double normalized_x = x / (double)attack_samples; // Normalize to -1 to 0
    double exponent =
        -(normalized_x * normalized_x) / (2.0 * sigma_attack * sigma_attack);
    wavetable[i] = exp(exponent);

    if (wavetable[i] > max_value) {
      max_value = wavetable[i];
    }
  }

  for (int i = attack_samples; i < size; i++) {
    double x = i - attack_samples; // 0 to positive values for right side
    double normalized_x = x / (double)decay_samples; // Normalize to 0 to 1
    double exponent =
        -(normalized_x * normalized_x) / (2.0 * sigma_decay * sigma_decay);
    wavetable[i] = exp(exponent);

    if (wavetable[i] > max_value) {
      max_value = wavetable[i];
    }
  }

  for (int i = 0; i < size; i++) {
    wavetable[i] /= max_value;
  }

  return wavetable;
}

/**
 * Creates a variant of the Gaussian wavetable suitable for high-quality
 * granular synthesis. This is a "window" function optimized to prevent spectral
 * leakage and artifacts.
 *
 * @param size The size of the wavetable (typically a power of 2 like 512)
 * @param alpha Parameter controlling the shape (typically between 2-4)
 * @return Pointer to the allocated wavetable, or NULL if allocation fails
 */
double *create_grain_window_wavetable(double *wavetable, int size,
                                      double alpha) {
  for (int i = 0; i < size; i++) {
    double x = (i / (double)(size - 1)) * 2.0 - 1.0; // -1 to 1
    double exponent = -alpha * x * x;
    wavetable[i] = exp(exponent);
  }

  return wavetable;
}

#ifdef READ_WTABLES
static double gaussian_win[SQ_TABSIZE] = {
#include "./assets/gaussian_win.csv"
};
#else
static double gaussian_win[GRAIN_WINDOW_TABSIZE];
#endif

#ifdef READ_WTABLES
double grain_win[SQ_TABSIZE] = {
#include "./assets/grain_win.csv"
};
#else
double grain_win[GRAIN_WINDOW_TABSIZE];
#endif
void maketable_grain_window() {

#ifndef READ_WTABLES
  create_gaussian_wavetable(gaussian_win, GRAIN_WINDOW_TABSIZE, 0.5);
  save_table_to_file(gaussian_win, GRAIN_WINDOW_TABSIZE,
                     "engine/assets/gaussian_win.csv");
  create_grain_window_wavetable(grain_win, GRAIN_WINDOW_TABSIZE, 2.);
  save_table_to_file(grain_win, GRAIN_WINDOW_TABSIZE,
                     "engine/assets/grain_win.csv");
#endif
}

typedef struct grain_osc_state {
  const int max_grains;
  int active_grains;
} grain_osc_state;

double pow2table_read(double pos, int tabsize, double *table) {
  int mask = tabsize - 1;

  double env_pos = pos * (mask);
  int env_idx = (int)env_pos;
  double env_frac = env_pos - env_idx;

  // Interpolate between envelope table values
  double env_val = table[env_idx & mask] * (1.0 - env_frac) +
                   table[(env_idx + 1) & mask] * env_frac;
  return env_val;
}

void *grain_osc_perform(Node *node, grain_osc_state *state, Node *inputs[],
                        int nframes, double spf) {

  double *out = node->output.buf;
  int out_layout = node->output.layout;

  double *buf = inputs[0]->output.buf;
  int buf_size = inputs[0]->output.size;

  double *trig = inputs[1]->output.buf;
  double *pos = inputs[2]->output.buf;
  double *rate = inputs[3]->output.buf;
  double *width = inputs[4]->output.buf;

  int max_grains = state->max_grains;

  char *mem = (char *)((grain_osc_state *)state + 1);
  double *rates = (double *)mem;
  mem += sizeof(double) * max_grains;

  double *phases = (double *)mem;
  mem += sizeof(double) * max_grains;

  double *widths = (double *)mem;
  mem += sizeof(double) * max_grains;

  double *remaining_secs = (double *)mem;
  mem += sizeof(double) * max_grains;

  double *starts = (double *)mem;
  mem += sizeof(double) * max_grains;

  int *active = (int *)mem;

  double d_index;
  int index;
  double frac;
  double a, b;
  double sample = 0.;
  const int table_mask = GRAIN_WINDOW_TABSIZE - 1;
  while (nframes--) {
    sample = 0.;
    if (*trig > 0.9 && state->active_grains < max_grains) {
      for (int i = 0; i < max_grains; i++) {
        if (active[i] == 0) {
          rates[i] = *rate;
          phases[i] = 0;
          starts[i] = *pos * buf_size;
          widths[i] = *width;
          remaining_secs[i] = *width;
          active[i] = 1;
          state->active_grains++;
          break;
        }
      }
    }

    for (int i = 0; i < max_grains; i++) {

      if (active[i]) {
        double r = rates[i];
        double p = phases[i];
        double s = starts[i];
        double w = widths[i];
        double rem = remaining_secs[i];

        d_index = s + (p * buf_size);

        index = (int)d_index;
        frac = d_index - index;

        a = buf[index % buf_size];
        b = buf[(index + 1) % buf_size];

        double grain_elapsed = 1.0 - (rem / w);
        double env_val =
            pow2table_read(grain_elapsed, GRAIN_WINDOW_TABSIZE, grain_win);

        sample += env_val * ((1.0 - frac) * a + (frac * b));
        phases[i] += (r / buf_size);

        remaining_secs[i] -= spf;
        if (remaining_secs[i] <= 0) {
          active[i] = 0; // Deactivate the grain
          state->active_grains--;
        }
      }
    }

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }
    trig++;
    rate++;
    pos++;
    width++;
  }
  return node->output.buf;
}

NodeRef grain_osc_node(int max_grains, Node *buf, Node *trig, Node *pos,
                       Node *rate, Node *width) {

  AudioGraph *graph = _graph;
  int state_size = sizeof(grain_osc_state) +
                   (max_grains * sizeof(double))   // rates
                   + (max_grains * sizeof(double)) // phases
                   + (max_grains * sizeof(double)) // widths
                   + (max_grains * sizeof(double)) // elapsed
                   + (max_grains * sizeof(double)) // starts
                   + (max_grains * sizeof(int))    // active grains
      ;
  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)grain_osc_perform,
      .node_index = node->node_index,
      .num_inputs = 5,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "grain_osc",
  };

  grain_osc_state *state =
      (grain_osc_state *)(graph->nodes_state_memory + node->state_offset);
  *((int *)&state->max_grains) = max_grains; // Cast away const to initialize
  state->active_grains = 0;

  node->connections[0].source_node_index = buf->node_index;
  node->connections[1].source_node_index = trig->node_index;
  node->connections[2].source_node_index = pos->node_index;
  node->connections[3].source_node_index = rate->node_index;
  node->connections[4].source_node_index = width->node_index;

  node->state_ptr = state;
  return node;
}

typedef struct array_choose_trig_state {
  int size;
  double *data;
  double sample;
} array_choose_trig_state;

void *array_choose_trig_perform(Node *node, array_choose_trig_state *state,
                                Node *inputs[], int nframes, double spf) {

  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double sample;
  while (nframes--) {

    if (*trig > 0.9) {
      state->sample = state->data[rand() % state->size];
    }

    *out = state->sample;
    trig++;
    out++;
  }
}

NodeRef array_choose_trig_node(int arr_size, double *arr_data, Node *trig) {

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(array_choose_trig_state));

  *node = (Node){
      .perform = (perform_func_t)array_choose_trig_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(array_choose_trig_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(array_choose_trig_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "array_choose_trig",
  };

  array_choose_trig_state *state =
      (array_choose_trig_state *)(graph->nodes_state_memory +
                                  node->state_offset);
  state->size = arr_size;
  state->data = arr_data;
  state->sample = state->data[rand() % state->size];

  node->connections[0].source_node_index = trig->node_index;
  node->state_ptr = state;
  return node;
}
