#ifndef _ENGINE_LFO_H
#define _ENGINE_LFO_H
#include "node.h"
NodeRef lfo_node(NodeRef input, NodeRef trig);
NodeRef buf_env_node(NodeRef time_scale, NodeRef input, NodeRef trig);
NodeRef lfpulse_node(NodeRef pw, NodeRef freq, NodeRef trig);

NodeRef perc_env_node(NodeRef decay, NodeRef trig);
#endif
