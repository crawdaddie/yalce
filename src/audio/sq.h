#ifndef _SQ_H
#define _SQ_H
#include "../common.h"
#include "../node.h"

typedef struct sq_data {
  double ramp;
} sq_data;
node_perform sq_perform(Node *node, int nframes, double spf);
Node *make_sq_node();

#endif
