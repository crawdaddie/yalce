#ifndef _ENGINE_ENVELOPE_H
#define _ENGINE_ENVELOPE_H
#include "./node.h"
NodeRef asr_kill_node(double attack_time, double sustain_level,
                      double release_time, NodeRef trigger);

NodeRef asr_node(double attack_time, double sustain_level, double release_time,
                 NodeRef trigger);

NodeRef aslr_node(double attack_time, double sustain_level, double sustain_time,
                  double release_time, NodeRef trigger);
#endif
