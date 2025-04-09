#ifndef _ENGINE_OSC_H
#define _ENGINE_OSC_H
#include "./node.h"
void maketable_sin(void);
void maketable_sq(void);
void maketable_saw(void);

NodeRef sin_node(NodeRef input);

NodeRef sq_node(NodeRef input);
NodeRef sq_pwm_node(NodeRef freq_input, NodeRef pw_input);
NodeRef phasor_node(NodeRef input);

NodeRef raw_osc_node(NodeRef table, NodeRef freq);

NodeRef osc_bank_node(NodeRef amps, NodeRef freq);
NodeRef bufplayer_node(NodeRef buf, NodeRef rate);
NodeRef bufplayer_trig_node(NodeRef buf, NodeRef rate, NodeRef start_pos,
                            NodeRef trig);
NodeRef white_noise_node();

NodeRef brown_noise_node();
NodeRef chirp_node(double start_freq, double end_freq, NodeRef lag_time,
                   NodeRef trig);
NodeRef impulse_node(NodeRef freq);
NodeRef ramp_node(NodeRef freq);

NodeRef trig_rand_node(NodeRef trig);

NodeRef trig_sel_node(NodeRef trig, NodeRef sels);
NodeRef granulator_node(int max_grains, NodeRef buf, NodeRef trig, NodeRef pos,
                        NodeRef rate);

NodeRef pm_node(NodeRef freq_input, NodeRef mod_index_input,
                NodeRef mod_ratio_input);

NodeRef lfnoise_node(NodeRef freq_input, NodeRef min_input, NodeRef max_input);

NodeRef saw_node(NodeRef input);

NodeRef rand_trig_node(NodeRef trig_input, NodeRef low, NodeRef high);

NodeRef grain_osc_node(int max_grains, NodeRef buf, NodeRef trig, NodeRef pos,
                       NodeRef rate, NodeRef width
                       );

void maketable_grain_window();
#endif
