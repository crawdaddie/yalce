#ifndef _ENGINE_CLAP_NODE_H
#define _ENGINE_CLAP_NODE_H
#include "node.h"
#include <stdint.h>

NodeRef clap_node(const char *plugin_path, SignalRef input);
NodeRef set_clap_param(NodeRef node, int idx, double value);

#endif
