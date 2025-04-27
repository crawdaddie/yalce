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
