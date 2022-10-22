#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include <stdlib.h>

typedef struct square_data {
  sample_t ramp;
  sample_t freq;
} square_data;
square_data *alloc_square_data() {
  square_data *data = malloc(sizeof(square_data));
  data->ramp = 0.0;
  data->freq = 220.0;
  return data;
}

sample_t scale_val_2(sample_t env_val, // 0-1
                   sample_t min, sample_t max) {
  return min + env_val * (max - min);
}

sample_t sq_sample(sample_t ramp, sample_t freq) {
  return scale_val_2((fmod(ramp * freq * 2.0 * PI, 2 * PI) > PI), -1, 1);
}
t_perform perform_square(Graph *graph, nframes_t nframes) {
  square_data *data = (square_data *)graph->data;
  for (int i = 0; i < nframes; i++) {
    if (i < graph->schedule) {
      break;
    };
    graph->schedule = -1;
    sample_t freq = data->freq;

    sample_t sample = sq_sample(data->ramp, freq);
    sample += sq_sample(data->ramp, freq * 1.01);
    graph->out[i] = sample * 0.0125;
    data->ramp += (sample_t)1.0 / 48000;
  }
}


void add_square_node_msg_handler(Graph *graph, int time, void **args) {
  square_data *data = alloc_square_data();
  sample_t *outbus = args[0];
  Graph *square_node = alloc_graph((NodeData *)data, outbus, (t_perform)perform_square, 1);
  add_after(graph, square_node);
}

void add_square_node_msg(UserCtx *ctx, nframes_t frame_time) {
  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "square node";
  msg->time = frame_time;
  msg->func = (MsgAction)add_square_node_msg_handler;
  msg->ref = NULL;


  msg->num_args = 1;
  msg->args = malloc(msg->num_args * sizeof(void *));
  msg->args[0] = ctx->buses[1];
  enqueue(ctx->msg_queue, msg);
}
void set_freq_msg_handler(Graph *graph, int time, void **args) {
  printf("set freq %f\n", args[0]);
}
void set_freq_msg(UserCtx *ctx, Graph *node, nframes_t frame_time, sample_t freq) {
  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "square node set freq";
  msg->time = frame_time;
  msg->func = (MsgAction)set_freq_msg_handler;
  msg->ref = NULL;


  msg->num_args = 1;
  msg->args = malloc(msg->num_args * sizeof(void *));
  sample_t *freq_ = &freq;
  msg->args[0] = freq_;
  enqueue(ctx->msg_queue, msg);
}
