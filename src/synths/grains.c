#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include "../audio/windows.c"
#include <stdlib.h>

typedef struct grain {
  t_sample  position;
  t_sample      rate;
  t_sample    offset;
  int            dur;
  int        playing;
} grain;


typedef struct grains_data {
  int     buf_frames;
  t_sample *buf_data;
  int     max_grains;
  int  voice_counter;
  grain      *grains;


  t_sample grain_freq;
  t_sample grain_dur;
  t_sample grain_pos;
  
  t_sample trigger_ramp;
  int trigger;
} grains_data;

grains_data *alloc_grains_data(int buf_frames, t_sample *buf_data, int max_grains) {
  grains_data *data = malloc(sizeof(grains_data));
  data->buf_frames = buf_frames;
  data->buf_data = buf_data;

  data->max_grains = max_grains;
  data->grain_freq = 40;
  data->grain_dur = 0.01;
  data->grain_pos = 0.0;

  data->trigger_ramp = 0;
  data->trigger = 0;

  data->voice_counter = 0;

  data->grains = malloc(sizeof(grain) * max_grains);
  for (int i = 0; i < max_grains; ++i) {
    grain g = data->grains[i];
    g.position = 0.0;
    g.offset = 0.0;
    g.rate = 1.0;
    g.playing = 0;
    g.dur = 200;
  };

  return data;
}


t_sample buf_sample(t_sample read_ptr, t_sample *buf, int max_frames) {
  t_sample r = read_ptr;

  if (r >= max_frames) {
    r = r - max_frames;
  }

  if (r <= 0) {
    r = max_frames + r;
  }

  int frame = ((int)r);
  t_sample fraction = r - frame;
  t_sample result = buf[frame] * fraction;
  result += buf[frame + 1] * (1.0 - fraction);
  return result;
}

t_sample grain_env(grain *g) {
  if (g->position >= g->dur) {
    g->playing = 0;
    return 0.0;
  };

  return hann((int)g->position, g->dur);
}

t_sample get_grain_sample(grain *grain, t_sample *buf, int max_frames) {
  t_sample read_ptr = grain->offset + grain->position * grain->rate;
  t_sample s = buf_sample(read_ptr, buf, max_frames);
  return s;
}

t_sample read_grain(grain *grain, t_sample *buf, int max_frames) {
  if (!grain->playing) {
    return 0.0;
  }

  t_sample s = grain_env(grain) * get_grain_sample(grain, buf, max_frames);
  grain->position += 1;
  return s;
}


void reset_grain(grain *grain, t_sample offset) {
  grain->playing = 1;
  grain->position = 0;
  grain->offset = offset;
}



t_sample impulse(grains_data *data, int sample_rate) {
  t_sample grain_trig_period = 48000 / (data->grain_freq);
  int trigger = 0;
  if (data->trigger_ramp >= grain_trig_period) {
    data->trigger_ramp -= grain_trig_period;
    trigger = 1;
  } else {
    trigger = 0;
  };
  data->trigger_ramp += 1;
  data->trigger = trigger;
  return trigger;
}


t_perform perform_grains(Graph *graph, t_nframes nframes) {
  grains_data *data = (grains_data *)graph->data;
  t_sample output_sample = 0.0;

  for (int i = 0; i < nframes; i++) {

    impulse(data, 48000);
    
    if (data->trigger) {
      reset_grain(
          &(data->grains[data->voice_counter]),
          data->grain_pos * data->buf_frames
        );
      data->voice_counter = (data->voice_counter + 1) % data->max_grains;
    }

    t_sample sample = 0.0;

    for (int v = 0; v < data->max_grains; ++v) {
      sample += read_grain(
          &(data->grains[v]),
          data->buf_data,
          data->buf_frames
        );
    }

    data->grain_pos = fmod(data->grain_pos + ((t_sample) 0.1 / 48000), 1.0);
    graph->out[i] = sample;
  }
}

Graph *init_grains_node(struct buf_info *buf, t_sample *out) {
  grains_data *data = alloc_grains_data(buf->frames, (t_sample *)buf->data, 10);
  Graph *node = alloc_graph((NodeData *)data, out, (t_perform)perform_grains, 1); 
  return node;
}
void add_grains_node_msg_handler(Graph *graph, int time, void **args) {
  struct buf_info *buf = args[1];
  Graph *node = init_grains_node(buf, args[0]);
  node->schedule = time;
  add_after(graph, node);
}
void add_grains_node_msg(UserCtx *ctx, t_nframes frame_time, int bufnum) {
  t_queue_msg *msg = msg_init("grains", frame_time, add_grains_node_msg_handler, 2);
  msg->args[0] = ctx->buses[0];
  msg->args[1] = ctx->buffers[bufnum];
  enqueue(ctx->msg_queue, msg);
}

