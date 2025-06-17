#ifndef GROUP_H
#define GROUP_H
#include "ctx.h"
#include "node.h"

void *perform_ensemble(Node *node, ensemble_state *state, Node *_inputs[],
                       int nframes, float spf);
#endif // GROUP_H
