#include "audio_graph.h"
#include "ctx.h"
#include "node.h"

NodeRef temper_node(double saturation, double feedback, double cutoff,
                    double resonance, double drive, double curve, double level,
                    NodeRef input);

NodeRef vital_rev_node(double size, double amount, double rate,
                       double high_shelf, double low_shelf, double high_gain,
                       double low_gain, double decay_time, double high_cutoff,
                       double low_cutoff, double pre_delay, double mix,
                       NodeRef input);

NodeRef zita_rev_node(double in_delay, double lf_x, double low_rt60, double mid_rt60, 
                     double hf_damping, double eq1_freq, double eq1_level, 
                     double eq2_freq, double eq2_level, double wet_dry_mix, 
                     double level, NodeRef input);

// Helper function to create a reverb with recommended preset values
NodeRef zita_rev_preset(NodeRef input);
// Create a zita_rev with "plate" like characteristics
NodeRef zita_rev_plate(NodeRef input);
// Create a zita_rev with "hall" like characteristics
NodeRef zita_rev_hall(NodeRef input);

NodeRef imp_train_node(NodeRef freq);
