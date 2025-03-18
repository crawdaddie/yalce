#ifndef _ENGINE_EXT_LIB_H
#define _ENGINE_EXT_LIB_H
#include "./audio_graph.h"

void start_blob();
AudioGraph *end_blob();

NodeRef inlet(double default_val);
NodeRef play_node(NodeRef s);
NodeRef set_input_scalar_offset(NodeRef target, int input, int offset,
                                double val);
NodeRef set_input_trig_offset(NodeRef target, int input, int frame_offset);
NodeRef set_input_buf(int input, NodeRef buf, NodeRef node);
NodeRef play_node_offset(int offset, NodeRef s);
NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node);
NodeRef load_soundfile(const char *path);

NodeRef trigger_gate(int offset, double dur, int gate_in, NodeRef s);
#endif
