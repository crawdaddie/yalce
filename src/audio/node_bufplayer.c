#include "node.h"
#include "util.c"
typedef struct bufplayer_data {
  double *buffer;
  int frames;
  int read_ptr;
  int loop;
} bufplayer_data;

void perform_bufplayer(Node *node, int frame_count, double seconds_per_frame,
                       double seconds_offset) {
  bufplayer_data *data = (bufplayer_data *)node->data;
  double *out = node->out;

  for (int i = 0; i < frame_count; i++) {
    schedule();

    int read_ptr = data->read_ptr;
    out[i] = read_ptr < data->frames ? data->buffer[read_ptr] : 0.0;
    if (data->loop > 0) {
      data->read_ptr = (read_ptr + 1) % data->loop;
    } else {
      data->read_ptr = read_ptr + 1;
    }
  }
}

Node *get_bufplayer_node(double *buf, int frames, int loop) {
  bufplayer_data *data = malloc(sizeof(bufplayer_data) + sizeof(buf));
  data->buffer = buf;
  data->frames = frames;
  data->read_ptr = 0;
  data->loop = loop;
  Node *node = alloc_node((NodeData *)data, NULL, (t_perform)perform_bufplayer,
                          "bufplayer", NULL);
  return node;
}

typedef struct bufplayer_interp_data {
  double *buffer;
  int frames;
  double read_ptr;
  double rate;
  int loop;
} bufplayer_interp_data;

void perform_bufplayer_interp(Node *node, int frame_count,
                              double seconds_per_frame, double seconds_offset) {
  bufplayer_interp_data *data = (bufplayer_interp_data *)node->data;
  double *out = node->out;

  for (int i = 0; i < frame_count; i++) {
    schedule();

    int read_ptr = data->read_ptr;

    out[i] = get_sample_interp(read_ptr, data->buffer, data->frames);

    if (data->loop > 0) {
      data->read_ptr = fmod(read_ptr + data->rate, (double)data->loop);
    } else {
      data->read_ptr = read_ptr + data->rate;
    }
  }
}

Node *get_bufplayer_interp_node(double *buf, int frames, double rate,
                                double start_pos, int loop) {
  bufplayer_interp_data *data =
      malloc(sizeof(bufplayer_interp_data) + sizeof(buf));
  data->buffer = buf;
  data->frames = frames;
  data->read_ptr = start_pos * frames;
  data->rate = rate;
  data->loop = loop;
  Node *node =
      alloc_node((NodeData *)data, NULL, (t_perform)perform_bufplayer_interp,
                 "bufplayer", NULL);
  return node;
}
