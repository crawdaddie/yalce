#include "node.h"
#include <soundio/soundio.h>
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
double sanitize_delay_pointer(double ptr, int bufsize) {
  if (ptr >= bufsize) {
    return ptr - bufsize;
  };
  if (ptr < 0) {
    return ptr + bufsize;
  };
  return ptr;
}
void perform_delay(Node *node, int frame_count, double seconds_per_frame,
                   double seconds_offset) {
  double *out = node->out;
  double *in = node->in;
  delay_data *data = (delay_data *)node->data;
  double input;
  double output;
  double *buffer = data->buffer;
  for (int i = 0; i < frame_count; i++) {
    data->write_ptr = sanitize_delay_pointer(data->write_ptr, data->bufsize);
    data->read_ptr = sanitize_delay_pointer(data->read_ptr, data->bufsize);

    input = in[i];
    output = data->buffer[data->read_ptr] * 0.5 + input;

    data->write_ptr++;
    data->read_ptr++;

    buffer[data->write_ptr] = output;
    out[i] = output;
    /* node_mul_add(node, i); */
  }
}
void free_delay_node(Node *node) {
  delay_data *data = (delay_data *)node->data;
  free(data->buffer);
  free_node(node);
}

Node *get_delay_node(double *in, int delay_time_ms, int max_delay_time_ms,
                     double feedback, int sample_rate) {

  int bufsize = (int)(sample_rate * max_delay_time_ms / 1000);
  int read_ptr = (int)(sample_rate * delay_time_ms / 1000) * -1;

  double *buffer = malloc(sizeof(double) * bufsize);
  delay_data *data = malloc(sizeof(delay_data) + sizeof(double) * bufsize);
  data->delay_time_ms = delay_time_ms;
  data->feedback = feedback;
  data->bufsize = bufsize;
  data->buffer = buffer;
  data->read_ptr = read_ptr;
  data->write_ptr = 0;

  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = "delay";
  node->perform = perform_delay;
  node->next = NULL;
  node->data = (NodeData *)data;
  node->free_node = free_delay_node;
  node->in = in;
  node->out = get_buffer();
  node->should_free = 0;
  return node;
}
