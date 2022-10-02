#include "node.h"
#include <soundio/soundio.h>
/* Node *alloc_delay_node(NodeData *data, t_perform perform, char *name, ) { */
/*   Node *node = malloc(sizeof(Node) + sizeof(data)); */
/*   node->name = name ? (name) : ""; */
/*   node->perform = perform; */
/*   node->next = NULL; */
/*   node->data = data; */
/*   return node; */
/* } */
typedef struct delay_data {
  int delay_time_ms;
  int bufsize;
  int read_ptr;
  int write_ptr;
  double *buffer;
} delay_data;

void perform_delay(Node *node, double *out, int frame_count,
                   double seconds_per_frame, double seconds_offset) {
  delay_data *data = (delay_data *)node->data;
  double input;
  double output;
  double *buffer = data->buffer;
  for (int i = 0; i < frame_count; i++) {
    input = out[i];
    /* buffer[data->write_ptr] = input; */
    /* output = data->buffer[data->read_ptr] + input; */
    data->write_ptr++;
    data->read_ptr++;
    if (data->write_ptr > data->bufsize) {
      data->write_ptr -= data->bufsize;
    };
    if (data->read_ptr > data->bufsize) {
      data->read_ptr -= data->bufsize;
    };

    out[i] = input;
  }
}

Node *get_delay_node(int delay_time_ms, int max_delay_time_ms,
                     struct SoundIoOutStream *outstream) {

  int bufsize = (int)(outstream->sample_rate * max_delay_time_ms / 1000);
  double *buffer = malloc(sizeof(double) * bufsize);

  int read_ptr = (int)outstream->sample_rate * delay_time_ms / 1000;

  delay_data *data = malloc(sizeof(delay_data));
  data->delay_time_ms = delay_time_ms;
  data->buffer = buffer;
  data->bufsize = bufsize;
  data->read_ptr = 0;
  data->write_ptr = 0;

  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = "delay";
  node->perform = perform_delay;
  node->next = NULL;
  node->data = (NodeData *)data;
  return node;
}
