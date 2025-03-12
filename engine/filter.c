#include "./filter.h"
#include "./common.h"
#include "./ctx.h"
#include "./node.h"
#include "audio_graph.h"
#include <math.h>

// Biquad filter state
typedef struct biquad_state {
  double b0, b1, b2; // Feedforward coefficients
  double a1, a2;     // Feedback coefficients
  double x1, x2;     // Input delay elements
  double y1, y2;     // Output delay elements
  double prev_freq;  // Previous frequency value for coefficient updates
  double prev_res;   // Previous resonance value
} biquad_state;

// Initialize filter coefficients and state variables
static void set_biquad_filter_state(biquad_state *filter, double b0, double b1,
                                    double b2, double a1, double a2) {
  filter->b0 = b0;
  filter->b1 = b1;
  filter->b2 = b2;
  filter->a1 = a1;
  filter->a2 = a2;
}

// Zero filter state variables
static void zero_biquad_filter_state(biquad_state *filter) {
  filter->x1 = 0.0;
  filter->x2 = 0.0;
  filter->y1 = 0.0;
  filter->y2 = 0.0;
}

// Low-pass filter coefficient calculation
static void set_biquad_lp_coefficients(double freq, double res, int fs,
                                       biquad_state *state) {
  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Quality factor

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double C = cos(w0);
  double b0 = (1 - C) / 2;
  double b1 = 1 - C;
  double b2 = (1 - C) / 2;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Band-pass filter coefficient calculation
static void set_biquad_bp_coefficients(double freq, double res, int fs,
                                       biquad_state *state) {
  double fc = freq; // Center frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Q factor for resonance

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double C = cos(w0);
  double b0 = A;
  double b1 = 0.0;
  double b2 = -A;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// High-pass filter coefficient calculation
static void set_biquad_hp_coefficients(double freq, double res, int fs,
                                       biquad_state *state) {
  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Quality factor

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double C = cos(w0);
  double b0 = (1 + C) / 2;
  double b1 = -(1 + C);
  double b2 = (1 + C) / 2;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Butterworth high-pass filter coefficient calculation
static void set_butterworth_hp_coefficients(double freq, int fs,
                                            biquad_state *state) {
  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double wc = tan(w0 / 2);
  double k = wc * wc;
  double sqrt2 = sqrt(2.0);

  // Compute filter coefficients
  double b0 = 1 / (1 + sqrt2 * wc + k);
  double b1 = -2 * b0;
  double b2 = b0;
  double a1 = 2 * (k - 1) * b0;
  double a2 = (1 - sqrt2 * wc + k) * b0;

  // Initialize filter
  set_biquad_filter_state(state, b0, b1, b2, a1, a2);
}

// Biquad filter perform function
void *biquad_perform(Node *node, biquad_state *state, Node *inputs[],
                     int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  while (nframes--) {
    double input = *in;
    double output = state->b0 * input + state->b1 * state->x1 +
                    state->b2 * state->x2 - state->a1 * state->y1 -
                    state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;
    in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad low-pass filter perform function
void *biquad_lp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *freq_in = inputs[1]->output.buf;
  double *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  double freq = *freq_in;
  double res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_lp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_lp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    double input = *in;
    double output = state->b0 * input + state->b1 * state->x1 +
                    state->b2 * state->x2 - state->a1 * state->y1 -
                    state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad band-pass filter perform function
void *biquad_bp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *freq_in = inputs[1]->output.buf;
  double *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  double freq = *freq_in;
  double res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_bp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_bp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    double input = *in;
    double output = state->b0 * input + state->b1 * state->x1 +
                    state->b2 * state->x2 - state->a1 * state->y1 -
                    state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad high-pass filter perform function
void *biquad_hp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *freq_in = inputs[1]->output.buf;
  double *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  double freq = *freq_in;
  double res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_hp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_hp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    double input = *in;
    double output = state->b0 * input + state->b1 * state->x1 +
                    state->b2 * state->x2 - state->a1 * state->y1 -
                    state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Butterworth high-pass filter perform function
void *butterworth_hp_dyn_perform(Node *node, biquad_state *state,
                                 Node *inputs[], int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *freq_in = inputs[1]->output.buf;

  // Initial check and coefficient update
  double freq = *freq_in;

  // Update coefficients if frequency changed
  if (freq != state->prev_freq) {
    set_butterworth_hp_coefficients(freq, (int)(1.0 / spf), state);
    state->prev_freq = freq;
  }

  while (nframes--) {
    // Check if frequency changed
    freq = *freq_in;

    if (freq != state->prev_freq) {
      set_butterworth_hp_coefficients(freq, (int)(1.0 / spf), state);
      state->prev_freq = freq;
    }

    double input = *in;
    double output = state->b0 * input + state->b1 * state->x1 +
                    state->b2 * state->x2 - state->a1 * state->y1 -
                    state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    out++;
  }

  return node->output.buf;
}

// -------------------- Node Creation Functions --------------------

// Create a biquad low-pass filter node
Node *biquad_lp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_lp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_lp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }
  if (freq) {
    node->connections[1].source_node_index = freq->node_index;
  }
  if (res) {
    node->connections[2].source_node_index = res->node_index;
  }

  return node;
}

// Create a biquad band-pass filter node
Node *biquad_bp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_bp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_bp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }
  if (freq) {
    node->connections[1].source_node_index = freq->node_index;
  }
  if (res) {
    node->connections[2].source_node_index = res->node_index;
  }

  return node;
}

// Create a biquad high-pass filter node
Node *biquad_hp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_hp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_hp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }
  if (freq) {
    node->connections[1].source_node_index = freq->node_index;
  }
  if (res) {
    node->connections[2].source_node_index = res->node_index;
  }

  return node;
}

// Create a butterworth high-pass filter node
Node *butterworth_hp_node(Node *freq, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)butterworth_hp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "butterworth_hp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;

  // Connect inputs
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }
  if (freq) {
    node->connections[1].source_node_index = freq->node_index;
  }

  return node;
}

// ---------------------- Comb Filter ---------------------------

typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
  double fb;
} comb_state;

void *comb_perform(Node *node, comb_state *state, Node *inputs[], int nframes,
                   double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  while (nframes--) {
    // Get write and read pointers
    double *write_ptr = state->buf + state->write_pos;
    double *read_ptr = state->buf + state->read_pos;

    // Calculate output and write to buffer
    *out = *in + *read_ptr;
    *write_ptr = state->fb * (*out);

    // Update positions
    state->read_pos = (state->read_pos + 1) % state->buf_size;
    state->write_pos = (state->write_pos + 1) % state->buf_size;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input) {
  AudioGraph *graph = _graph;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate);

  // Allocate node
  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  // Allocate state
  node->state_size = sizeof(comb_state) + (bufsize * sizeof(double));
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  // Get state pointer and buffer pointer
  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);
  double *buf = (double *)((char *)state + sizeof(comb_state));

  // Initialize buffer
  for (int i = 0; i < bufsize; i++) {
    buf[i] = 0.0;
  }

  // Initialize state
  state->buf = buf;
  state->buf_size = bufsize;
  state->fb = fb;
  state->write_pos = 0;
  state->read_pos = state->buf_size - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)comb_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "comb",
  };

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

  return node;
}

// ---------------------- Lag Filter ---------------------------

typedef struct {
  double current_value;
  double target_value;
  double coeff;
  double lag_time;
} lag_state;

void *lag_perform(Node *node, lag_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  // Check if lag time changed
  if (fabs(state->lag_time - state->coeff) > 1e-6) {
    state->coeff = exp(-1.0 / (state->lag_time * (1.0 / spf)));
  }

  while (nframes--) {
    state->target_value = *in;

    // Update current value with lag
    state->current_value =
        state->current_value +
        (state->target_value - state->current_value) * (1.0 - state->coeff);

    // Write output
    *out = state->current_value;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *lag_node(double lag_time, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(lag_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)lag_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(lag_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(lag_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lag",
  };

  // Initialize state
  lag_state *state =
      (lag_state *)(graph->nodes_state_memory + node->state_offset);
  state->current_value = 0.0;
  state->target_value = 0.0;
  state->lag_time = lag_time;
  double spf = 1.0 / ctx_sample_rate();
  state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

  return node;
}

// ---------------------- Tanh Distortion ---------------------------

typedef struct {
  double gain;
} tanh_state;

void *tanh_perform(Node *node, tanh_state *state, Node *inputs[], int nframes,
                   double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  double gain = state->gain;

  while (nframes--) {
    *out = tanh(*in * gain);
    in++;
    out++;
  }

  return node->output.buf;
}

Node *tanh_node(double gain, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(tanh_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)tanh_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(tanh_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(tanh_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "tanh",
  };

  // Initialize state
  tanh_state *state =
      (tanh_state *)(graph->nodes_state_memory + node->state_offset);
  state->gain = gain;

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

  return node;
}
