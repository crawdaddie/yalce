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
                          SDL_Renderer *renderer, SDL_Color text_color);

SDL_Renderer *render_scope(char *text, int x, int y, SDL_Renderer *renderer);

int create_scope(double *signal, int layout, int size);

int create_static_plot(int layout, int size, double *signal);

int create_envelope_edit_view(int size, double *data);
int create_envelope_edit_view_cb(int size, double *data, void *cb);
int create_vst_view(char *handle);

#endif
