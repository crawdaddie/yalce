#ifndef _ENGINE_FILTER_H
#define _ENGINE_FILTER_H

#include "./node.h"
NodeRef tanh_node(double gain, NodeRef input);

NodeRef lag_node(NodeRef lag_time, NodeRef input);

NodeRef comb_node(double delay_time, double max_delay_time, double fb,
                  NodeRef input);
NodeRef dyn_comb_node(NodeRef delay_time, double max_delay_time, double fb,
                      NodeRef input);
NodeRef butterworth_hp_node(NodeRef freq, NodeRef input);
NodeRef biquad_hp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_bp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_lp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef gverb_node(NodeRef input);

NodeRef allpass_node(double time, double coeff, NodeRef input);

NodeRef reverb_node(double room_size, double wet, double dry, double width,
                    NodeRef input);

NodeRef grain_pitchshift_node(double shift, double fb, NodeRef input);

NodeRef dyn_tanh_node(NodeRef gain, NodeRef input);

typedef double (*MathNodeFn)(double);
NodeRef math_node(MathNodeFn math_fn, NodeRef input);
#endif
