#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H
#include "node.h"

NodeRef sin_node(SignalRef input);
NodeRef sq_node(SignalRef input);
NodeRef simple_gate_env_node(double len);

NodeRef adsr_env_node(double attack_time, double decay_time,
                      double sustain_level, double release_time,
                      SignalRef gate);

NodeRef asr_env_node(double attack_time, double release_time, SignalRef gate);
void maketable_sq();
void maketable_sin();

#endif
