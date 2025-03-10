#include "./audio_loop.h"
#include "./ctx.h"
#include "./node.h"
#include "./osc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Graph container
typedef struct {
  Node *nodes;          // Array of all nodes
  int node_count;       // Number of nodes in the graph
  int capacity;         // Maximum nodes in the graph
  double *buffer_pool;  // Pool of audio buffer memory
  int buffer_pool_size; // Size of buffer pool in doubles
  int buffer_pool_capacity;
  char *nodes_state_memory; // Pool of node state memory
  int state_memory_size;    // Size of state memory pool
  int state_memory_capacity;
  int inlets[MAX_INPUTS]; // index of nodes which are 'inlet' nodes
  int num_inlets;
} AudioGraph;

double *allocate_buffer_from_pool(AudioGraph *graph, int size) {
  // Ensure we have enough space
  if (graph->buffer_pool_size + size > graph->buffer_pool_capacity) {
    graph->buffer_pool_capacity *= 2;
    graph->buffer_pool =
        realloc(graph->buffer_pool, graph->buffer_pool_capacity);

    // Realloc buffer pool if needed
    // ...
  }

  double *buffer = &graph->buffer_pool[graph->buffer_pool_size];
  graph->buffer_pool_size += size;
  return buffer;
}

int allocate_state_memory(AudioGraph *graph, int size) {
  size = (size + 7) & ~7; // 8-byte alignment

  int offset = graph->state_memory_size;
  if (graph->state_memory_size + size > graph->state_memory_capacity) {
    graph->state_memory_capacity *= 2;
    graph->nodes_state_memory =
        realloc(graph->nodes_state_memory, graph->state_memory_capacity);
  }

  graph->state_memory_size += size;

  return offset;
}
Node *allocate_node_in_graph(AudioGraph *graph) {
  int idx = graph->node_count++;
  // printf("idx %d graph->node_count %d\n", idx, graph->node_count);

  if (graph->node_count >= graph->capacity) {
    graph->capacity *= 2;
    graph->nodes = realloc(graph->nodes, graph->capacity * sizeof(Node));
  }

  Node *node = &graph->nodes[idx];
  node->node_index = idx;
  return node;
}
static AudioGraph *_graph = NULL;

typedef struct sin_state {
  double phase;
} sin_state;

void *sin_perform(Node *node, sin_state *state, Node *inputs[], int nframes,
                  double spf) {

  double *out = node->output.data;
  int out_layout = node->output.layout;

  // Get input buffer (frequency control) if connected
  double *in = inputs[0]->output.data;
  double d_index;
  int index = 0;
  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *in;
    in++;

    d_index = state->phase * (1 << 11);
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % (1 << 11)];
    b = sin_table[(index + 1) % (1 << 11)];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.data;
}

Node *sin_node(Node *input) {
  AudioGraph *graph = _graph;
  // Find next available slot in nodes array
  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)sin_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      // Allocate state memory
      .state_size = sizeof(sin_state),
      .state_offset = allocate_state_memory(graph, sizeof(sin_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .capacity = BUF_SIZE,
                         .data = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sin",
  };

  // Initialize state
  sin_state *state =
      (sin_state *)(graph->nodes_state_memory + node->state_offset);

  *state = (sin_state){.phase = 0.0};

  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

  return node;
}

Node *const_sig(double val) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = NULL,
      .node_index = node->node_index,
      .num_inputs = 0,
      // Allocate state memory
      .state_size = 0,
      .state_offset = graph->state_memory_size,
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .capacity = BUF_SIZE,
                         .data = allocate_buffer_from_pool(graph, BUF_SIZE)},

      .meta = "const",
  };

  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.data[i] = val;
  }
  return node;
}
// ------- Signal Multiplication Node -------

// No state needed for multiplication

void *mul_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.data;
  int out_layout = node->output.layout;

  // Get input buffers
  double *in1 = inputs[0]->output.data;
  double *in2 = inputs[1]->output.data;

  // Multiply samples
  double *out_ptr = out;
  while (nframes--) {
    double sample = (*in1++) * (*in2++);

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.data;
}

Node *mul_node(Node *input1, Node *input2) {
  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)mul_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      // No state needed
      .state_size = 0,
      .state_offset = graph->state_memory_size,
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .capacity = BUF_SIZE,
                         .data = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "mul",
  };

  // Connect inputs
  if (input1) {
    node->connections[0].source_node_index = input1->node_index;
  }
  if (input2) {
    node->connections[1].source_node_index = input2->node_index;
  }

  return node;
}

// ------- ASR Envelope Node -------

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
  double *out = node->output.data;
  int out_layout = node->output.layout;

  double *trigger = inputs[0]->output.data;

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

  return node->output.data;
}

Node *asr_node(Node *trigger, double attack_time, double sustain_level,
               double release_time) {

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)asr_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      // Allocate state memory
      .state_size = sizeof(asr_state),
      .state_offset = allocate_state_memory(graph, sizeof(asr_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .capacity = BUF_SIZE,
                         .data = allocate_buffer_from_pool(graph, BUF_SIZE)},
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

void print_graph(AudioGraph *g) {
  int node_count = g->node_count;

  for (int i = 0; i < node_count; i++) {
    Node *n = g->nodes + i;

    printf("[%d] node (%s) \n\t[", i, n->meta);

    for (int j = 0; j < n->num_inputs; j++) {
      printf("%d, ", n->connections[j].source_node_index);
    }
    printf("]\n");
  }
  printf("buffer pool: %d\n", g->buffer_pool_size);
  printf("state size: %d\n", g->state_memory_size);
}

void node_get_inputs(Node *node, AudioGraph *graph, Node *inputs[]) {
  int num_inputs = node->num_inputs;
  for (int i = 0; i < num_inputs; i++) {
    inputs[i] = graph->nodes + node->connections[i].source_node_index;
  }
}

char *node_get_state(Node *node, AudioGraph *graph) {
  if (graph == NULL) {
    // no graph context, node + state probably allocated together
    return (char *)node + sizeof(Node);
  }
  char *state = (char *)graph->nodes_state_memory + node->state_offset;
  return state;
}

void perform_audio_graph(Node *_node, AudioGraph *graph, Node *_inputs[],
                         int nframes, double spf) {
  int node_count = graph->node_count;
  Node *node = graph->nodes;
  Node *inputs[MAX_INPUTS];
  while (node_count--) {
    if (node->perform) {
      node_get_inputs(node, graph, inputs);
      char *state = node_get_state(node, graph);
      node->perform(node, state, inputs, nframes, spf);
    }

    node++;
  }
}

void write_to_dac(int dac_layout, double *dac_buf, int _layout, double *buf,
                  int output_num, int nframes) {
  int layout = 1;

  if (output_num > 0) {

    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) += *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  } else {
    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) = *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  }
}

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {

  if (!head) {
    // printf("Error: NULL head\n");
    return;
  }
  if (head->perform) {
    head->perform(head, head + 1, NULL, frame_count, spf);
    if (head->write_to_output) {
      write_to_dac(layout, dac_buf, head->output.layout, head->output.data,
                   output_num, frame_count);
    }
  }

  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}

static void write_null_to_output_buf(double *out, int nframes, int layout) {
  double *dest = out;
  while (nframes--) {
    for (int ch = 0; ch < layout; ch++) {
      *dest = 0.0;
      dest++;
    }
  }
}

void user_ctx_callback(Ctx *ctx, int frame_count, double spf) {
  // reset_buf_ptr();
  if (ctx->head == NULL) {
    write_null_to_output_buf(ctx->output_buf, frame_count, LAYOUT);
  } else {
    perform_graph(ctx->head, frame_count, spf, ctx->output_buf, LAYOUT, 0);
  }
}

Node *audio_graph_inlet(AudioGraph *g, int inlet_idx) {
  Node *inlet = g->nodes + g->inlets[inlet_idx];
  return inlet;
}

Node *inlet(double default_val) {
  Node *f = const_sig(default_val);
  _graph->inlets[_graph->num_inlets] = f->node_index;
  _graph->num_inlets++;
  return f;
}

AudioGraph *sin_ensemble() {
  AudioGraph *graph = malloc(sizeof(AudioGraph));

  *graph = (AudioGraph){
      .nodes = malloc(16 * sizeof(Node)),
      .capacity = 16,
      .buffer_pool = malloc(sizeof(double) * (1 << 12)),
      .buffer_pool_capacity = 1 << 12,
      .nodes_state_memory = malloc(sizeof(char) * (1 << 6)),
      .state_memory_capacity = 1 << 6,
  };
  _graph = graph;

  Node *f = inlet(150.);
  Node *g = inlet(1.);
  Node *s = sin_node(f);
  Node *env = asr_node(g, 0.01, 0.8, 3.0);
  Node *m = mul_node(env, s);

  graph->capacity = graph->node_count;
  graph->nodes = realloc(graph->nodes, (sizeof(Node) * graph->capacity));
  graph->buffer_pool_capacity = graph->buffer_pool_size;
  graph->buffer_pool = realloc(graph->buffer_pool,
                               (sizeof(double) * graph->buffer_pool_capacity));

  graph->state_memory_capacity = graph->state_memory_size;
  graph->nodes_state_memory =
      realloc(graph->nodes_state_memory, graph->state_memory_capacity);

  print_graph(_graph);
  return _graph;
}

Node *instantiate_template(AudioGraph *g) {
  // Allocate all required memory in one contiguous block
  char *mem =
      malloc(sizeof(Node) + sizeof(AudioGraph) + sizeof(Node) * g->capacity +
             sizeof(double) * g->buffer_pool_capacity +
             sizeof(char) * g->state_memory_capacity);

  // Set up the ensemble node at the start of memory
  Node *ensemble = (Node *)mem;
  mem += sizeof(Node);

  // Copy the AudioGraph structure next
  AudioGraph *graph_state = (AudioGraph *)mem;
  *graph_state = *g;
  mem += sizeof(AudioGraph);

  // Set up the nodes array
  graph_state->nodes = (Node *)mem;
  memcpy(graph_state->nodes, g->nodes, sizeof(Node) * g->capacity);
  mem += sizeof(Node) * g->capacity;

  // Set up the buffer pool
  graph_state->buffer_pool = (double *)mem;
  memcpy(graph_state->buffer_pool, g->buffer_pool,
         sizeof(double) * g->buffer_pool_capacity);
  mem += sizeof(double) * g->buffer_pool_capacity;

  double *buf_mem = graph_state->buffer_pool;
  for (int i = 0; i < graph_state->node_count; i++) {
    graph_state->nodes[i].output.data = buf_mem;
    buf_mem += graph_state->nodes[i].output.layout *
               graph_state->nodes[i].output.capacity;
  }

  // Set up the state memory
  graph_state->nodes_state_memory = mem;
  memcpy(graph_state->nodes_state_memory, g->nodes_state_memory,
         g->state_memory_capacity);

  // Assume the output node is the last node (the multiplier)
  Node *output_node = &graph_state->nodes[graph_state->node_count - 1];

  // Initialize the ensemble node
  *ensemble = (Node){.perform = (perform_func_t)perform_audio_graph,
                     .node_index = -1, // Special index for ensemble nodes
                     .num_inputs = 0,
                     .output = output_node->output,
                     .write_to_output = true,
                     .meta = "sin_ensemble",
                     .next = NULL};

  return ensemble;
}
// Function to add ensemble to audio context (assuming this exists)
void audio_ctx_add(Node *ensemble) {
  Ctx *ctx = get_audio_ctx();

  // Add to existing chain
  if (ctx->head == NULL) {
    ctx->head = ensemble;
  } else {
    // Find the end of the chain
    Node *current = ctx->head;
    while (current->next != NULL) {
      current = current->next;
    }
    // Append to the end
    current->next = ensemble;
  }
}
int main(int argc, char **argv) {
  AudioGraph *template = sin_ensemble();

  init_audio();

  Ctx *ctx = get_audio_ctx();

#define S 5
  double freqs[S] = {150., 300., 450, 200., 175.};
  int note_count = 0;
  int current_freq_idx = 0;

  while (1) {

    Node *ensemble = instantiate_template(template);
    AudioGraph *gr = (char *)ensemble + (sizeof(Node));
    double *freq_buf = audio_graph_inlet(gr, 0)->output.data;
    double *trig_buf = audio_graph_inlet(gr, 1)->output.data;

    double freq = freqs[current_freq_idx];
    for (int i = 0; i < BUF_SIZE; i++) {
      freq_buf[i] = freq;
    }

    for (int i = 0; i < BUF_SIZE; i++) {
      trig_buf[i] = 1.0;
    }
    audio_ctx_add(ensemble);

    useconds_t tt = 1 << 17;
    sleep(1);

    // Set trigger LOW (release phase)
    for (int i = 0; i < BUF_SIZE; i++) {
      trig_buf[i] = 0.0;
    }
    sleep(1);
  }

  return 0;
}
