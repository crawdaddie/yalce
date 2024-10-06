#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H

#include "node.h"

void maketable_sq(void);
node_perform sq_perform(Node *node, int nframes, double spf);
node_perform sin_perform(Node *node, int nframes, double spf);
void maketable_sin(void);
// Node *bufplayer_node(Signal *buf, Signal *rate, Signal *start_pos);
Node *bufplayer_node(Signal *buf, Signal *rate, Signal *start_pos,
                     Signal *trig);

Node *bufplayer_1shot_node(Signal *buf, Signal *rate, Signal *start_pos,
                     Signal *trig); 
Node *white_noise_node();
Node *brown_noise_node();
Node *sin_node(Signal *freq);
Node *sq_node(Signal *freq);

Node *chirp_node(double start, double end, Signal * lag_time, Signal *trig);

#endif
