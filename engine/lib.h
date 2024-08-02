#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "node.h"

// Node *sq_node(double freq);
// Node *sin_node(double freq);
// // void group_add_tail(Node *group, Node *node);
// Node *sum_nodes(Node *l, Node *r);
//
// node_perform sum_perform(Node *node, int nframes, double spf);
//
// node_perform mul_perform(Node *node, int nframes, double spf);
//
Node *play_test_synth();

Node *play_node(Node *s);

void accept_callback(int (*callback)(int, int));
#endif
