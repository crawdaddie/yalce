#ifndef _ENGINE_GLUE_COMP_H
#define _ENGINE_GLUE_COMP_H

#include "node.h"
NodeRef glue_node(sample_t threshold, sample_t ratio, sample_t attack_ms,
                  sample_t release_ms, sample_t makeup_gain,
                  sample_t knee_width, NodeRef input);
#endif
