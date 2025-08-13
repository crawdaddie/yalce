#ifndef _ENGINE_ENVELOPE_H
#define _ENGINE_ENVELOPE_H
#include "./node.h"
NodeRef asr_kill_node(sample_t attack_time, sample_t sustain_level,
                      sample_t release_time, NodeRef trigger);

NodeRef asr_node(sample_t attack_time, sample_t sustain_level,
                 sample_t release_time, NodeRef trigger);

NodeRef aslr_node(sample_t attack_time, sample_t sustain_level,
                  sample_t sustain_time, sample_t release_time,
                  NodeRef trigger);
#endif
