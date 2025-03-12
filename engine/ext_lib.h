#ifndef _ENGINE_EXT_LIB_H
#define _ENGINE_EXT_LIB_H
#include "./audio_graph.h"

void start_blob();
AudioGraph *end_blob();

NodeRef inlet(double default_val);

NodeRef play_node(NodeRef s);

void set_input_scalar_offset(NodeRef target, int input, int offset, double val);

NodeRef play_node_offset(int offset, NodeRef s);
#endif
