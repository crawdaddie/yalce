#include "node.h"
typedef struct out_data {
} out_data;

void perform_out(Node *node, int frame_count, double seconds_per_frame,
                 double seconds_offset) {
  double *out = node->out;
  double *in = node->in;
  for (int i = 0; i < frame_count; i++) {
    /* out[i] = out[i] + in[i]; */
    out[i] = in[i];
  };
}
Node *node_out(double *in, double *bus) {
  out_data *data = malloc(sizeof(out_data));
  Node *out = alloc_node((NodeData *)data, in, bus, (t_perform)perform_out,
                         "out", NULL);
  return out;
}
