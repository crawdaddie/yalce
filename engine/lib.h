#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "node.h"

// Node *sq_node(double freq);
Node *sin_node(double freq);
void group_add_tail(Node *group, Node *node);
Node *sum_nodes(Node *l, Node *r);

void sum_perform(void *_state, double *out, int num_ins, double **inputs,
                 int nframes, double spf);

void mul_perform(void *_state, double *out, int num_ins, double **inputs,
                 int nframes, double spf);
#endif
