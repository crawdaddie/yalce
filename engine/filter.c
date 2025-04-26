#include "./filter.h"
#include "./common.h"
#include "./ctx.h"
#include "./node.h"
#include "./primes.h"
#include "audio_graph.h"
#include "node_util.h"
#include "osc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = freq->node_index;
  node->connections[2].source_node_index = res->node_index;

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
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = freq->node_index;
  node->connections[2].source_node_index = res->node_index;

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
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = freq->node_index;
  node->connections[2].source_node_index = res->node_index;

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
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = freq->node_index;

  return node;
}

// ---------------------- Comb Filter ---------------------------

typedef struct {
  int read_pos;
  int write_pos;
  double fb;
} comb_state;

void *comb_perform(Node *node, comb_state *state, Node *inputs[], int nframes,
                   double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  double *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  while (nframes--) {
    // Get write and read pointers
    double *write_ptr = buf + state->write_pos;
    double *read_ptr = buf + state->read_pos;

    // Calculate output and write to buffer
    *out = *in + *read_ptr;
    *write_ptr = state->fb * (*out);

    // Update positions
    state->read_pos = (state->read_pos + 1) % bufsize;
    state->write_pos = (state->write_pos + 1) % bufsize;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input) {
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  node->state_size = sizeof(comb_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;
  state->read_pos = bufsize - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)comb_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "comb",
  };

  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = delay_buf->node_index;

  return node;
}

// ---------------------- Dyn Comb Filter ---------------------------

void *dyn_comb_perform(Node *node, comb_state *state, Node *inputs[],
                       int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *delay_buf = inputs[1]->output.buf;
  int buf_size = inputs[1]->output.size;
  double *delay_time = inputs[2]->output.buf;
  double sample_rate =
      1.0 / spf; // Calculate sample rate from seconds per frame

  while (nframes--) {
    int write_pos = state->write_pos;

    delay_buf[write_pos] = *in + (state->fb * delay_buf[state->read_pos]);

    double delay_samples = *delay_time * sample_rate;
    int read_offset = (int)delay_samples;
    double frac =
        delay_samples - read_offset; // Fractional part for interpolation

    // Ensure read position stays within buffer bounds with proper modulo
    int read_pos = (write_pos - read_offset + buf_size) % buf_size;
    int read_pos_next = (read_pos + 1) % buf_size;

    // Linear interpolation for smoother delay time changes
    double sample =
        delay_buf[read_pos] * (1.0 - frac) + delay_buf[read_pos_next] * frac;

    *out = sample;
    *out += *in;

    state->read_pos = read_pos;
    state->write_pos = (write_pos + 1) % buf_size;

    in++;
    out++;
    delay_time++;
  }

  return node->output.buf;
}

Node *dyn_comb_node(Node *delay_time, double max_delay_time, double fb,
                    Node *input) {
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  // Allocate node
  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  // Allocate state
  node->state_size = sizeof(comb_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  // Get state pointer and buffer pointer
  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)dyn_comb_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "comb",
  };

  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = delay_buf->node_index;
  node->connections[2].source_node_index = delay_time->node_index;

  return node;
}

// ---------------------- Lag Filter ---------------------------

typedef struct {
  double current_value;
  double target_value;
  double coeff;
  double lag_time;
} lag_state;

// Node *static_lag_node(double lag_time, Node *input) {
//   AudioGraph *graph = _graph;
//   Node *node = allocate_node_in_graph(graph, sizeof(lag_state));
//
//   // Initialize node
//   *node = (Node){
//       .perform = (perform_func_t)lag_perform,
//       .node_index = node->node_index,
//       .num_inputs = 1,
//       .state_size = sizeof(lag_state),
//       .state_offset = state_offset_ptr_in_graph(graph, sizeof(lag_state)),
//       .output = (Signal){.layout = 1,
//                          .size = BUF_SIZE,
//                          .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
//       .meta = "lag",
//   };
//
//   // Initialize state
//   lag_state *state =
//       (lag_state *)(graph->nodes_state_memory + node->state_offset);
//   state->current_value = 0.0;
//   state->target_value = 0.0;
//   state->lag_time = lag_time;
//   double spf = 1.0 / ctx_sample_rate();
//   state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));
//
//   // Connect input
//   node->connections[0].source_node_index = input->node_index;
//
//   return node;
// }
void *lag_perform(Node *node, lag_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *lag_time = inputs[1]->output.buf;

  while (nframes--) {
    double lt = *lag_time;
    lag_time++;

    // Special case: when lag_time is 0, immediately jump to target value
    if (lt <= 0.0) {
      state->current_value = *in;
      state->coeff = 0.0; // Set coefficient to 0 to indicate immediate response
    } else {
      if (fabs(lt - state->lag_time) > 1e-6) {
        // state->lag_time = lt;
        state->coeff = exp(-1.0 / (lt * (1.0 / spf)));
      }

      // Store target value
      state->target_value = *in;

      if (lt > 0.0) {
        // Normal lag behavior: update current value with interpolation
        state->current_value =
            state->current_value +
            (state->target_value - state->current_value) * (1.0 - state->coeff);
      }
    }

    // Write output
    *out = state->current_value;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *lag_node(NodeRef lag_time, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(lag_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)lag_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
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
  // state->lag_time = 0.0;
  state->coeff = 0.0; // Initialize coefficient to 0

  // Connect input
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = lag_time->node_index;

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
  node->connections[0].source_node_index = input->node_index;

  return node;
}

void *dyn_tanh_perform(Node *node, tanh_state *state, Node *inputs[],
                       int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *gain = inputs[1]->output.buf;

  while (nframes--) {

    *out = tanh(*in * *gain);
    gain++;
    in++;
    out++;
  }

  return node->output.buf;
}

Node *dyn_tanh_node(NodeRef gain, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)dyn_tanh_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = state_offset_ptr_in_graph(graph, 0),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "tanh",
  };

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  node->connections[1].source_node_index = gain->node_index;

  //
  return node;
}

typedef struct {
  int length;
  int pos;
  double coeff;
} allpass_state;

double process_allpass_frame(double *buf, int length, int *pos, double coeff,
                             double in) {
  int bufsize = length;
  double delayed = buf[*pos];
  buf[*pos] = in;
  *pos = (*pos + 1) % bufsize;
  return delayed - coeff * in;
}

void *allpass_perform(Node *node, allpass_state *state, Node *inputs[],
                      int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  char *mem = state + 1;
  double *buf = mem;

  while (nframes--) {
    *out = process_allpass_frame(buf, state->length, &state->pos, state->coeff,
                                 *in);
    in++;
    out++;
  }

  return node->output.buf;
}

static int compute_allpass_delay_length(double sample_rate, double delay_sec) {
  int delay_samples = (int)(delay_sec * sample_rate);
  return delay_samples;
}

Node *allpass_node(double time, double coeff, Node *input) {
  AudioGraph *graph = _graph;
  int state_size = sizeof(allpass_state);
  allpass_state ap = {
      .length = compute_allpass_delay_length(ctx_sample_rate(), time),
      .pos = 0,
      .coeff = coeff,
  };

  state_size += (sizeof(double) * ap.length); // leave space for buffer

  Node *node = allocate_node_in_graph(graph, sizeof(allpass_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)allpass_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "ap",
  };

  // Initialize state
  allpass_state *state =
      (allpass_state *)(graph->nodes_state_memory + node->state_offset);
  *state = ap;

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  return node;
}

#define MAX_DELAY_LENGTH 20000
#define NUM_EARLY_REFLECTIONS 8
#define NUM_ALLPASS 4

static double early_gains[NUM_EARLY_REFLECTIONS] = {1.0, 0.9, 0.8, 0.7,
                                                    0.6, 0.5, 0.4, 0.3};
static double early_scaling_factors[NUM_EARLY_REFLECTIONS] = {
    1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7};
static double early_gain_factors[NUM_EARLY_REFLECTIONS] = {1.0, 0.9, 0.8, 0.7,
                                                           0.6, 0.5, 0.4, 0.3};

static double delay_scaling_factors[4] = {1.0, 0.9, 0.8, 0.7};
static double allpass_scaling_factors[NUM_ALLPASS] = {0.15, 0.12, 0.10, 0.08};

static int next_prime(int n) {
  int i = 0;
  while (PRIMES[i] <= n && i < NUM_PRIMES) {
    i++;
  }
  return PRIMES[i];
}

static int compute_delay_length(double room_size, double sample_rate,
                                double scaling_factor) {
  double delay_sec = (room_size * scaling_factor) / 343.0f;
  int delay_samples = (int)(delay_sec * sample_rate);
  return next_prime(delay_samples);
}

typedef struct GVerb {
  double room_size;
  double reverb_time;
  double damping;
  double input_bandwidth;
  double dry;
  double wet;
  double early_level;
  double tail_level;
  double sample_rate;

  double input_lp_state;
  double input_lp_coeff;
  int early_lengths[NUM_EARLY_REFLECTIONS];
  int early_pos[NUM_EARLY_REFLECTIONS];
  double early_gains[NUM_EARLY_REFLECTIONS];
  int fb_delay_lengths[4];
  int fb_delay_pos[4];
  double damping_coeff;
  double damping_state[4];
  int allpass_lengths[NUM_ALLPASS];
  int allpass_pos[NUM_ALLPASS];
  double allpass_feedback;
} GVerb;

static double lowpass_filter(double input, double *state, double coeff) {
  double output = input * (1.0f - coeff) + (*state * coeff);
  *state = output;
  return output;
}

static double process_early_reflections(GVerb *refl, double **delays,
                                        double input) {

  double early_out = 0.;
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    int bufsize = refl->early_lengths[i];
    double *buf = delays[i];
    int write_pos = refl->early_pos[i] % bufsize;
    early_out += process_allpass_frame(delays[i], bufsize, &refl->early_pos[i],
                                       -1., input);
  }
  return early_out;
}

static double series_allpass_process(GVerb *gverb, double **allpass_delays,
                                     double input) {

  for (int i = 0; i < NUM_ALLPASS; i++) {
    double *delay_line = allpass_delays[i];
    int pos = gverb->allpass_pos[i];
    int length = gverb->allpass_lengths[i];
    input = process_allpass_frame(delay_line, length,
                                  &gverb->allpass_lengths[i], 0.0, input);
  }
  return input;
}

double feedback_delay_network(GVerb *gverb, double **delay_lines, double mix) {
  double delay_out[4];
  for (int i = 0; i < 4; i++) {
    int fb_pos = gverb->fb_delay_pos[i];
    delay_out[i] = delay_lines[i][fb_pos];
  }

  for (int i = 0; i < 4; i++) {
    delay_out[i] = lowpass_filter(delay_out[i], &gverb->damping_state[i],
                                  gverb->damping_coeff);
  }

  double feedback[4];
  feedback[0] = delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3];
  feedback[1] = delay_out[0] + delay_out[1] - delay_out[2] - delay_out[3];
  feedback[2] = delay_out[0] - delay_out[1] + delay_out[2] - delay_out[3];
  feedback[3] = delay_out[0] - delay_out[1] - delay_out[2] + delay_out[3];

  double rt_gain = pow(10.0, -3.0 * (1.0 / gverb->reverb_time));

  for (int i = 0; i < 4; i++) {
    feedback[i] *= 0.25 * rt_gain; /* 0.25 for Hadamard normalization */
  }

  feedback[0] += mix;

  for (int i = 0; i < 4; i++) {
    int pos = gverb->fb_delay_pos[i];
    int len = gverb->fb_delay_lengths[i];
    delay_lines[i][pos] = feedback[i];
    gverb->fb_delay_pos[i] = (pos + 1) % len;
  }
  return delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3];
}

static double allpass_process(double input, double *delay_line, int *pos,
                              int length, double feedback) {
  double delayed = delay_line[*pos];
  double new_input = input + feedback * delayed;
  delay_line[*pos] = new_input;
  *pos = (*pos + 1) % length;
  return delayed - feedback * new_input;
}

void *gverb_perform(Node *node, GVerb *gverb, Node *inputs[], int nframes,
                    double spf) {

  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  char *mem = ((GVerb *)gverb + 1);

  double *early_delays[NUM_EARLY_REFLECTIONS];
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    early_delays[i] = mem;
    mem += (sizeof(double) * gverb->early_lengths[i]);
  }

  double *delay_lines[4];
  for (int i = 0; i < 4; i++) {
    delay_lines[i] = mem;
    mem += (sizeof(double) * gverb->fb_delay_lengths[i]);
  }

  double *allpass_delays[NUM_ALLPASS];
  for (int i = 0; i < NUM_ALLPASS; i++) {
    allpass_delays[i] = mem;
    mem += (sizeof(double) * gverb->allpass_lengths[i]);
  }

  while (nframes--) {
    double filt =
        lowpass_filter(*in, &gverb->input_lp_state, gverb->input_lp_coeff);

    double early_out = process_early_reflections(gverb, early_delays, filt);

    double mix = filt * 0.5 + early_out * 0.5;

    double diffuse_signal = feedback_delay_network(gverb, delay_lines, mix);

    diffuse_signal *= 0.25;

    for (int i = 0; i < NUM_ALLPASS; i++) {
      double *delay_line = allpass_delays[i];
      int pos = gverb->allpass_pos[i];
      int length = gverb->allpass_lengths[i];

      diffuse_signal = allpass_process(diffuse_signal, delay_line, &pos, length,
                                       gverb->allpass_feedback);
      gverb->allpass_pos[i] = pos;
    }

    *out = *in * gverb->dry + early_out * gverb->early_level * gverb->wet +
           diffuse_signal * gverb->tail_level * gverb->wet;
    in++;
    out++;
  }
}

Node *gverb_node(Node *input) {
  double sr = (double)ctx_sample_rate();
  double room_size = 10.;
  GVerb gverb = {
      .room_size = room_size,
      .reverb_time = 10.,
      .damping_coeff = 0.4,
      .input_lp_coeff = 0.4,
      .dry = 0.5,
      .wet = 0.5,
      .early_level = 0.5,
      .tail_level = 0.9,
      .sample_rate = sr,
      .early_pos = {0, 0, 0, 0, 0, 0, 0, 0},
      .early_gains = {1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3},
      .early_lengths =
          {
              compute_delay_length(room_size, sr, 1.0),
              compute_delay_length(room_size, sr, 1.1),
              compute_delay_length(room_size, sr, 1.2),
              compute_delay_length(room_size, sr, 1.3),
              compute_delay_length(room_size, sr, 1.4),
              compute_delay_length(room_size, sr, 1.5),
              compute_delay_length(room_size, sr, 1.6),
              compute_delay_length(room_size, sr, 1.7),
          },
      .fb_delay_pos = {0, 0, 0, 0},
      .fb_delay_lengths =
          {
              compute_delay_length(room_size, sr, delay_scaling_factors[0]),
              compute_delay_length(room_size, sr, delay_scaling_factors[1]),
              compute_delay_length(room_size, sr, delay_scaling_factors[2]),
              compute_delay_length(room_size, sr, delay_scaling_factors[3]),
          },
      .allpass_pos = {0, 0, 0, 0},
      .allpass_lengths =
          {
              compute_delay_length(room_size, sr, allpass_scaling_factors[0]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[1]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[2]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[3]),
          },

  };

  AudioGraph *graph = _graph;
  int state_size = sizeof(GVerb);
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    state_size += (gverb.early_lengths[i] * sizeof(double));
  }

  for (int i = 0; i < 4; i++) {
    state_size += (gverb.fb_delay_lengths[i] * sizeof(double));
  }

  for (int i = 0; i < NUM_ALLPASS; i++) {
    state_size += (gverb.allpass_lengths[i] * sizeof(double));
  }

  state_size = (state_size + 7) & ~7;
  Node *node = allocate_node_in_graph(graph, state_size);
  *node = (Node){
      .perform = (perform_func_t)gverb_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "gverb_lp",
  };

  char *mem = graph->nodes_state_memory + node->state_offset;
  memset(mem, 0, state_size);
  GVerb *state = (GVerb *)(mem);
  *state = gverb;
  node->connections[0].source_node_index = input->node_index;

  return node;
}
typedef struct grain_pitchshift_state {
  int length;
  int pos;
  int max_grains;
  int active_grains;
  int next_trig;
  int trig_gap_in_frames;
  double width;
  double rate;
  double fb;
} grain_pitchshift_state;

double __pow2table_read(double pos, int tabsize, double *table) {
  int mask = tabsize - 1;

  double env_pos = pos * (mask);
  int env_idx = (int)env_pos;
  double env_frac = env_pos - env_idx;

  // Interpolate between envelope table values
  double env_val = table[env_idx & mask] * (1.0 - env_frac) +
                   table[(env_idx + 1) & mask] * env_frac;
  return env_val;
}

void *granular_pitchshift_perform(Node *node, grain_pitchshift_state *state,
                                  Node *inputs[], int nframes, double spf) {

  int out_layout = node->output.layout;
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  char *mem = state + 1;
  double *buf = mem;
  int buf_size = state->length;
  mem += buf_size * sizeof(double);

  int max_grains = state->max_grains;

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
  double r = state->rate;
  const int table_mask = GRAIN_WINDOW_TABSIZE - 1;
  while (nframes--) {
    sample = 0.;
    if ((state->next_trig <= 0) && state->active_grains < max_grains) {
      state->next_trig = state->trig_gap_in_frames;
      for (int i = 0; i < max_grains; i++) {
        if (active[i] == 0) {
          phases[i] = 0;
          starts[i] = state->pos * buf_size;
          widths[i] = state->width;
          remaining_secs[i] = state->width;
          active[i] = 1;
          state->active_grains++;
          break;
        }
      }
    }

    for (int i = 0; i < max_grains; i++) {

      if (active[i]) {
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
            __pow2table_read(grain_elapsed, GRAIN_WINDOW_TABSIZE, grain_win);

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
      *out = sample + *in;
      out++;
    }
    buf[state->pos] = *in + state->fb * *out;
    state->next_trig--;
    state->pos = (state->pos + 1) % buf_size;
  }
}

NodeRef grain_pitchshift_node(double shift, double fb, NodeRef input) {
  AudioGraph *graph = _graph;

  int max_grains = 32;
  int state_size = sizeof(grain_pitchshift_state) +
                   (1024 * sizeof(double))         // delay buffer
                   + (max_grains * sizeof(double)) // phases
                   + (max_grains * sizeof(double)) // widths
                   + (max_grains * sizeof(double)) // elapsed
                   + (max_grains * sizeof(double)) // starts
                   + (max_grains * sizeof(int))    // active grains
      ;

  grain_pitchshift_state pshift = {

      .length = 1024,
      .pos = 0,
      .max_grains = max_grains,
      .active_grains = 0,
      .next_trig = ((double)1024) / shift,
      .trig_gap_in_frames = ((double)1024) / shift,
      .width = 0.01,
      .rate = shift,
      .fb = fb};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)granular_pitchshift_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "grain_pitchshift",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  grain_pitchshift_state *state = mem;
  *state = pshift;

  node->connections[0].source_node_index = input->node_index;

  return node;
}

typedef double (*node_math_func_t)(double samp);
typedef struct math_node_state {
  node_math_func_t math_fn;
} math_node_state;

void *math_perform(Node *node, math_node_state *state, Node *inputs[],
                   int nframes, double spf) {
  double *out = node->output.buf;
  Signal in = inputs[0]->output;

  for (int i = 0; i < in.size * in.layout; i++) {
    out[i] = state->math_fn(in.buf[i]);
  }

  return node->output.buf;
}

NodeRef math_node(MathNodeFn math_fn, NodeRef input) {
  AudioGraph *graph = _graph;

  int state_size = sizeof(math_node_state);

  math_node_state m = {.math_fn = math_fn};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)math_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = input->output.layout,
                         .size = input->output.size,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * input->output.size)},
      .meta = "node_math",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  math_node_state *state = mem;
  *state = m;

  node->connections[0].source_node_index = input->node_index;
  //
  return node;
}
