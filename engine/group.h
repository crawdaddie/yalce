#ifndef GROUP_H
#define GROUP_H
#include "ctx.h"
#include "node.h"

NodeRef group_add(NodeRef node, NodeRef group);

void *perform_ensemble(Node *node, node_group_state *state, Node *_inputs[],
                       int nframes, double spf);
#endif // GROUP_H
