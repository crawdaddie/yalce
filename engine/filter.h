#ifndef _ENGINE_FILTER_H
#define _ENGINE_FILTER_H

#include "./node.h"
NodeRef tanh_node(sample_t gain, NodeRef input);

NodeRef lag_node(NodeRef lag_time, NodeRef input);

NodeRef delay_node(sample_t delay_time, sample_t max_delay_time, sample_t fb,
                   NodeRef input);

NodeRef delay2_node(sample_t delay_time_left, sample_t delay_time_right,
                    sample_t max_delay_time, sample_t fb, NodeRef input);

NodeRef stereo_spread_delay_node(sample_t delay_time, sample_t spread_amount,
                                 sample_t max_delay_time, sample_t fb,
                                 NodeRef input);

NodeRef dyn_delay_node(NodeRef delay_time, sample_t max_delay_time, sample_t fb,
                       NodeRef input);

NodeRef comb_node(sample_t delay_time, sample_t max_delay_time, sample_t fb,
                  sample_t ff, NodeRef input);
NodeRef butterworth_hp_node(NodeRef freq, NodeRef input);
NodeRef biquad_hp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_bp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_lp_node(NodeRef freq, NodeRef res, NodeRef input);

NodeRef allpass_node(sample_t delay_time, sample_t max_delay_time, sample_t g,
                     NodeRef input);

NodeRef reverb_node(sample_t room_size, sample_t wet, sample_t dry,
                    sample_t width, NodeRef input);

NodeRef grain_pitchshift_node(sample_t shift, sample_t fb, NodeRef input);

NodeRef dyn_tanh_node(NodeRef gain, NodeRef input);

typedef double (*MathNodeFn)(double);

NodeRef math_node(MathNodeFn math_fn, NodeRef input);

NodeRef stutter_node(sample_t max_time, NodeRef repeat_time, NodeRef gate,
                     NodeRef input);

NodeRef wrap_opaque_ref_in_node(void *opaque_ref, void *perform, int out_chans,
                                NodeRef input);
#endif
