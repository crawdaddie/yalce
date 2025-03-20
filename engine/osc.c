#include "./osc.h"
#include "./common.h"
#include "./node.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>

#define SQ_TABSIZE (1 << 11)
#define SIN_TABSIZE (1 << 11)
double sq_table[SQ_TABSIZE];

// Function to save sq_table to a file
int save_table_to_file(double *table, int size, const char *filename,
                       const char *TAB_SIZE_NAME, const char *tab_name) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    return -1; // Error opening file
  }

  fprintf(file, "// Generated table with %d entries\n", size);
  fprintf(file, "#define %s %d\n", TAB_SIZE_NAME, size);
  fprintf(file, "static const double %s[%d] = {\n", tab_name, size);

  for (int i = 0; i < size; i++) {
    fprintf(file, "    %.16f", table[i]);
    if (i < size - 1) {
      fprintf(file, ",");
    }
    fprintf(file, "\n");
  }

  fprintf(file, "};\n");
  fclose(file);
  return 0;
}
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
  save_table_to_file(sq_table, SQ_TABSIZE, "sq_table.h", "SQ_TABSIZE",
                     "sq_table");
}

double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }

  save_table_to_file(sq_table, SQ_TABSIZE, "sin_table.h", "SIN_TABSIZE",
                     "sin_table");
}

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
  // Find next available slot in nodes array
  Node *node = allocate_node_in_graph(graph, sizeof(sin_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)sin_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      // Allocate state memory
      .state_size = sizeof(sin_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(sin_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sin",
  };

  // Initialize state
  sin_state *state = (sin_state *)(state_ptr(graph, node));

  *state = (sin_state){.phase = 0.0};

  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

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

  // Initialize node
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

  // Initialize state
  sq_state *state =
      (sq_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (sq_state){.phase = 0.0};

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

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

    // Output the current phase directly
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

  // Initialize node
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

  // Initialize state
  phasor_state *state =
      (phasor_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (phasor_state){.phase = 0.0};

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

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

  // Initialize node
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

  // Initialize state
  raw_osc_state *state =
      (raw_osc_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (raw_osc_state){.phase = 0.0};

  // Connect inputs
  if (freq) {
    node->connections[0].source_node_index = freq->node_index;
  }
  if (table) {
    node->connections[1].source_node_index = table->node_index;
  }

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

  double a = sin_table[index % table_size];
  double b = sin_table[(index + 1) % table_size];

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

    output /= norm; // Normalize output based on amplitudes

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

  // Initialize node
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

  // Initialize state
  osc_bank_state *state =
      (osc_bank_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (osc_bank_state){.phase = 0.0};

  // Connect inputs
  if (freq) {
    node->connections[0].source_node_index = freq->node_index;
  }
  if (amps) {
    node->connections[1].source_node_index = amps->node_index;
  }

  return node;
}

// Buffer player
typedef struct bufplayer_state {
  double phase;
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

  // Initialize node
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

  // Initialize state
  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  // Connect inputs
  if (buf) {
    node->connections[0].source_node_index = buf->node_index;
  }
  if (rate) {
    node->connections[1].source_node_index = rate->node_index;
  }

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
    // Calculate buffer position
    int buf_frames = buf_size; // Number of frames in buffer
    double d_index = state->phase * buf_frames;
    int index = (int)d_index;
    double frac = d_index - index;

    // Get base positions for the current frame in buffer (accounting for
    // layout)
    int pos_a = (index % buf_frames) * buf_layout;
    int pos_b = ((index + 1) % buf_frames) * buf_layout;

    // Process each channel
    for (int ch = 0; ch < out_layout; ch++) {
      // Get the channel value from the buffer
      // If output has more channels than input, repeat the last input channel
      int buf_ch = (ch < buf_layout) ? ch : (buf_layout - 1);

      double a = buf[pos_a + buf_ch];
      double b = buf[pos_b + buf_ch];

      // Linear interpolation
      double sample = (1.0 - frac) * a + (frac * b);

      // Write to output
      out[frame * out_layout + ch] = sample;
    }

    // Advance phase
    state->phase = fmod(state->phase + rate[frame] / buf_frames, 1.0);
  }

  return node->output.buf;
}

Node *bufplayer_node(Node *buf, Node *rate) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  // Initialize node
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

  // Initialize state
  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  // Connect inputs
  if (buf) {
    node->connections[0].source_node_index = buf->node_index;
  }
  if (rate) {
    node->connections[1].source_node_index = rate->node_index;
  }

  return node;
}

// Buffer player with trigger
void *bufplayer_trig_perform(Node *node, bufplayer_state *state, Node *inputs[],
                             int nframes, double spf) {
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
    // Reset phase on trigger
    if (*trig == 1.0) {
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

    rate++;
    trig++;
    start_pos++;
  }

  return node->output.buf;
}

Node *bufplayer_trig_node(Node *buf, Node *rate, Node *start_pos, Node *trig) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(bufplayer_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)bufplayer_trig_perform,
      .node_index = node->node_index,
      .num_inputs = 4,
      .state_size = sizeof(bufplayer_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(bufplayer_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "bufplayer_trig",
  };

  // Initialize state
  bufplayer_state *state =
      (bufplayer_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (bufplayer_state){.phase = 0.0};

  // Connect inputs
  if (buf) {
    node->connections[0].source_node_index = buf->node_index;
  }
  if (rate) {
    node->connections[1].source_node_index = rate->node_index;
  }
  if (trig) {
    node->connections[2].source_node_index = trig->node_index;
  }
  if (start_pos) {
    node->connections[3].source_node_index = start_pos->node_index;
  }

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

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)white_noise_perform,
      .node_index = node->node_index,
      .num_inputs = 0,
      .state_size = 0, // No state needed
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

    // Prevent unbounded drift
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

  // Initialize node
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

  // Initialize state
  brown_noise_state *state =
      (brown_noise_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (brown_noise_state){.last = 0.0};

  return node;
}

// Exponential chirp generator
// typedef struct chirp_state {
//   double current_freq;
//   double target_freq;
//   double elapsed_time;
//   int trigger_active;
//   double start_freq;
//   double end_freq;
// } chirp_state;
//
// void *chirp_perform(Node *node, chirp_state *state, Node *inputs[], int
// nframes,
//                     double spf) {
//   double *out = node->output.buf;
//   int out_layout = node->output.layout;
//   double *trig = inputs[0]->output.buf;
//   double *lag = inputs[1]->output.buf;
//
//   while (nframes--) {
//     double lag_time = *lag;
//
//     if (*trig == 1.0) {
//       state->trigger_active = 1;
//       state->current_freq = state->start_freq;
//       state->elapsed_time = 0.0;
//     }
//
//     if (state->trigger_active) {
//       // Calculate progress (0 to 1)
//       double progress = state->elapsed_time / lag_time;
//       if (progress > 1.0)
//         progress = 1.0;
//
//       // Use exponential interpolation for frequency
//       state->current_freq = state->start_freq *
//                             pow(state->end_freq / state->start_freq,
//                             progress);
//
//       // Update elapsed time
//       state->elapsed_time += spf;
//
//       // Check if we've reached the end of the chirp
//       if (state->elapsed_time >= lag_time) {
//         state->trigger_active = 0;
//         state->current_freq = state->end_freq;
//       }
//     }
//
//     for (int i = 0; i < out_layout; i++) {
//       *out = state->current_freq;
//       out++;
//     }
//
//     lag++;
//     trig++;
//   }
//
//   return node->output.buf;
// }
//
// Node *chirp_node(double start_freq, double end_freq, Node *lag_time,
//                  Node *trig) {
//   AudioGraph *graph = _graph;
//   Node *node = allocate_node_in_graph(graph, sizeof(chirp_state));
//
//   // Initialize node
//   *node = (Node){
//       .perform = (perform_func_t)chirp_perform,
//       .node_index = node->node_index,
//       .num_inputs = 2,
//       .state_size = sizeof(chirp_state),
//       .state_offset = state_offset_ptr_in_graph(graph, sizeof(chirp_state)),
//       .output = (Signal){.layout = 1,
//                          .size = BUF_SIZE,
//                          .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
//       .meta = "chirp",
//   };
//
//   // Initialize state
//   chirp_state *state =
//       (chirp_state *)(graph->nodes_state_memory + node->state_offset);
//   *state = (chirp_state){.current_freq = start_freq,
//                          .target_freq = end_freq,
//                          .trigger_active = 0,
//                          .start_freq = start_freq,
//                          .end_freq = end_freq,
//                          .elapsed_time = 0.0};
//
//   // Connect inputs
//   if (trig) {
//     node->connections[0].source_node_index = trig->node_index;
//   }
//   if (lag_time) {
//     node->connections[1].source_node_index = lag_time->node_index;
//   }
//
//   return node;
// }
typedef struct chirp_state {
  double start_freq;
  double end_freq;
  double current_freq;
  double elapsed_time;
  int trigger_active;
  double prev_trig_value; // Added to track previous trigger value
} chirp_state;

void *chirp_perform(Node *node, chirp_state *state, Node *inputs[], int nframes,
                    double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *trig = inputs[0]->output.buf;
  double *lag = inputs[1]->output.buf;

  for (int i = 0; i < nframes; i++) {
    double trig_value = trig[i];
    double lag_time = lag[i];

    // Detect both impulses (value == 1.0) and rising edges (prev <= 0 &&
    // current > 0)
    if (((trig_value == 1.0) && (state->prev_trig_value != 1.0)) ||
        (state->prev_trig_value <= 0.5 && trig_value > 0.5)) {

      state->trigger_active = 1;
      state->current_freq = state->start_freq;
      state->elapsed_time = 0.0;
    }

    // Store current trigger value for next iteration
    state->prev_trig_value = trig_value;

    if (state->trigger_active) {
      // Calculate progress (0 to 1)
      double progress = state->elapsed_time / lag_time;
      if (progress > 1.0)
        progress = 1.0;

      // Use exponential interpolation for frequency
      state->current_freq = state->start_freq *
                            pow(state->end_freq / state->start_freq, progress);

      // Update elapsed time
      state->elapsed_time += spf;

      // Check if we've reached the end of the chirp
      if (state->elapsed_time >= lag_time) {
        state->trigger_active = 0;
        state->current_freq = state->end_freq;
      }
    }

    // Write to all output channels
    for (int ch = 0; ch < out_layout; ch++) {
      out[i * out_layout + ch] = state->current_freq;
    }
  }

  return node->output.buf;
}

Node *chirp_node(double start_freq, double end_freq, Node *lag_input,
                 Node *trig_input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(chirp_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)chirp_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(chirp_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(chirp_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "chirp",
  };

  // Initialize state
  chirp_state *state =
      (chirp_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (chirp_state){
      .start_freq = start_freq,
      .end_freq = end_freq,
      .current_freq = start_freq,
      .elapsed_time = 0.0,
      .trigger_active = 0,
      .prev_trig_value = 0.0 // Initialize the previous trigger value
  };

  // Connect inputs
  if (trig_input) {
    node->connections[0].source_node_index = trig_input->node_index;
  }
  if (lag_input) {
    node->connections[1].source_node_index = lag_input->node_index;
  }

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

    // Generate impulse when phase is exactly 1.0
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

  // Initialize node
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

  // Initialize state
  impulse_state *state =
      (impulse_state *)(graph->nodes_state_memory + node->state_offset);
  *state =
      (impulse_state){.phase = 1.0}; // Start at 1.0 to get immediate impulse

  // Connect input
  if (freq) {
    node->connections[0].source_node_index = freq->node_index;
  }

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

  // Initialize node
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

  // Initialize state
  ramp_state *state =
      (ramp_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (ramp_state){.phase = 0.0};

  // Connect input
  if (freq) {
    node->connections[0].source_node_index = freq->node_index;
  }

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

  // Initialize node
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

  // Initialize state
  trig_rand_state *state =
      (trig_rand_state *)(graph->nodes_state_memory + node->state_offset);
  *state = (trig_rand_state){.value = _random_double_range(0.0, 1.0)};

  // Connect input
  if (trig) {
    node->connections[0].source_node_index = trig->node_index;
  }

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

  // Initialize node
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

  // Initialize state
  trig_sel_state *state =
      (trig_sel_state *)(graph->nodes_state_memory + node->state_offset);

  // Connect inputs
  if (trig) {
    node->connections[0].source_node_index = trig->node_index;
  }
  if (sels) {
    node->connections[1].source_node_index = sels->node_index;
    // Initialize value with a random selection
    double *sels_buf = sels->output.buf;
    int sels_size = sels->output.size;
    state->value = sels_buf[rand() % sels_size];
  } else {
    state->value = 0.0;
  }

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
  // Linear interpolation
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
    // Apply envelope
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
    // Reset the output for this sample
    double sample = 0.0;

    // Check for new grain trigger
    if (*trig == 1.0 && state->active_grains < state->max_concurrent_grains) {
      double p = fabs(*pos);
      int start_pos = (int)(p * buf_size);
      int length = state->min_grain_length +
                   rand() % (state->max_grain_length - state->min_grain_length);

      // Initialize new grain
      int index = state->next_free_grain;
      init_grain(state, index, start_pos, length, *rate);

      // Find next free grain slot
      do {
        state->next_free_grain =
            (state->next_free_grain + 1) % state->max_concurrent_grains;
      } while (state->lengths[state->next_free_grain] != 0 &&
               state->next_free_grain != index);
    }

    // Process all active grains
    for (int i = 0; i < state->max_concurrent_grains; i++) {
      if (state->lengths[i] > 0) {
        process_grain(state, i, &sample, buf, buf_size);
      }
    }

    // Normalize output
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

  // Limit max grains to reasonable size
  if (max_grains <= 0)
    max_grains = 8;
  if (max_grains > 32)
    max_grains = 32;

  // Calculate state size (fixed)
  size_t state_size = sizeof(granulator_state);

  // Initialize node
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

  // Initialize state
  granulator_state *state =
      (granulator_state *)(graph->nodes_state_memory + node->state_offset);

  // Initialize grain parameters
  state->max_concurrent_grains = max_grains;
  state->active_grains = 0;
  state->min_grain_length = 7000;
  state->max_grain_length = 7001;
  state->overlap = 0.3;
  state->next_free_grain = 0;

  // Clear grain arrays
  for (int i = 0; i < max_grains; i++) {
    state->starts[i] = 0;
    state->lengths[i] = 0;
    state->positions[i] = 0;
    state->rates[i] = 1.0;
    state->amps[i] = 0.0;
  }

  // Connect inputs
  if (buf) {
    node->connections[0].source_node_index = buf->node_index;
  }
  if (trig) {
    node->connections[1].source_node_index = trig->node_index;
  }
  if (pos) {
    node->connections[2].source_node_index = pos->node_index;
  }
  if (rate) {
    node->connections[3].source_node_index = rate->node_index;
  }

  return node;
}

typedef struct fm_state {
  double carrier_phase;
  double modulator_phase;
  double modulator_to_carrier_ratio;
  double modulation_index;
} fm_state;

void *fm_perform_optimized(Node *node, fm_state *state, Node **inputs,
                           int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get frequency input
  double *freq_in = inputs[0]->output.buf;

  // Get modulation inputs if connected
  double *mod_index_in = inputs[1] ? inputs[1]->output.buf : NULL;
  double *mod_ratio_in = inputs[2] ? inputs[2]->output.buf : NULL;

  // Precompute table size constants
  const double table_size = (double)SIN_TABSIZE;
  const int table_mask = SIN_TABSIZE - 1; // Assuming SIN_TABSIZE is power of 2

  while (nframes--) {
    // Get parameters
    double carrier_freq = *freq_in++;
    double mod_index = mod_index_in ? *mod_index_in++ : state->modulation_index;
    double mod_ratio =
        mod_ratio_in ? *mod_ratio_in++ : state->modulator_to_carrier_ratio;

    // Calculate modulator frequency
    double modulator_freq = carrier_freq * mod_ratio;

    // Get modulator sample with interpolation
    double mod_phase_scaled = state->modulator_phase * table_size;
    int mod_index_int = (int)mod_phase_scaled;
    double mod_frac = mod_phase_scaled - mod_index_int;

    mod_index_int &= table_mask; // Fast modulo for power of 2
    int mod_index_next = (mod_index_int + 1) & table_mask;

    double mod_a = sin_table[mod_index_int];
    double mod_b = sin_table[mod_index_next];
    double modulator_value =
        ((1.0 - mod_frac) * mod_a + mod_frac * mod_b) * mod_index;

    // Calculate carrier phase with modulation
    double modulated_phase = state->carrier_phase + modulator_value;
    modulated_phase =
        modulated_phase - floor(modulated_phase); // Wrap to 0-1 range

    // Get carrier sample with interpolation
    double carrier_phase_scaled = modulated_phase * table_size;
    int carrier_index_int = (int)carrier_phase_scaled;
    double carrier_frac = carrier_phase_scaled - carrier_index_int;

    carrier_index_int &= table_mask; // Fast modulo for power of 2
    int carrier_index_next = (carrier_index_int + 1) & table_mask;

    double carrier_a = sin_table[carrier_index_int];
    double carrier_b = sin_table[carrier_index_next];
    double carrier_value =
        (1.0 - carrier_frac) * carrier_a + carrier_frac * carrier_b;

    // Output carrier value
    for (int i = 0; i < out_layout; i++) {
      *out++ = carrier_value;
    }

    // Update phases
    state->modulator_phase =
        fmod(state->modulator_phase + modulator_freq * spf, 1.0);
    state->carrier_phase = fmod(state->carrier_phase + carrier_freq * spf, 1.0);
  }

  return node->output.buf;
}
Node *fm_node(Node *freq_input, Node *mod_index_input, Node *mod_ratio_input) {
  AudioGraph *graph = _graph;
  // Find next available slot in nodes array
  Node *node = allocate_node_in_graph(graph, sizeof(fm_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)fm_perform_optimized,
      .node_index = node->node_index,
      .num_inputs = 3, // frequency, mod index, mod ratio
      // Allocate state memory
      .state_size = sizeof(fm_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(fm_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "fm",
  };

  // Initialize state
  fm_state *state = (fm_state *)(state_ptr(graph, node));

  *state = (fm_state){
      .carrier_phase = 0.0,
      .modulator_phase = 0.0,
      .modulator_to_carrier_ratio = 1.0, // Default 1:1 ratio
      .modulation_index = 5.0,           // Default modulation index
  };

  // Connect inputs if provided
  if (freq_input) {
    node->connections[0].source_node_index = freq_input->node_index;
  }

  if (mod_index_input) {
    node->connections[1].source_node_index = mod_index_input->node_index;
  }

  if (mod_ratio_input) {
    node->connections[2].source_node_index = mod_ratio_input->node_index;
  }

  return node;
}
