#ifndef _ENGINE_EXT_LIB_H
#define _ENGINE_EXT_LIB_H
#include "./audio_graph.h"

void start_blob();
AudioGraph *end_blob();

NodeRef inlet(double default_val);
NodeRef buf_ref(NodeRef buf);
NodeRef play_node(NodeRef s);
NodeRef set_input_scalar_offset(NodeRef target, int input, int offset,
                                double val);

NodeRef set_input_scalar(NodeRef node, int input, double value);
NodeRef set_input_trig_offset(NodeRef target, int input, int frame_offset);
NodeRef set_input_buf(int input, NodeRef buf, NodeRef node);
NodeRef play_node_offset(int offset, NodeRef s);
NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node);
NodeRef load_soundfile(const char *path);

NodeRef trigger_gate(int offset, double dur, int gate_in, NodeRef s);

double midi_to_freq(int midi_note);

double *ctx_main_out();

SignalRef node_out(NodeRef node);
double *sig_raw(SignalRef sig);
int sig_size(SignalRef sig);
int sig_layout(SignalRef sig);

NodeRef render_to_buf(int frames, NodeRef node);

void node_replace(NodeRef a, NodeRef b);

NodeRef null_synth_node();

#endif
