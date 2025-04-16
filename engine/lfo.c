#include "./lfo.h"
#include "audio_graph.h"
#include "node.h"
#include "node_util.h"
#include <stdio.h>
#include <string.h>

typedef struct lfo_state {
  double phase;
  double prev_trig;
  int current_segment;
  double start;
  double target;
  double time;
  double curve;
  int active;
} lfo_state;

// Helper function to interpolate between two points based on curve parameter
static double interpolate_value(double t, double y1, double y2, double curve) {
  // Linear interpolation if curve is close to zero
  if (fabs(curve) < 0.001) {
    return y1 + t * (y2 - y1);
  } else {
    // Exponential curve
    double sign = curve > 0 ? 1.0 : -1.0;
    double k = exp(sign * fabs(curve) * 3.0); // Scale the curve effect
    double curve_t = (exp(sign * fabs(curve) * 3.0 * t) - 1) / (k - 1);
    return y1 + curve_t * (y2 - y1);
  }
}

void reset_lfo(lfo_state *state, int num_points, double *data) {

  state->phase = 0.0;
  state->current_segment = 0;
  state->start = data[0]; // First point value

  if (num_points > 1) {
    state->target = data[3]; // Second point value (at index 3*1-1)
    state->time = data[1];   // Time interval
    state->curve = data[2];  // Curve parameter
  } else {
    // If there's only one point, stay at that value
    state->target = state->start;
    state->time = 1.0;  // Default time
    state->curve = 0.0; // Linear
  }
}

void set_segment(lfo_state *state, int num_points, int seg, double *data) {

  state->phase = 0.0;
  state->current_segment = seg;
  state->start = data[seg * 3]; // First point value

  state->target = data[(seg + 1) * 3]; // Second point value (at index 3*1-1)
  state->time = data[(seg * 3) + 1];   // Time interval
  state->curve = data[(seg * 3) + 2];  // Curve parameter
}

void *lfo_perform(Node *node, lfo_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  double *data = inputs[1]->output.buf;
  int size = inputs[1]->output.size;
  int num_points = (size + 2) / 3;

  for (int i = 0; i < nframes; i++) {
    double trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      reset_lfo(state, num_points, data);
    }

    double norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    double value = interpolate_value(norm_phase, state->start, state->target,
                                     state->curve);
    out[i] = value;

    state->phase += spf;

    if (state->phase >= state->time) {
      state->current_segment++;

      if (state->current_segment >= num_points - 1) {
        // repeat
        reset_lfo(state, num_points, data);
      } else {
        int curr = state->current_segment;
        set_segment(state, num_points, curr, data);
      }
    }

    state->prev_trig = trig;
  }

  return node->output.buf;
}

NodeRef lfo_node(NodeRef input, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfo_state);
  lfo_state lfo = {.phase = 0., .prev_trig = 0.};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)lfo_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lfo",
  };

  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  node->connections[0].source_node_index = trig->node_index;
  node->connections[1].source_node_index = input->node_index;
  return node;
}

void *buf_env_perform(Node *node, lfo_state *state, Node *inputs[], int nframes,
                      double spf) {

  double *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  double *data = inputs[1]->output.buf;
  double *time_scale = inputs[2]->output.buf;
  int size = inputs[1]->output.size;
  int num_points = (size + 2) / 3;

  while (nframes--) {
    // for (int i = 0; i < nframes; i++) {
    double trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->active = 1;
      reset_lfo(state, num_points, data);
    }

    double norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    double value = interpolate_value(norm_phase, state->start, state->target,
                                     state->curve);
    if (state->active) {
      state->phase += (spf / (*time_scale));
      *out = value;
    } else {
      *out = 0.;
    }
    out++;

    if (state->phase >= state->time) {
      state->current_segment++;

      if (state->current_segment >= num_points - 1) {
        // nothing
      } else {
        int curr = state->current_segment;
        set_segment(state, num_points, curr, data);
      }
    }

    time_scale++;
    state->prev_trig = trig;
  }

  return node->output.buf;
}

NodeRef buf_env_node(NodeRef time_scale, NodeRef input, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfo_state);
  lfo_state lfo = {.phase = 0., .prev_trig = 0.};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)buf_env_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lfo_env",
  };

  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  node->connections[0].source_node_index = trig->node_index;
  node->connections[1].source_node_index = input->node_index;
  node->connections[2].source_node_index = time_scale->node_index;
  return node;
}

typedef struct lfpulse_state {
  double phase;
} lfpulse_state;

void *lfpulse_perform(Node *node, lfpulse_state *state, Node *inputs[],
                      int nframes, double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  double *freq_in = inputs[0]->output.buf;
  double *trig_in = inputs[1]->output.buf;
  double *pw_in = inputs[2]->output.buf;

  double freq;
  double trig;
  double pw;
  double sample;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    trig = *trig_in;
    trig_in++;

    pw = *pw_in;
    pw++;

    if (state->phase >= 1.0) {
      state->phase = 0.;
    }

    if (pw == 0.) {
      sample = (state->phase == 0.0) ? 1.0 : 0.;
    } else {
      sample = (state->phase <= pw) ? 1.0 : 0.;
    }

    *out = sample;
    out++;

    state->phase += freq * spf;
  }

  return node->output.buf;
}

NodeRef lfpulse_node(NodeRef pw, NodeRef freq, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfo_state);
  lfo_state lfo = {.phase = 0., .prev_trig = 0.};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)lfpulse_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lfpulse",
  };

  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  node->connections[0].source_node_index = freq->node_index;
  node->connections[1].source_node_index = trig->node_index;
  node->connections[2].source_node_index = pw->node_index;
  return node;
}
static double perc_env[10] = {0.000000, 0.000000,  2.400000, 1.,
                              0.185902, -2.300000, 0.011111, 2.814099,
                              1.300000, 0.000000};

void *perc_env_perform(Node *node, lfo_state *state, Node *inputs[],
                       int nframes, double spf) {

  double *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  double *time_scale = inputs[1]->output.buf;

  double *data = perc_env;
  int size = 10;
  int num_points = (size + 2) / 3;

  while (nframes--) {
    // for (int i = 0; i < nframes; i++) {
    double trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->active = 1;
      reset_lfo(state, num_points, data);
    }

    double norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    double value = interpolate_value(norm_phase, state->start, state->target,
                                     state->curve);
    if (state->active) {
      state->phase += (spf / (*time_scale));
      *out = value;
    } else {
      *out = 0.;
    }
    out++;

    if (state->phase >= state->time) {
      state->current_segment++;

      if (state->current_segment >= num_points - 1) {
        // nothing
      } else {
        int curr = state->current_segment;
        set_segment(state, num_points, curr, data);
      }
    }

    time_scale++;
    state->prev_trig = trig;
  }

  return node->output.buf;
}

NodeRef perc_env_node(NodeRef decay, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfo_state);
  lfo_state lfo = {.phase = 0., .prev_trig = 0.};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)perc_env_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "perc_env",
  };

  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  node->connections[0].source_node_index = trig->node_index;
  node->connections[1].source_node_index = decay->node_index;
  return node;
}
