#ifndef _SIGNAL_ARITHMETIC_H
#define _SIGNAL_ARITHMETIC_H

#include "node.h"
typedef struct {
  double lagtime;
  double target;
  double level;
  double slope;
  int counter;
} lag_state;

node_perform lag_perform(Node *node, int nframes, double spf);

Node *lag_sig(double lagtime, Signal *in);

typedef struct {
  double min;
  double max;
} scale_state;

node_perform scale_perform(Node *node, int nframes, double spf);

node_perform scale2_perform(Node *node, int nframes, double spf);

Node *scale_node(double min, double max, Node *in);

Node *scale2_node(double min, double max, Node *in);

void *sum_signals(double *out, int out_chans, double *in, int in_chans);

node_perform sum_perform(Node *node, int nframes, double spf);

Node *sum_nodes(int num, ...);

Node *sum_nodes_arr(int num, Node **nodes);

Node *mix_nodes_arr(int num, Node **nodes, double *scalars);

node_perform div_perform(Node *node, int nframes, double spf);

node_perform mul_perform(Node *node, int nframes, double spf);

Node *mul_nodes(Node *a, Node *b);

Node *mul_scalar_node(double scalar, Node *node);

Node *add_scalar_node(double scalar, Node *node);
#endif
