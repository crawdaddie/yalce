#ifndef _LANG_GUI_H
#define _LANG_GUI_H

#include "common.h"
int gui();

int push_create_window_event(WindowType, void *data);
int _create_scope(double *output);
int create_array_editor(int32_t size, double *data_ptr);
int create_slider_window(int32_t size, double *data_ptr);
#endif
