#ifndef _LANG_GUI_H
#define _LANG_GUI_H
#include <SDL2/SDL_ttf.h>
extern TTF_Font *DEFAULT_FONT;
int init_gui();
int gui_loop();

int create_window(void *data, void *renderer, void *event_handler
                  // ,
                  // int num_children, void *children
);

// void render_text(const char *text, int x, int y, SDL_Color color,
//                  SDL_Renderer *renderer);

SDL_Renderer *render_text(const char *text, int x, int y,
                          SDL_Renderer *renderer);

SDL_Renderer *render_scope(char *text, int x, int y, SDL_Renderer *renderer);

int create_scope(double *signal, int layout, int size);

#endif
