#ifndef _LANG_GUI_COMMON_H
#define _LANG_GUI_COMMON_H
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_video.h"
#define DEFAULT_FONT TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 12)
#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef enum { WINDOW_TYPE_BASIC, WINDOW_TYPE_ARRAY_EDITOR } WindowType;
typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(Window *window);

typedef struct Window {
  SDL_Window *window;
  SDL_Renderer *renderer;
  WindowType type;
  void *data;
  TTF_Font *font;
  int width;
  int height;
  EventHandler handle_event; // Function pointer for event handling
  WindowRenderFn render_fn;
} Window;

#endif
