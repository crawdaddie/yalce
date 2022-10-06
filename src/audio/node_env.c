#include "node.h"
typedef struct {
  double attack;
  double sustain;
  double release;
  double time_s;
  Node *ctx;
  void (*on_free)(Node *ctx)
} env_data;

void ramp_value_tick(double *val, double target, double speed) {}

void perform_env(Node *node, int frame_count, double seconds_per_frame,
                 double seconds_offset) {
  env_data *data = (env_data *)node->data;
  double level = 0.0;
  double *out = node->out;
  double attack = data->attack;
  double sustain = data->sustain;
  double release = data->release;

  for (int i = 0; i < frame_count; i++) {
    schedule();
    double time_ms = data->time_s * 1000;
    if (time_ms <= 0.0) {
      level = 0.0;
    } else if (time_ms <= attack) {
      level = time_ms / attack;
    } else if ((time_ms > attack) && (time_ms <= attack + sustain)) {
      level = 1.0;
    } else if ((time_ms > attack + sustain) &&
               (time_ms <= attack + sustain + release)) {
      level = (attack + sustain + release - time_ms) / release;
    } else {
      if ((time_ms > attack + sustain + release)) {
        if (data->on_free)
          data->on_free(data->ctx);
      };
      level = 0.0;
    };
    out[i] = level;
    data->time_s = data->time_s + seconds_per_frame;
  }
}

Node *get_env_node(double attack, double sustain, double release) {
  env_data *data = malloc(sizeof(env_data) + sizeof(Node));
  data->attack = attack;
  data->sustain = sustain;
  data->release = release;
  data->time_s = 0.0;
  data->ctx = NULL;
  data->on_free = NULL;
  return alloc_node((NodeData *)data, NULL, (t_perform)perform_env, "env",
                    NULL);
}

void set_on_free_handler(Node *env_node, Node *ctx,
                         void (*on_free)(Node *ctx)) {
  ((env_data *)env_node->data)->ctx = ctx;
  ((env_data *)env_node->data)->on_free = on_free;
}

void reset_env(Node *node) {
  env_data *data = (env_data *)node->data;
  data->time_s = 0.0;
}
