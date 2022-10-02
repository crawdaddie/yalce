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
  double feedback;
  int bufsize;
  int read_ptr;
  int write_ptr;
  double *buffer;
} delay_data;

void debug_delay_data(delay_data *data) {

  printf("bufsize: %d\n", data->bufsize);
  printf("write_ptr: %d\n", data->write_ptr);
  printf("read_ptr: %d\n", data->read_ptr);
  printf("buffer &: %#08x\n", data->buffer);
  printf("-------\n");
}
void perform_delay(Node *node, double *out, int frame_count,
                   double seconds_per_frame, double seconds_offset) {
  delay_data *data = (delay_data *)node->data;
  double input;
  double output;
  double *buffer = data->buffer;
  for (int i = 0; i < frame_count; i++) {
    input = out[i];
    output = data->buffer[data->read_ptr] * 0.5 + input;

    data->write_ptr++;
    data->read_ptr++;

    if (data->write_ptr > data->bufsize) {
      data->write_ptr -= data->bufsize;
    };
    if (data->read_ptr > data->bufsize) {
      data->read_ptr -= data->bufsize;
    };

    buffer[data->write_ptr] = output;
    out[i] = output;
  }
}

Node *get_delay_node(int delay_time_ms, int max_delay_time_ms, double feedback,
                     struct SoundIoOutStream *outstream) {

  int bufsize = (int)(48000 * max_delay_time_ms / 1000);

  int read_ptr = (int)(48000 * delay_time_ms / 1000);

  double *buffer = malloc(sizeof(double) * bufsize);
  printf("buffer &: %#08x\n", buffer);
  delay_data *data = malloc(sizeof(delay_data) + sizeof(double) * bufsize);
  data->delay_time_ms = delay_time_ms;
  data->feedback = feedback;
  data->bufsize = bufsize;
  data->buffer = buffer;
  data->read_ptr = read_ptr;
  data->write_ptr = 0;
  debug_delay_data(data);

  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = "delay";
  node->perform = perform_delay;
  node->next = NULL;
  node->data = (NodeData *)data;
  return node;
}
