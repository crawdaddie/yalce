#include "./bus.h"
#include "audio_graph.h"
#include <string.h>

NodeRef pipe_into(NodeRef node, NodeRef filter) {
  // if (filter->)
}

void *bus_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int layout = node->output.layout;
  memset(out, 0, layout * node->output.size * sizeof(double));
}

NodeRef bus(int layout) {
  Node *node = allocate_node_in_graph(NULL, 0 /* sizeof(bus_state) */);
  *node = (Node){
      .perform = (perform_func_t)bus_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = 0,
      .state_offset = state_offset_ptr_in_graph(NULL, 0),
      .output =
          (Signal){.layout = layout,
                   .size = BUF_SIZE,
                   .buf = allocate_buffer_from_pool(NULL, BUF_SIZE * layout)},
      .meta = "bus",
  };
  return node;
}
