#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include <stdlib.h>

typedef struct grains_data {
  int buf_frames;
  sample_t *buf_data;
  int max_grains;

  sample_t ramp;
  sample_t *voice_positions;
  sample_t *voice_pitches;
  sample_t grain_freq;
  sample_t grain_dur;
  
  sample_t trigger_ramp;
  sample_t trigger;
  int voice_counter;

} grains_data;

grains_data *alloc_grains_data(int buf_frames, sample_t *buf_data, int max_grains) {
  grains_data *data = malloc(sizeof(grains_data));
  data->buf_frames = buf_frames;
  data->buf_data = buf_data;
  data->max_grains = max_grains;
  data->voice_counter = 0;
  data->voice_positions = calloc(max_grains, sizeof(sample_t));
  data->voice_pitches = calloc(max_grains, sizeof(sample_t));
  data->grain_freq = 10.0;
  data->grain_dur = 0.01;
  data->ramp = 0;
  data->trigger_ramp = 0;
  data->trigger = 0.0;
  return data;
}


sample_t grain_sample(int v, int i, sample_t *buf_data) {
}

sample_t impulse(grains_data *data, int sample_rate) {
  sample_t grain_trig_period = 48000 / (data->grain_freq);
  sample_t trigger = 0.0;
  if (data->trigger_ramp >= grain_trig_period) {
    data->trigger_ramp -= grain_trig_period;
    data->voice_counter = (data->voice_counter + 1) % data->max_grains;
    trigger = 1.0;
  } else {
    trigger = 0.0;
  };
  data->trigger_ramp += 1;
  return trigger;
}


t_perform perform_grains(Graph *graph, nframes_t nframes) {
  grains_data *data = (grains_data *)graph->data;
  sample_t output_sample = 0.0;

  for (int i = 0; i < nframes; i++) {
    sample_t trigger = impulse(data, 48000);



    for (int v = 0; v < data->max_grains; v++) {
      /* sample_t pos =  */

    };

    data->ramp += (sample_t)1.0 / 48000;
    graph->out[i] = trigger;
  }
}

Graph *init_grains_node(struct buf_info *buf, sample_t *out) {
  grains_data *data = alloc_grains_data(buf->frames, (sample_t *)buf->data, 10);
  Graph *node = alloc_graph((NodeData *)data, out, (t_perform)perform_grains, 1); 
  return node;
}
void add_grains_node_msg_handler(Graph *graph, int time, void **args) {
  /* struct buf_info *buf = args[1]; */
  /* Graph *node = init_grains_node(buf, args[0]); */
  /* node->schedule = time; */
  /* add_after(graph, node); */
}
void add_grains_node_msg(UserCtx *ctx, nframes_t frame_time, int bufnum) {
  queue_msg_t *msg = msg_init("grains", frame_time, add_grains_node_msg_handler, 2);
  msg->args[0] = ctx->buses[0];
  msg->args[1] = ctx->buffers[bufnum];
  enqueue(ctx->msg_queue, msg);
}

