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

struct __color {
  int32_t r;
  int32_t g;
  int32_t b;
  int32_t a;
} __color;

// void render_text(const char *text, int x, int y, SDL_Color color,
//                  SDL_Renderer *renderer);

void render_text(const char *text, int x, int y, SDL_Renderer *renderer);
#endif
