#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H

#include "node.h"

void maketable_sq(void);
node_perform sq_perform(Node *node, int nframes, double spf);
node_perform sin_perform(Node *node, int nframes, double spf);
void maketable_sin(void);
Node *bufplayer_node(Signal *buf, Signal *rate, Signal *start_pos);
Node *white_noise_node();
Node *brown_noise_node();
Node *sin_node(Signal *freq);
Node *sq_node(Signal *freq);

#endif
