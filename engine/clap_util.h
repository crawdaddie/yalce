#ifndef _ENGINE_CLAP_UTIL_H
#define _ENGINE_CLAP_UTIL_H

#include "node.h"

#include <stdint.h>

void export_param_specs(uint32_t pc, double *param_vals, double *min_vals,
                        double *max_vals, char **labels, NodeRef node);

uint32_t get_param_num(NodeRef);

typedef struct clap_plugin_specs {
  int num_params;
  double *param_vals;
  double *min_vals;
  double *max_vals;
  char **labels;
  char *name;
} clap_plugin_specs;

clap_plugin_specs *get_specs(void *state);
#endif
