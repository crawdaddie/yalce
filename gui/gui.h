#ifndef _LANG_GUI_H
#define _LANG_GUI_H

#include "common.h"
int init_gui();
int gui_loop();

int push_create_window_event(WindowType, void *data);
int _create_scope(double *output, int layout, int size);
int create_array_editor(int32_t size, double *data_ptr);
int _create_plot_array_window(int32_t size, double *data_ptr);

struct _String {
  int32_t length;
  char *chars;
};

int create_slider_window(int32_t size, double *data_ptr, struct _String *labels,
                         void (*on_update)(int, double));

int create_scope(double *output, int layout, int size);

int create_clap_node_slider_window(_clap_slider_window_data *data);
#endif
