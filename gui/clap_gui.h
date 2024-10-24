#ifndef _LANG_CLAP_GUI_H
#define _LANG_CLAP_GUI_H

#include "common.h"
#include <clap/ext/gui.h>
#include <clap/plugin.h>

typedef struct clap_plugin_specs {
  int num_params;
  double *param_vals;
  double *min_vals;
  double *max_vals;
  char **labels;
  char *name;
} clap_plugin_specs;

typedef struct {
  void *target;
  clap_plugin_specs *specs;
  int active_slider;
  double *unit_vals;
} clap_ui_window_t;

void *init_clap_ui_window(Window *window, clap_ui_window_t *data);

#endif
