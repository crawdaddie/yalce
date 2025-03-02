#ifndef _ENGINE_FILTERS_H
#define _ENGINE_FILTERS_H
#include "node.h"

NodeRef biquad_node(SignalRef in);
NodeRef biquad_lp_node(SignalRef freq, SignalRef res, SignalRef in);
NodeRef biquad_hp_node(SignalRef freq, SignalRef res, SignalRef in);
NodeRef biquad_bp_node(SignalRef freq, SignalRef res, SignalRef in);

// NodeRef butterworth_hp_dyn_node(double freq, NodeRef in);
//
NodeRef comb_node(double delay_time, double max_delay_time, double fb,
                  SignalRef input);

NodeRef lag_node(double lag_time, SignalRef in);

NodeRef tanh_node(double gain, SignalRef in);

NodeRef grain_delay_node(double spray, double freq, double pitch,
                         double feedback, double mix, SignalRef input);
#endif
