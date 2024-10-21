#ifndef CLAP_HOST_LIB_H
#define CLAP_HOST_LIB_H

#include "node.h"
#include <clap/clap.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __clap_plugin_state {
  const struct clap_plugin *plugin;
  const struct clap_plugin_descriptor *desc;
  void *instance;
  void *entry;
} __clap_plugin_state;

// Function to create a CLAP node
Node *clap_node(const char *plugin_path, int num_inputs, Signal **inputs);

// CLAP perform function (to be used as node_perform)
void clap_perform(Node *node, int nframes, double samplerate);

// Function to destroy a CLAP node
void clap_node_destroy(Node *node);

#ifdef __cplusplus
}
#endif

#endif // CLAP_HOST_LIB_H
