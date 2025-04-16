#include "./lfo.h"
#include "audio_graph.h"
#include "node.h"
#include "node_util.h"
#include <string.h>

typedef struct lfo_state {
  double phase;
  double prev_trig;
} lfo_state;

void *lfo_perform(Node *node, lfo_state *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  Signal _trig = inputs[0]->output;
  while (nframes--) {
    double trig = *READ(_trig);
    if (trig >= 0.5 && state->prev_trig < 0.5) {
      state->phase = 0.;
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

  /* Initialize state memory */
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
