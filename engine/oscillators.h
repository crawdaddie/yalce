#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H

#include "node.h"


void maketable_sq(void);

void maketable_sin(void);

NodeRef bufplayer_node(SignalRef buf, SignalRef rate, SignalRef start_pos,
                     SignalRef trig);

NodeRef bufplayer_1shot_node(SignalRef buf, SignalRef rate, SignalRef start_pos,
                           SignalRef trig);
NodeRef white_noise_node();
NodeRef brown_noise_node();
NodeRef sin_node(SignalRef freq);
NodeRef sq_node(SignalRef freq);

NodeRef chirp_node(double start, double end, SignalRef lag_time, SignalRef trig);

/** comment text */
NodeRef nbl_impulse_node(SignalRef freq);

NodeRef ramp_node(SignalRef freq);

NodeRef trig_rand_node(SignalRef trig);
NodeRef granulator_node(int max_concurrent_grains, SignalRef buf, SignalRef trig,
                      SignalRef pos, SignalRef rate);

NodeRef trig_sel_node(SignalRef trig, SignalRef sels);

#endif
