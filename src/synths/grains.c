#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include <stdlib.h>

typedef struct grains_data {
  int buf_frames;
  sample_t *buf_data;
  int max_grains;

  sample_t ramp;
  sample_t *voice_read_ptrs;
  sample_t *voice_rates;
  sample_t grain_freq;
  sample_t grain_dur;
  sample_t grain_pos;
  
  sample_t trigger_ramp;
  int trigger;
  int voice_counter;

} grains_data;

grains_data *alloc_grains_data(int buf_frames, sample_t *buf_data, int max_grains) {
  grains_data *data = malloc(sizeof(grains_data));
  data->buf_frames = buf_frames;
  data->buf_data = buf_data;
  data->max_grains = max_grains;
  data->voice_counter = 0;
  data->voice_read_ptrs = calloc(max_grains, sizeof(sample_t));
  data->voice_rates = calloc(max_grains, sizeof(sample_t));

  data->voice_rates[0] = 1.0;

  for (int v = 1; v < max_grains; v++) {
    data->voice_rates[v] = 0.0;
  };
  data->grain_freq = 10.0;
  data->grain_dur = 0.01;
  data->grain_pos = 0.0;
  data->ramp = 0;
  data->trigger_ramp = 0;
  data->trigger = 0;
  return data;
}

sample_t grain_sample(sample_t read_ptr, sample_t *buf, int max_frames) {

  sample_t r = read_ptr;
  if (r >= max_frames) {
    return 0.0;
  }
  if (r <= 0) {
    r = max_frames + r;
  }
  int frame = ((int)r);
  sample_t fraction = r - frame;
  sample_t result = buf[frame] * fraction;
  result += buf[frame + 1] * (1.0 - fraction);
  return result;
}

sample_t impulse(grains_data *data, int sample_rate) {
  sample_t grain_trig_period = 48000 / (data->grain_freq);
  int trigger = 0;
  if (data->trigger_ramp >= grain_trig_period) {
    data->trigger_ramp -= grain_trig_period;
    data->voice_counter = (data->voice_counter + 1) % data->max_grains;
    trigger = 1;
  } else {
    trigger = 0;
  };
  data->trigger_ramp += 1;
  data->trigger = trigger;
  return trigger;
}


t_perform perform_grains(Graph *graph, nframes_t nframes) {
  grains_data *data = (grains_data *)graph->data;
  sample_t output_sample = 0.0;

  for (int i = 0; i < nframes; i++) {
    impulse(data, 48000);
    if (data->trigger) {
      data->voice_read_ptrs[data->voice_counter] = data->grain_pos * data->buf_frames;
      /* data->voice_rates[data->voice_counter] = 1 + rand() % 3;  */
    };
    sample_t sample = 0.0;



    for (int v = 0; v < data->max_grains; v++) {
      data->voice_read_ptrs[v] = data->voice_read_ptrs[v] + data->voice_rates[v];
      sample += grain_sample(data->voice_read_ptrs[v], data->buf_data, data->buf_frames);
    };


    data->ramp += (sample_t)1.0 / 48000;
    /* graph->out[i] = (sample_t) data->trigger; */
    data->grain_pos = fmod(data->grain_pos + ((sample_t) 0.1 / 48000), 1.0);
    graph->out[i] = sample;
  }
}

Graph *init_grains_node(struct buf_info *buf, sample_t *out) {
  grains_data *data = alloc_grains_data(buf->frames, (sample_t *)buf->data, 10);
  Graph *node = alloc_graph((NodeData *)data, out, (t_perform)perform_grains, 1); 
  return node;
}
void add_grains_node_msg_handler(Graph *graph, int time, void **args) {
  struct buf_info *buf = args[1];
  Graph *node = init_grains_node(buf, args[0]);
  node->schedule = time;
  add_after(graph, node);
}
void add_grains_node_msg(UserCtx *ctx, nframes_t frame_time, int bufnum) {
  queue_msg_t *msg = msg_init("grains", frame_time, add_grains_node_msg_handler, 2);
  msg->args[0] = ctx->buses[0];
  msg->args[1] = ctx->buffers[bufnum];
  enqueue(ctx->msg_queue, msg);
}

