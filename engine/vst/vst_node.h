#ifndef VST_NODE_H
#define VST_NODE_H

#include "../node.h"

/**
 * Create a new VST plugin node
 *
 * @param path Path to the VST plugin (.dll, .so, .vst, or .vst3)
 * @param input Input node (can be NULL for no input)
 * @return A new node reference
 */
NodeRef vstfx_node(const char *path, NodeRef input);
NodeRef vstfx_load_preset(const char *preset_path, NodeRef vst_node);
char *vst_get_handle(NodeRef vst_node);

#endif /* VST_NODE_H */
