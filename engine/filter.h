#ifndef _ENGINE_FILTER_H
#define _ENGINE_FILTER_H

#include "./node.h"
NodeRef tanh_node(double gain, NodeRef input);

NodeRef lag_node(NodeRef lag_time, NodeRef input);

NodeRef delay_node(double delay_time, double max_delay_time, double fb,
                   NodeRef input);

NodeRef delay2_node(double delay_time_left, double delay_time_right,
                    double max_delay_time, double fb, NodeRef input);

NodeRef stereo_spread_delay_node(double delay_time, double spread_amount,
                                 double max_delay_time, double fb,
                                 NodeRef input);

NodeRef dyn_delay_node(NodeRef delay_time, double max_delay_time, double fb,
                       NodeRef input);

NodeRef comb_node(double delay_time, double max_delay_time, double fb,
                  double ff, NodeRef input);
NodeRef butterworth_hp_node(NodeRef freq, NodeRef input);
NodeRef biquad_hp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_bp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_lp_node(NodeRef freq, NodeRef res, NodeRef input);

NodeRef allpass_node(double delay_time, double max_delay_time, double g,
                     NodeRef input);

NodeRef reverb_node(double room_size, double wet, double dry, double width,
                    NodeRef input);

NodeRef grain_pitchshift_node(double shift, double fb, NodeRef input);

NodeRef dyn_tanh_node(NodeRef gain, NodeRef input);

typedef double (*MathNodeFn)(double);

NodeRef math_node(MathNodeFn math_fn, NodeRef input);

NodeRef stutter_node(double max_time, NodeRef repeat_time, NodeRef gate,
                     NodeRef input);

NodeRef wrap_opaque_ref_in_node(void *opaque_ref, void *perform, int out_chans,
                                NodeRef input);
#endif
