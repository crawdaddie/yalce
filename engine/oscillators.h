#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H
#include "node.h"

NodeRef sin_node(SignalRef input);

NodeRef simple_gate_env_node(double len);
void maketable_sq();
void maketable_sin();

#endif
