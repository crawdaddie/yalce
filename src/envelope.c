#include "envelope.h"
#include "ctx.h"
#include "signal.h"
#include <stdio.h>
#include <stdlib.h>
//
//
// static node_destroy env_destroy(Node *node) {
//   env_state *state = node->state;
//   free(state->arr);
//   if (node->num_ins > 0) {
//     free(node->ins);
//   }
//   free(node->state);
//   free(node->out);
//   free(node);
// }
//
// Node *env_node(int len, // length of times array
//                double *levels, double *times) {
//
//   int SR = ctx_sample_rate();
//   env_state *state = malloc(sizeof(env_state));
//   state->arr = malloc(sizeof(double) * (2 * len + 1));
//   for (int i = 0; i < 2 * len + 1; i++) {
//     int x = (int)(i / 2);
//     state->arr[i] =
//         i % 2 == 0 ? levels[x] : (levels[x + 1] - levels[x]) / (SR *
//         times[x]);
//   }
//
//   state->len = len;
//   state->state = -1;
//   state->value = levels[0];
//   state->should_kill = false;
//
//   Node *env = node_new(state, (node_perform *)env_perform, NULL, NULL);
//   env->num_ins = 1;
//   env->ins = malloc(sizeof(Signal *));
//   env->ins[0] = get_sig_default(1, 0);
//   env->out = get_sig(1);
//   env->name = env_name;
//   env->destroy = *(node_destroy *)env_destroy;
//   return env;
// }
//
//
static char *env_name = "env";

static node_destroy env_destroy(Node *node) {
  env_state *state = node->state;
  free(state->arr);
  if (node->num_ins > 0) {
    free(node->ins);
  }
  free(node->state);
  free(node->out);
  free(node);
}

static node_perform env_perform(Node *node, int nframes, double spf) {
  env_state *state = (env_state *)node->state;
  double *trig = node->ins[0]->buf;
  double *out = node->out->buf;

  while (nframes--) {
    if (*trig == 1.0) {
      state->state = 0;
      state->value = state->arr[0];
    }

    *out = state->value;

    double target =
        state->state == -1 ? state->arr[0] : state->arr[(state->state + 1) * 2];
    double rate = state->state == -1 ? 0.0 : state->arr[(state->state) * 2 + 1];

    state->value += rate;

    if (rate > 0 && state->value >= target) {
      state->state++;
      state->value = target;
    }

    if (rate < 0 && state->value < target) {
      state->state++;
      state->value = target;
    }

    if (state->state >= state->len && state->should_kill) {
      node->parent->killed = true;
      // env finished
    }
    // printf("%f %f\n", *trig, *out);

    trig++;
    out++;
  }
}

Node *env_node(int len, // length of times array
               double *levels, double *times) {

  int SR = ctx_sample_rate();
  env_state *state = malloc(sizeof(env_state));
  state->arr = malloc(sizeof(double) * (2 * len + 1));
  for (int i = 0; i < 2 * len + 1; i++) {
    int x = (int)(i / 2);
    state->arr[i] =
        i % 2 == 0 ? levels[x] : (levels[x + 1] - levels[x]) / (SR * times[x]);
  }

  state->len = len;
  state->state = 0;
  state->value = levels[0];
  state->should_kill = false;

  Node *env = node_new(state, (node_perform *)env_perform, NULL, NULL);
  env->num_ins = 1;
  env->ins = malloc(sizeof(Signal *));
  env->ins[0] = get_sig(1);
  env->out = get_sig(1);
  env->name = env_name;
  env->destroy = *(node_destroy *)env_destroy;
  return env;
}

// one-shot autotrig env
static node_perform autotrig_env_perform(Node *node, int nframes, double spf) {
  env_state *state = (env_state *)node->state;
  double *out = node->out->buf;

  while (nframes--) {

    *out = state->value;

    double target =
        state->state == -1 ? state->arr[0] : state->arr[(state->state + 1) * 2];
    double rate = state->state == -1 ? 0.0 : state->arr[(state->state) * 2 + 1];

    state->value += rate;

    if (rate > 0 && state->value >= target) {
      state->state++;
      state->value = target;
    }

    if (rate < 0 && state->value < target) {
      state->state++;
      state->value = target;
    }

    if (state->state >= state->len && state->should_kill) {
      node->parent->killed = true;
      // env finished
    }

    out++;
  }
}

// one-shot autotrig env -- doesn't need a trig input
Node *autotrig_env_node(int len, // length of times array
                        double *levels, double *times) {

  int SR = ctx_sample_rate();
  env_state *state = malloc(sizeof(env_state));
  state->arr = malloc(sizeof(double) * (2 * len + 1));
  for (int i = 0; i < 2 * len + 1; i++) {
    int x = (int)(i / 2);
    state->arr[i] =
        i % 2 == 0 ? levels[x] : (levels[x + 1] - levels[x]) / (SR * times[x]);
  }

  state->len = len;
  state->state = 0;
  state->value = levels[0];
  state->should_kill = true;

  Node *env = node_new(state, (node_perform *)autotrig_env_perform, NULL, NULL);
  env->num_ins = 0;
  env->out = get_sig(1);
  env->name = env_name;
  env->destroy = *(node_destroy *)env_destroy;
  return env;
}
