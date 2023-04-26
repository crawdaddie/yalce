#include "delay.h"
#include "../common.h"
#include "../log.h"
#include "math.h"
#include <stdlib.h>

static double sanitize_delay_pointer(double ptr, int bufsize) {
  if (ptr >= bufsize) {
    return ptr - bufsize;
  };
  if (ptr < 0) {
    return ptr + bufsize;
  };
  return ptr;
}

static node_perform delay_perform(Node *node, int nframes, double spf) {
  Signal *in = INS(node);

  delay_data *data = (delay_data *)node->data;

  double input;
  double output;
  double *buffer = data->buffer;

  for (int i = 0; i < nframes; i++) {

    data->write_ptr = sanitize_delay_pointer(data->write_ptr, data->bufsize);
    data->read_ptr = sanitize_delay_pointer(data->read_ptr, data->bufsize);

    input = in->data[i];
    output = get_sample_interp(data->read_ptr, data->buffer, data->bufsize) *
                 data->delay_fb +
             input;

    data->write_ptr++;
    data->read_ptr++;

    buffer[data->write_ptr] = output;
    node_write_out(node, i, output);
  }
}

Node *simple_delay_node(double delay_time_s, double delay_fb,
                        double max_delay_time_s, Signal *ins) {

  Node *delay = ALLOC_NODE(delay_data, "Delay", 1);
  delay->perform = (node_perform)delay_perform;
  delay_data *data = delay->data;

  int bufsize = (int)(48000 * max_delay_time_s);
  double read_ptr = (double)(48000 * delay_time_s) * -1;

  data->bufsize = bufsize;
  data->read_ptr = read_ptr;
  data->write_ptr = 0;
  data->delay_fb = delay_fb;

  data->buffer = malloc((sizeof(double)) * bufsize);

  INS(delay) = ins == NULL ? ALLOC_SIGS(DELAY_SIG_OUT) : ins;
  init_signal(INS(delay), BUF_SIZE, 0.0);
  NUM_INS(delay) = 1;

  init_out_signal(&delay->out, BUF_SIZE, 1);
  write_log("build delay node %p\n", delay);

  return delay;
}
