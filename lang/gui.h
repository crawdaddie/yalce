#ifndef _LANG_GUI_H
#define _LANG_GUI_H

#include <SDL2/SDL.h>

int gui_main(int argc, char **argv);

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
} WindowPair;
WindowPair create_dynamic_window(const char *title, int x, int y, int w, int h);
#endif
