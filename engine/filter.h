#ifndef _ENGINE_FILTER_H
#define _ENGINE_FILTER_H

#include "./node.h"
NodeRef tanh_node(double gain, NodeRef input);

NodeRef lag_node(double lag_time, NodeRef input);

NodeRef comb_node(double delay_time, double max_delay_time, double fb,
                  NodeRef input);
NodeRef butterworth_hp_node(NodeRef freq, NodeRef input);
NodeRef biquad_hp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_bp_node(NodeRef freq, NodeRef res, NodeRef input);
NodeRef biquad_lp_node(NodeRef freq, NodeRef res, NodeRef input);

#endif
