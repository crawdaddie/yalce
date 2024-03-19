#include "envelope.h"
#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
node_perform env_perform(Node *node, int nframes, double spf) {
  env_state *state = node->state;
  double *out = node->out->buf;
  double *trig = node->ins[0]->buf;

  while (nframes--) {
    if (*trig == 1.0) {
      state->value = state->levels[0];
      state->state = 0;
    }

    *out = state->value;

    double target = state->levels[state->state + 1];
    double rate =
        (state->state >= state->len) ? 0.0 : state->rates[state->state];

    state->value += rate;

    if (rate > 0 && state->value >= target) {
      state->state++;
      state->value = target;
    }

    if (rate < 0 && state->value < target) {
      state->state++;
      state->value = target;
    }

    if (state->state >= state->len) {
      // env finished
    }

    out++;
    trig++;
  }
}

Node *env_node(int len, double *levels, double *times) {

  int SR = ctx_sample_rate();
  env_state *state = malloc(sizeof(env_state));

  state->levels = levels;
  state->rates = times;

  for (int i = 0; i < len; i++) {
    state->rates[i] =
        (state->levels[i + 1] - state->levels[i]) / (SR * state->rates[i]);
  }

  state->len = len;
  state->state = 0;
  state->value = levels[0];
  state->should_kill = true;
  Node *env = node_new(state, (node_perform *)env_perform, NULL, get_sig(1));
  env->num_ins = 1;
  env->ins = malloc(sizeof(Signal *));
  env->ins[0] = get_sig(1);
  return env;
}

static inline double adsr_perform_tick(adsr_state *state) {
  _adsr_state istate = state->state;

  switch (state->state) {
  case ATTACK:
    state->value += state->attack_rate;
    if (state->value >= state->target) {
      state->value = state->target;
      state->target = state->sustain_level;
      state->state = DECAY;
    }
    break;

  case DECAY:
    if (state->value > state->sustain_level) {
      state->value -= state->decay_rate;
      if (state->value <= state->sustain_level) {
        state->value = state->sustain_level;
        state->state = SUSTAIN;
      }
    } else {
      state->value += state->decay_rate; // attack target < sustain level
      if (state->value >= state->sustain_level) {
        state->value = state->sustain_level;
        state->state = SUSTAIN;
      }
    }
    break;

  case SUSTAIN:

    if (state->counter > 0) {
      state->counter--;
      state->value = state->sustain_level;
    } else {
      state->state = RELEASE;
    }
    break;

  case RELEASE:
    state->value -= state->release_rate;
    if (state->value <= 0.0) {
      state->value = 0.0;
      state->state = IDLE;
    }
    break;
  }

  // printf("%f\n", state->value);
  return state->value;
}
static node_perform adsr_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  Signal *in_sig = node->ins[0];
  double *trig = in_sig->buf;

  adsr_state *state = node->state;
  while (nframes--) {
    if (*trig == 1.0) {
      state->state = ATTACK;
      state->counter = (int)state->sustain_time / spf;
      state->value = -state->attack_rate;
    }

    switch (state->state) {
    case ATTACK:
      state->value += state->attack_rate;
      if (state->value >= state->target) {
        state->value = state->target;
        state->state = DECAY;
      }

    case DECAY:
      if (state->value > state->sustain_level) {
        state->value -= state->decay_rate;
      } else {
        state->value = state->sustain_level;
        state->state = SUSTAIN;
      }

    case SUSTAIN:
      if (state->counter > 0) {
        state->counter--;
      } else {
        state->state = RELEASE;
      }

    case RELEASE:
      if (state->value > 0.0) {
        state->value -= state->release_rate;
      } else {
        state->value = 0.0;
        state->state = IDLE;
      }
    case IDLE:
      // state->value = 0.0;
      // node->killed = true;
    }

    *out = state->value;
    // printf("%f\n", state->value);

    trig++;
    out++;
  }
}

Node *adsr_node(double attack_time, double decay_time, double sustain_level,
                double sustain_time, double release_time) {
  adsr_state *state = malloc(sizeof(adsr_state));
  int SR = ctx_sample_rate();

  state->value = 0.0;
  state->target = 1.0;
  state->attack_rate = 1.0 / (SR * attack_time);
  state->decay_rate = (1.0 - sustain_level) / (SR * decay_time);
  state->release_rate = sustain_level / (SR * release_time);
  state->sustain_level = sustain_level;

  state->counter = (int)state->sustain_time * SR;
  state->sustain_time = sustain_time;
  state->state = ATTACK;

  Node *node = node_new(state, (node_perform *)adsr_perform, NULL, get_sig(1));
  node->num_ins = 1;
  node->ins = malloc(sizeof(Signal *));
  node->ins[0] = get_sig(1);
  printf("at: %f dt %f sl %f st %f rt %f\n", attack_time, decay_time,
         sustain_level, sustain_time, release_time);
  return node;
}
