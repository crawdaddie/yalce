#include "./node.h"
typedef NodeRef (*wrap_opaque_ref_in_node_t)(void *opaque_ref, void *perform,
                                             int out_chans, NodeRef input);
