#ifndef _DELAY_H
#define _DELAY_H
#include "../common.h"
#include "../graph/graph.h"

typedef struct delay_data {
  int bufsize;
  double read_ptr;
  int write_ptr;
  double *buffer;
} delay_data;

static double sanitize_delay_pointer(double ptr, int bufsize) {
  if (ptr >= bufsize) {
    return ptr - bufsize;
  };
  if (ptr < 0) {
    return ptr + bufsize;
  };
  return ptr;
}

static double get_sample_interp(double read_ptr, double *buf, int step,
                                int max_frames) {
  double r = read_ptr;
  if (r >= max_frames) {
    return 0.0;
  }
  if (r <= 0) {
    r = max_frames + r;
  }
  int frame = ((int)r);
  double fraction = r - frame;
  double result = buf[frame] * fraction;
  result += buf[frame + step] * (1.0 - fraction);
  return result;
}

static double loop_delay_pointer(double ptr, int bufsize) {
  if (ptr >= bufsize) {
    return ptr - bufsize;
  };

  if (ptr < 0) {
    return ptr + bufsize;
  };
  return ptr;
}

static void loop_delay_pointers(delay_data *data) {
  data->write_ptr = loop_delay_pointer(data->write_ptr, data->bufsize);
  data->read_ptr = loop_delay_pointer(data->read_ptr, data->bufsize);
}
static void increment_delay_pointers(delay_data *data) {
  data->write_ptr++;
  data->read_ptr++;
}

static void perform_delay(Graph *node, int frame_count,
                          double seconds_per_frame) {
  delay_data *data = node->data;
  Signal input = node->in[0];
  Signal delay_time_ms = node->in[1];
  Signal fb = node->in[2];
  double *buffer = data->buffer;
  double *out = node->out;

  for (int frame = 0; frame < frame_count; frame++) {
    loop_delay_pointers(data);
    double output_0 =
        get_sample_interp(2 * data->read_ptr, buffer, 2, data->bufsize) *
            unwrap(fb, frame) +
        unwrap(input, 2 * frame);

    double output_1 =
        get_sample_interp(2 * data->read_ptr + 1, buffer, 2, data->bufsize) *
            unwrap(fb, frame) +
        unwrap(input, 2 * frame + 1);

    buffer[2 * data->write_ptr] = output_0;
    buffer[2 * data->write_ptr + 1] = output_1;

    out[2 * frame] = output_0;
    out[2 * frame + 1] = output_0;

    increment_delay_pointers(data);
  }
}

Graph *delay_create(double *input) {

  int max_delay_time_ms = 1000;
  int delay_time_ms = 750;

  int sample_rate = 48000;
  int bufsize = (int)(sample_rate * max_delay_time_ms / 1000);
  int read_ptr = (double)(sample_rate * delay_time_ms / 1000) * -1;

  Graph *node = calloc(sizeof(Graph), 1);
  node->perform = perform_delay;

  double *buffer = malloc(sizeof(double) * bufsize * 2);
  delay_data *data = malloc(sizeof(delay_data) + sizeof(double) * bufsize);
  data->bufsize = bufsize;
  data->buffer = buffer;
  data->write_ptr = 0;
  data->read_ptr = read_ptr;
  node->data = data;

  node->out = calloc(sizeof(double), 2 * BUF_SIZE);
  node->num_outs = 2;

  node->num_ins = 3;
  node->in = malloc(sizeof(Signal) * 3);
  node->in[0].data = input;
  node->in[0].size = BUF_SIZE * 2;

  node->in[1].data = calloc(sizeof(double), 1);
  *node->in[1].data = delay_time_ms;
  node->in[1].size = 1;

  node->in[2].data = calloc(sizeof(double), 1);
  *node->in[2].data = 0.5;
  node->in[2].size = 1;

  return node;
}

#endif
