#ifndef _LANG_GUI_SLIDER_H
#define _LANG_GUI_SLIDER_H

#include "common.h"
void draw_slider_window(Window *window);

void draw_clap_slider_window(Window *window);

void handle_slider_window_events(Window *window, SDL_Event *event);
void handle_clap_slider_window_events(Window *window, SDL_Event *event);
#endif
