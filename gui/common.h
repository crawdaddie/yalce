#ifndef _GUI_COMMON_H
#define _GUI_COMMON_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>

typedef struct {
  int size;
  double *data;
} _ArrDouble;

typedef struct {
  int32_t size;
  bool *data;
} _ArrBool;

extern Uint32 CREATE_WINDOW_EVENT;
extern Uint32 CREATE_OPENGL_WINDOW_EVENT;

typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(void *window, SDL_Renderer *renderer);
typedef bool (*GLWindowInitFn)(void *state);

typedef struct window_creation_data {
  void *handle_event;
  void *render_fn;
  void *data;
  GLWindowInitFn init_gl;
} window_creation_data;

typedef struct Window {
  SDL_Window *window;
  SDL_Renderer *renderer;
  void *data;
  int width;
  int height;
  EventHandler handle_event; // Function pointer for event handling
  WindowRenderFn render_fn;
  int num_children;
  struct Window *children;
} Window;

SDL_Renderer *render_text(const char *text, int x, int y,
                          SDL_Renderer *renderer, SDL_Color text_color);

Window *get_window(SDL_Event event);
#endif
