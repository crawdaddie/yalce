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
#endif
