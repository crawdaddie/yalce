#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>

typedef struct {
  int size;
  double *data;
} _DoubleArray;

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
