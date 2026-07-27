#ifndef _ENGINE_CORE_H
#define _ENGINE_CORE_H

#include "ctx.h"
#include "node.h"
#include <stdint.h>

void audio_engine_render(Ctx *ctx, int frame_count, double spf);
void audio_engine_mark_dirty(Ctx *ctx);

void write_to_dac(int dac_layout, double *dac_buf, int layout, double *buf,
                  int output_num, int nframes);
double ylc_read_inlet_node(void *node_raw, int64_t frame);

NodeRef play_node(NodeRef node);
NodeRef play_node_offset(uint64_t tick, NodeRef node);
NodeRef play_node_before(NodeRef target, NodeRef node);
NodeRef play_node_dur(uint64_t tick, double dur, int gate_in, NodeRef node);

NodeRef set_input_scalar(NodeRef node, int input, double value);
NodeRef set_input_scalar_offset(NodeRef node, int input, uint64_t tick,
                                double value);
NodeRef set_input_trig(NodeRef node, int input);
NodeRef set_input_trig_offset(NodeRef node, int input, uint64_t tick);
NodeRef set_input_buf(int input, NodeRef buf, NodeRef node);
NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node);

NodeRef const_sig(double value);
void node_connect_input(int idx, NodeRef node, NodeRef input);
void plug_input_in_graph(int idx, NodeRef node, NodeRef input);
NodeRef pipe_into(NodeRef filter, int idx, NodeRef node);

NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef mul2_node(NodeRef input1, NodeRef input2);
NodeRef sub2_node(NodeRef input1, NodeRef input2);
NodeRef div2_node(NodeRef input1, NodeRef input2);
NodeRef mod2_node(NodeRef input1, NodeRef input2);

NodeRef sin_node(NodeRef freq);
NodeRef sq_node(NodeRef freq);
NodeRef saw_node(NodeRef freq);
NodeRef pm_node(NodeRef freq, NodeRef mod_index, NodeRef mod_ratio);

double midi_to_freq(int midi_note);
double dmidi_to_freq(double midi_note);
double semi_to_ratio(int semitones);

double *ctx_main_out(void);
SignalRef node_out(NodeRef node);
double *sig_raw(SignalRef sig);
int sig_size(SignalRef sig);
int sig_layout(SignalRef sig);

struct arr {
  int size;
  void *data;
};

NodeRef inlet(double default_val);
NodeRef hw_inlet(int idx);
NodeRef multi_chan_inlet(int layout, double default_val);
NodeRef buf_ref(NodeRef buf);

NodeRef array_to_buf(struct arr a);
NodeRef render_to_buf(int frames, NodeRef node);
void node_replace(NodeRef a, NodeRef b);
NodeRef null_synth_node(void);
NodeRef chain(NodeRef tail);
NodeRef play_node_offset_w_kill(uint64_t tick, double dur, int gate_in,
                                NodeRef node);
NodeRef play_into(NodeRef target, NodeRef node);
NodeRef play_into_offset(uint64_t tick, NodeRef target, NodeRef node);
NodeRef play_into_idx(NodeRef target, int idx, NodeRef node);

void set_main_vol(double vol);

#endif
