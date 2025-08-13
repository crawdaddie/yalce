#include "./lfo.h"
#include "audio_graph.h"
#include "node.h"
#include "node_util.h"
#include <stdio.h>
#include <string.h>

typedef struct lfo_state {
  sample_t phase;
  sample_t prev_trig;
  int current_segment;
  sample_t start;
  sample_t target;
  sample_t time;
  sample_t curve;
  int active;
  int sustaining;
} lfo_state;

// Helper function to interpolate between two points based on curve parameter
static sample_t interpolate_value(sample_t t, sample_t y1, sample_t y2,
                                  sample_t curve) {
  // Linear interpolation if curve is close to zero
  if (fabs(curve) < 0.001) {
    return y1 + t * (y2 - y1);
  } else {
    // Exponential curve
    sample_t sign = curve > 0 ? 1.0 : -1.0;
    sample_t k = exp(sign * fabs(curve) * 3.0); // Scale the curve effect
    sample_t curve_t = (exp(sign * fabs(curve) * 3.0 * t) - 1) / (k - 1);
    return y1 + curve_t * (y2 - y1);
  }
}

void reset_lfo(lfo_state *state, int num_points, sample_t *data) {

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

void set_segment(lfo_state *state, int num_points, int seg, sample_t *data) {

  state->phase = 0.0;
  state->current_segment = seg;
  state->start = data[seg * 3]; // First point value

  state->target = data[(seg + 1) * 3]; // Second point value (at index 3*1-1)
  state->time = data[(seg * 3) + 1];   // Time interval
  state->curve = data[(seg * 3) + 2];  // Curve parameter
}

void *lfo_perform(Node *node, lfo_state *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  sample_t *data = inputs[1]->output.buf;
  int size = inputs[1]->output.size;
  int num_points = (size + 2) / 3;

  for (int i = 0; i < nframes; i++) {
    sample_t trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      reset_lfo(state, num_points, data);
    }

    sample_t norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    sample_t value = interpolate_value(norm_phase, state->start, state->target,
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

  char *mem = state_ptr(graph, node);
  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  // node->connections[0].source_node_index = trig->node_index;
  plug_input_in_graph(0, node, trig);
  // node->connections[1].source_node_index = input->node_index;
  plug_input_in_graph(1, node, input);
  return graph_embed(node);
}

void *buf_env_perform(Node *node, lfo_state *state, Node *inputs[], int nframes,
                      sample_t spf) {

  sample_t *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  sample_t *data = inputs[1]->output.buf;
  sample_t *time_scale = inputs[2]->output.buf;
  int size = inputs[1]->output.size;
  int num_points = (size + 2) / 3;

  while (nframes--) {
    // for (int i = 0; i < nframes; i++) {
    sample_t trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->active = 1;
      reset_lfo(state, num_points, data);
    }

    sample_t norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    sample_t value = interpolate_value(norm_phase, state->start, state->target,
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

  // char *mem = (graph != NULL)
  //                 ? (char *)(graph->nodes_state_memory + node->state_offset)
  //                 : (char *)((Node *)node + 1);

  lfo_state *state = state_ptr(graph, node);
  *state = lfo;

  // node->connections[0].source_node_index = trig->node_index;
  plug_input_in_graph(0, node, trig);

  // node->connections[1].source_node_index = input->node_index;
  plug_input_in_graph(1, node, input);
  // node->connections[2].source_node_index = time_scale->node_index;
  plug_input_in_graph(2, node, time_scale);
  return graph_embed(node);
}

typedef struct lfpulse_state {
  sample_t phase;
  sample_t prev_trig;
} lfpulse_state;

void *__lfpulse_perform(Node *node, lfpulse_state *state, Node *inputs[],
                        int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;
  sample_t *freq_in = inputs[0]->output.buf;
  sample_t *trig_in = inputs[1]->output.buf;
  sample_t *pw_in = inputs[2]->output.buf;

  sample_t freq;
  sample_t trig;
  sample_t pw;
  sample_t sample;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;

    trig = *trig_in;
    trig_in++;

    pw = *pw_in;
    pw_in++;

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->phase = 0.;

      if (pw == 0. && freq == 0.) {
        sample = 1.;
      }
      printf("trig smaple %f\n", sample);
    }

    if (state->phase >= 1.0) {
      state->phase = 0.;
    }

    if (pw != 0.) {
      sample = (state->phase <= pw) ? 1.0 : 0.;
    }

    *out = sample;
    out++;
    sample_t incr = (freq * spf);
    state->phase += incr;
    state->prev_trig = trig;
  }

  return node->output.buf;
}
void *___lfpulse_perform(Node *node, lfpulse_state *state, Node *inputs[],
                         int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;
  sample_t *freq_in = inputs[0]->output.buf;
  sample_t *trig_in = inputs[1]->output.buf;
  sample_t *pw_in = inputs[2]->output.buf;

  sample_t freq;
  sample_t trig;
  sample_t pw;
  sample_t sample;

  while (nframes--) {
    freq = *freq_in;
    freq_in++;
    pw = *pw_in;
    pw_in++;
    trig = *trig_in;
    trig_in++;

    if (freq == 0.0) {
      *out = trig;
      out++;
    } else if (pw == 0.0) {

      sample = 0.;
      if (trig >= 0.5 && state->prev_trig < 0.5) {
        state->phase = 0.;
        sample = 1.;
      }

      if (state->phase >= 1.0) {
        state->phase = 0.;
        sample = 1.;
      }

      sample_t incr = (freq * spf);
      state->phase += incr;
      state->prev_trig = trig;

      *out = sample;
      out++;

    } else {

      trig = *trig_in;
      trig_in++;

      sample = 0.0;

      if (trig >= 0.5 && state->prev_trig < 0.5) {
        state->phase = 0.;

        if (pw == 0. && freq == 0.) {
          sample = 1.;
        }
      }

      if (state->phase >= 1.0) {
        state->phase = 0.;
      }

      // Only set sample based on pw if it's non-zero AND we haven't already set
      // it to 1
      sample = (state->phase <= pw) ? 1.0 : 0.;

      *out = sample;
      out++;
      sample_t incr = (freq * spf);
      state->phase += incr;
      state->prev_trig = trig;
    }
  }

  return node->output.buf;
}
void *fpulse_perform(Node *node, lfpulse_state *state, Node *inputs[],
                     int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;
  sample_t *freq_in = inputs[0]->output.buf;
  sample_t *trig_in = inputs[1]->output.buf;
  sample_t *pw_in = inputs[2]->output.buf;

  sample_t freq;
  sample_t trig;
  sample_t pw;
  sample_t sample;

  for (int i = 0; i < nframes; i++) {
    freq = *freq_in++;
    trig = *trig_in++;
    pw = *pw_in++;

    // Initialize sample to 0 by default
    sample = 0.0;

    // Handle trigger (sync) - always reset phase on trigger regardless of other
    // settings
    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->phase = 0.0;
      sample = 1.0;
    }

    // Special case: if frequency is zero, just pass through the trigger
    if (freq == 0.0) {
      sample = trig;
    }
    // Normal oscillator operation
    else {
      // Calculate new phase value for this sample
      sample_t incr = (freq * spf);
      printf("incr %f\n", incr);

      // Check if we've completed a cycle
      if (state->phase >= 1.0) {
        // Reset phase to fractional part
        state->phase = state->phase - 1.0;

        // For zero pulsewidth, emit a pulse exactly when phase wraps
        if (pw == 0.0 &&
            sample == 0.0) { // Don't override a trigger-induced pulse
          sample = 1.0;
        }
      }

      // For non-zero pulsewidth, standard variable-width pulse behavior
      if (pw > 0.0) {
        // Generate pulse based on pulsewidth
        sample = (state->phase < pw) ? 1.0 : 0.0;
      }

      // Advance phase
      state->phase += incr;
    }

    // Store output and update previous trigger
    *out++ = sample;
    state->prev_trig = trig;
  }

  return node->output.buf;
}
void *lfpulse_perform(Node *node, lfpulse_state *state, Node *inputs[],
                      int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;
  sample_t *freq_in = inputs[0]->output.buf;
  sample_t *trig_in = inputs[1]->output.buf;
  sample_t *pw_in = inputs[2]->output.buf;

  for (int i = 0; i < nframes; i++) {
    sample_t freq = *freq_in++;
    sample_t trig = *trig_in++;
    sample_t pw = *pw_in++;

    // Initialize sample to 0 by default
    sample_t sample = 0.0;

    // Track if phase wrapped this sample
    bool phase_wrapped = false;

    // Check if we've completed a cycle BEFORE incrementing the phase
    if (state->phase >= 1.0) {
      state->phase = fmodf(state->phase, 1.0);
      phase_wrapped = true;
    }

    // Handle trigger (sync)
    bool triggered = false;
    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->phase = 0.0;
      triggered = true;
    }

    // Special case: if frequency is zero, just pass through the trigger
    if (freq == 0.0) {
      sample = trig;
    }
    // Normal oscillator operation
    else {
      // For zero pulsewidth, emit a pulse ONLY on phase wrap or trigger
      if (pw <= 0.00001) {
        if (phase_wrapped || triggered) {
          sample = 1.0;
        } else {
          sample = 0.0;
        }
      } else {
        sample = (state->phase < pw) ? 1.0 : 0.0;
      }

      sample_t incr = freq * spf;
      state->phase += incr;
    }

    *out++ = sample;
    state->prev_trig = trig;
  }

  return node->output.buf;
}

NodeRef lfpulse_node(NodeRef pw, NodeRef freq, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfpulse_state);
  lfpulse_state lfo = {.phase = 0., .prev_trig = 0.};

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

  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  lfpulse_state *state = mem;
  *state = lfo;

  // node->connections[0].source_node_index = freq->node_index;
  plug_input_in_graph(0, node, freq);
  // node->connections[1].source_node_index = trig->node_index;
  plug_input_in_graph(1, node, trig);
  // node->connections[2].source_node_index = pw->node_index;
  plug_input_in_graph(2, node, pw);
  return graph_embed(node);
}
// clang-format off
static sample_t perc_env[10] = {0.000000, 0.000000,  2.400000,
                              1.,       0.185902, -2.300000,
                              0.011111, 0.1, 1.300000,
                              0.000000
};
// clang-format on
void *perc_env_perform(Node *node, lfo_state *state, Node *inputs[],
                       int nframes, sample_t spf) {

  sample_t *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  sample_t *time_scale = inputs[1]->output.buf;

  sample_t *data = perc_env;
  int size = 10;
  int num_points = (size + 2) / 3;

  while (nframes--) {
    // for (int i = 0; i < nframes; i++) {
    sample_t trig = *READ(_trig);

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->active = 1;
      reset_lfo(state, num_points, data);
    }

    sample_t norm_phase;
    if (state->time > 0.0) {
      norm_phase = state->phase / state->time; // Normalized phase [0,1]
      norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
    } else {
      norm_phase = 1.0; // At end of segment
    }

    sample_t value = interpolate_value(norm_phase, state->start, state->target,
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

  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  plug_input_in_graph(0, node, trig);
  plug_input_in_graph(1, node, decay);
  return graph_embed(node);
}

void *gated_buf_env_perform(Node *node, lfo_state *state, Node *inputs[],
                            int nframes, sample_t spf) {

  sample_t *out = node->output.buf;
  sample_t *_trig = inputs[0]->output.buf;
  sample_t *data = inputs[1]->output.buf;
  int size = inputs[1]->output.size;
  int num_points = (size + 2) / 3;

  while (nframes--) {
    sample_t trig = *_trig;
    _trig++;

    if (trig >= 0.5 && state->prev_trig < 0.5) {
      // Attack - gate opened
      state->active = 1;
      state->sustaining = 0;
      reset_lfo(state, num_points, data);
      state->prev_trig = trig;
    }

    if (state->prev_trig >= 0.5 && trig < 0.5) {
      // Release - gate closed

      if (state->sustaining) {
        // If we were in sustain, move to release phase (second-to-last to last
        // point)
        state->current_segment = num_points - 2;
        state->phase = 0.0;

        // Set up the segment from second-to-last to last point
        int curr = state->current_segment;
        set_segment(state, num_points, curr, data);

        // No longer sustaining
        state->sustaining = 0;
      }
    }

    sample_t value;

    if (state->sustaining) {
      // If in sustain mode, hold at the second-to-last point's value
      value = state->start;
    } else {
      // Normal envelope traversal
      sample_t norm_phase;
      if (state->time > 0.0) {
        norm_phase = state->phase / state->time; // Normalized phase [0,1]
        norm_phase = fmin(1.0, norm_phase);      // Clamp to [0,1]
      } else {
        norm_phase = 1.0; // At end of segment
      }

      value = interpolate_value(norm_phase, state->start, state->target,
                                state->curve);

      // Only advance phase if not sustaining
      if (state->active) {
        state->phase += spf;
      }
    }

    // Output the value
    if (state->active) {
      *out = value;
    } else {
      *out = 0.;
    }
    out++;

    // Check if we need to advance to the next segment
    if (!state->sustaining && state->phase >= state->time) {
      state->current_segment++;

      if (state->current_segment == num_points - 2) {
        // Reached second-to-last point - enter sustain mode if gate is still
        // open
        if (trig >= 0.5) {
          state->sustaining = 1;
          // printf("sustaining at value: %f\n", state->target);
        } else {
          // Gate already closed, continue to release phase
          int curr = state->current_segment;
          set_segment(state, num_points, curr, data);
        }
      } else if (state->current_segment >= num_points - 1) {
        // Reached the end of the envelope
        state->active = 0; // Envelope completed
        // printf("envelope completed\n");
      } else {
        // Normal segment transition
        int curr = state->current_segment;
        set_segment(state, num_points, curr, data);
      }
    }

    state->prev_trig = trig;
  }

  return node->output.buf;
}

NodeRef gated_buf_env_node(NodeRef input, NodeRef trig) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(lfo_state);
  lfo_state lfo = {.phase = 0., .prev_trig = 0.};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)gated_buf_env_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "gated_buf_env",
  };

  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  lfo_state *state = mem;
  *state = lfo;

  plug_input_in_graph(0, node, trig);
  plug_input_in_graph(1, node, input);
  return graph_embed(node);
}
