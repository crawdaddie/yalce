#ifndef GROUP_H
#define GROUP_H
#include "ctx.h"
#include "node.h"

void *perform_ensemble(Node *node, node_group *state, Node *_inputs[],
                       int nframes, double spf);
#endif // GROUP_H
