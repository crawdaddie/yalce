#include "./envelope.h"
#include "./audio_graph.h"
#include <stdio.h>

typedef enum {
  ASR_ENV_IDLE,
  ASR_ENV_ATTACK,
  ASR_ENV_SUSTAIN,
  ASR_ENV_RELEASE
} EnvPhase;

typedef struct asr_state {
  EnvPhase phase;
  double value;         // Current envelope value
  double attack_time;   // Attack time in seconds
  double sustain_level; // Sustain level (0.0 to 1.0)
  double release_time;  // Release time in seconds
  double attack_rate;   // Precalculated rate of change during attack
  double release_rate;  // Precalculated rate of change during release
  double prev_trigger;  // Previous trigger value for edge detection
  double threshold;     // Trigger threshold (default 0.5)
} asr_state;

void *asr_perform(Node *node, asr_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  double *trigger = inputs[0]->output.buf;

  while (nframes--) {
    // Check for trigger events
    double current_trigger = *trigger;
    trigger++;

    // Rising edge - start attack phase
    if (current_trigger >= state->threshold &&
        state->prev_trigger < state->threshold) {
      state->phase = ASR_ENV_ATTACK;
    }
    // Falling edge - start release phase
    else if (current_trigger < state->threshold &&
             state->prev_trigger >= state->threshold) {
      state->phase = ASR_ENV_RELEASE;
    }

    // Update envelope based on current phase
    switch (state->phase) {
    case ASR_ENV_ATTACK:
      state->value += state->attack_rate * spf;
      if (state->value >= 1.0) {
        state->value = 1.0;
        state->phase = ASR_ENV_SUSTAIN;
      }
      break;

    case ASR_ENV_SUSTAIN:
      state->value = state->sustain_level;
      break;

    case ASR_ENV_RELEASE:
      state->value -= state->release_rate * spf;
      if (state->value <= 0.0) {
        state->value = 0.0;
        state->phase = ASR_ENV_IDLE;
        node->trig_end = true;
        // printf("finish node containing node addr: %p %p\n", node,
        //        (Node *)((char *)node - (sizeof(Node) * node->node_index) -
        // sizeof(Node) - 0xf8));
      }
      break;

    case ASR_ENV_IDLE:
      state->value = 0.0;
      break;
    }

    // Write envelope value to output
    for (int ch = 0; ch < out_layout; ch++) {
      *out = state->value;
      out++;
    }
    // printf("asr val %f trig %f phase %d\n", state->value, *trigger,
    //        state->phase);

    // Store current trigger for next iteration
    state->prev_trigger = current_trigger;
  }

  return node->output.buf;
}

Node *asr_node(double attack_time, double sustain_level, double release_time,
               Node *trigger) {

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(asr_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)asr_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      // Allocate state memory
      .state_size = sizeof(asr_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(asr_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "asr",
  };

  asr_state *state =
      (asr_state *)(graph->nodes_state_memory + node->state_offset);

  *state = (asr_state){
      .phase = ASR_ENV_ATTACK,
      .value = 0.0,
      .attack_time = attack_time,
      .sustain_level = sustain_level,
      .release_time = release_time,
      .attack_rate = (attack_time > 0.0) ? (1.0 / attack_time) : 1000.0,
      .release_rate =
          (release_time > 0.0) ? (sustain_level / release_time) : 1000.0,
      .prev_trigger = 0.0,
      .threshold = 0.5};

  // Connect trigger input
  if (trigger) {
    node->connections[0].source_node_index = trigger->node_index;
  }

  return node;
}
