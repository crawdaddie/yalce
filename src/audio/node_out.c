#include "node.h"
typedef struct out_data {
  double *out_bus;
} out_data;

void perform_out(Node *node, int frame_count, double seconds_per_frame,
                 double seconds_offset) {
  out_data *data = (out_data *)node->data;

  double *in = node->in;
  for (int i = 0; i < frame_count; i++) {
    data->out_bus[i] = data->out_bus[i] + in[i];
    /* printf("out bus %f %f\n", data->out_bus[i], in[i]); */
  };
}
Node *node_out(double *in, double *bus) {
  out_data *data = malloc(sizeof(out_data) + sizeof(bus));
  data->out_bus = bus;
  Node *out =
      alloc_node((NodeData *)data, in, (t_perform)perform_out, "out", NULL);
  return out;
}
