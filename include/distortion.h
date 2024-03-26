#ifndef _DISTORTION_H
#define _DISTORTION_H
// ------------------------------------------- DISTORTION
#include "node.h"
typedef struct {
  double gain;
} tanh_state;

node_perform tanh_perform(Node *node, int nframes, double spf);

Node *tanh_node(double gain, Node *node);
#endif
