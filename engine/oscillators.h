#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H
#include "node.h"

NodeRef tanh_node(SignalRef input);
NodeRef sin_node(SignalRef input);
void maketable_sq();
void maketable_sin();
#endif
