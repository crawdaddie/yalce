#include "signal_arithmetic.h"
#include <stdarg.h>
#include <stdlib.h>
// ------------------------------ SIGNAL ARITHMETIC

node_perform lag_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  double *in = node->ins[0]->buf;

  lag_state *state = node->state;
  int counter = 0;

  while (nframes--) {
    if (*in != state->level) {
      counter = (int)(state->lagtime / spf);
      state->counter = counter;
      state->target = *in;
      state->slope = (state->target - state->level) / counter;
    }

    if (state->counter > 0) {
      state->counter--;
      state->level += state->slope;

      state->slope = (state->target - state->level) / state->counter;
      *out = state->level;
    } else {
      state->counter = 0;
      *out = *in;
      state->level = *in;
    }
    out++;
    in++;
  }
}

Node *lag_sig(double lagtime, Signal *in) {
  lag_state *state = malloc(sizeof(lag_state));
  state->lagtime = lagtime;
  state->counter = 0;
  state->level = in->buf[0];
  state->target = in->buf[0];

  Node *s = node_new(state, (node_perform *)lag_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in;
  s->out = get_sig(1);

  return s;
}

// perform scaling of an input which is between 0-1 to min-max
node_perform scale_perform(Node *node, int nframes, double spf) {
  scale_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double min = state->min;
  double max = state->max;
  while (nframes--) {
    double val = *in;
    val *= max - min;
    val += min;
    *out = val;
    out++;
    in++;
  }
}

// perform scaling of an input which is between -1-1 to min-max
node_perform scale2_perform(Node *node, int nframes, double spf) {
  scale_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double min = state->min;
  double max = state->max;
  while (nframes--) {
    double val = (*in * (0.5)) + 0.5; // scale to 0-1 first
    val *= max - min;
    val += min;
    *out = val;
    out++;
    in++;
  }
}
// scales a node with outputs between 0-1 to values between min & max (linear)
Node *scale_node(double min, double max, Node *in) {
  scale_state *state = malloc(sizeof(scale_state));
  state->min = min;
  state->max = max;
  Node *s = node_new(state, (node_perform *)scale_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in->out;
  s->out = get_sig(in->out->layout);
  return s;
}

// scales a node with outputs between -1-1 to values between min & max (linear)
Node *scale2_node(double min, double max, Node *in) {
  scale_state *state = malloc(sizeof(scale_state));
  state->min = min;
  state->max = max;
  Node *s = node_new(state, (node_perform *)scale2_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in->out;
  s->out = get_sig(in->out->layout);
  return s;
}

//
//
void *sum_signals(double *out, int out_chans, double *in, int in_chans) {
  for (int ch = 0; ch < out_chans; ch++) {
    *(out + ch) += *(in + (ch % in_chans));
  }
}

node_perform sum_perform(Node *node, int nframes, double spf) {
  int num_ins = node->num_ins;
  double *out = node->out->buf;
  int layout = node->out->layout;
  // printf("out %p [%d] (num_ins %d)\n", out, layout, num_ins);

  for (int i = 0; i < nframes; i++) {
    for (int x = 0; x < num_ins; x++) {
      int in_layout = node->ins[x]->layout;
      double *in = node->ins[x]->buf + (i * in_layout);
      // printf("scalar inbuf %p\n", in);
      sum_signals(out, layout, in, in_layout);
    }
    out += layout;
  }
}

Node *sum_nodes(int num, ...) {
  va_list args; // Define a variable to hold the arguments
  va_start(args, num);
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);
  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;

  Node *first = va_arg(args, Node *);
  new_node->out = first->out;
  Node *n;
  for (int i = 0; i < num - 1; i++) {
    n = va_arg(args, Node *);
    new_node->ins[i] = n->out;
    (new_node->ins[i])->layout = n->out->layout;
    // new_node->ins[i]->layout = 1;
  }

  // Clean up the argument list
  va_end(args);

  // return sum;
  return new_node;
}

Node *sum_nodes_arr(int num, Node **nodes) {
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);

  Node *first = *nodes;
  new_node->out = first->out;

  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;
  Node *n;
  for (int i = 1; i < num; i++) {
    n = *(nodes + i);
    new_node->ins[i - 1] = n->out;
  }
  return new_node;
}

Node *mix_nodes_arr(int num, Node **nodes, double *scalars) {
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);

  Node *first = *nodes;
  new_node->out = first->out;

  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;
  Node *n;
  for (int i = 1; i < num; i++) {
    n = *(nodes + i);
    new_node->ins[i - 1] = n->out;
  }
  return new_node;
}

node_perform div_perform(Node *node, int nframes, double spf) {

  Signal *b = node->ins[0];
  int in_chans = b->layout;
  double *in = b->buf;

  Signal *a = node->out;
  double *out = a->buf;

  while (nframes--) {
    for (int ch = 0; ch < a->layout; ch++) {
      *out = *in / *out;
      out++;
    }
    in++;
  }
}

node_perform mul_perform(Node *node, int nframes, double spf) {

  Signal *b = node->ins[0];
  int in_chans = b->layout;
  double *in = b->buf;

  Signal *a = node->out;
  double *out = a->buf;

  while (nframes--) {
    for (int ch = 0; ch < a->layout; ch++) {
      *out = *(in + (ch % in_chans)) * *out;
      // printf("scalar inbuf %p\n", in);
      out++;
    }
    in = in + in_chans;
  }
}
static char *mul_name = "mul";
Node *mul_nodes(Node *a, Node *b) {
  Node *mul = node_new(NULL, mul_perform, &(Signal){}, &(Signal){});

  mul->ins = malloc(sizeof(Signal *));
  mul->ins[0] = b->out;
  mul->out = a->out;
  mul->name = mul_name;
  return mul;
}

Node *mul_scalar_node(double scalar, Node *node) {
  Node *mul = node_new(NULL, mul_perform, NULL, NULL);
  mul->ins = malloc(sizeof(Signal *));
  mul->ins[0] = get_sig_default(1, scalar);
  mul->num_ins = 1;
  mul->out = node->out;
  return mul;
}

Node *add_scalar_node(double scalar, Node *node) {
  Node *add = node_new(NULL, sum_perform, NULL, NULL);
  add->ins = malloc(sizeof(Signal *));
  add->ins[0] = get_sig_default(1, scalar);
  add->num_ins = 1;
  add->out = node->out;
  return add;
}
