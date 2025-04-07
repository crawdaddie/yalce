#include "gui.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

TTF_Font *DEFAULT_FONT;

void render_text(const char *text, int x, int y, SDL_Renderer *renderer) {
  SDL_Color text_color = {0, 0, 0, 255}; // black

  SDL_Surface *surface = TTF_RenderText_Blended(DEFAULT_FONT, text, text_color);

  // Create a texture from the surface
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
    SDL_FreeSurface(surface);
    return;
  }

  SDL_Rect rect = {x, y, surface->w, surface->h};

  SDL_RenderCopy(renderer, texture, NULL, &rect);

  // Clean up
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(void *window, SDL_Renderer *renderer);

typedef struct window_creation_data {
  void *handle_event;
  void *render_fn;
  void *data;
} window_creation_data;

bool _create_window(window_creation_data *data);

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

Window windows[MAX_WINDOWS];

int window_count = 0;

struct _String {
  int32_t length;
  char *chars;
};

Uint32 CREATE_WINDOW_EVENT;
int init_gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() == -1) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    return 1;
  }
  DEFAULT_FONT = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 16);
  if (!DEFAULT_FONT) {
    fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    return 1;
  }

  CREATE_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_WINDOW_EVENT == (Uint32)-1) {
    printf("Failed to register custom event\n");
    return 1;
  }

  return 0;
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == CREATE_WINDOW_EVENT) {
      // handle window creation
      //
      _create_window(event.user.data1);
    } else {
      for (int i = 0; i < window_count; i++) {
        if (SDL_GetWindowID(windows[i].window) == event.window.windowID &&
            windows[i].handle_event) {
          windows[i].handle_event(windows + i, &event);
        }
      }
    }
  }
}

void render_window(Window *window) {
  printf("render window %p\n", window->render_fn);
  window->render_fn(window->data, window->renderer);

  for (int i = 0; i < window->num_children; i++) {
    render_window(window->children + i);
  }
}

int gui_loop() {
  while (true) {

    handle_events();

    for (int i = 0; i < window_count; i++) {

      if (windows[i].render_fn) {
        SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
        SDL_RenderClear(windows[i].renderer);

        windows[i].render_fn(windows[i].data, windows[i].renderer);
        SDL_RenderPresent(windows[i].renderer);
      }
    }

    SDL_Delay(16); // Cap at roughly 60 fps
  }

  return 0;
}

bool _create_window(window_creation_data *data) {
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }
  int win_idx = window_count;
  window_count++;

  windows[win_idx].width = WINDOW_WIDTH;
  windows[win_idx].height = WINDOW_HEIGHT;

  windows[win_idx].render_fn = data->render_fn;
  windows[win_idx].handle_event = data->handle_event;

  windows[win_idx].window = SDL_CreateWindow(
      "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      windows[win_idx].width, windows[win_idx].height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  windows[win_idx].renderer =
      SDL_CreateRenderer(windows[win_idx].window, -1, SDL_RENDERER_ACCELERATED);

  printf("created window %d\n", win_idx);

  free(data);
  return true;
}

// Function to push a create window event to the SDL event queue
int create_window(void *data, void *renderer, void *event_handler
                  // ,
                  // int num_children, void *children

) {

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_WINDOW_EVENT;

  window_creation_data *cdata = malloc(sizeof(window_creation_data));
  cdata->render_fn = renderer;
  cdata->handle_event = event_handler;
  printf("created event data %p renderer %p handler %p\n", data,
         cdata->render_fn, cdata->handle_event);
  // cdata->data = data;

  event.user.data1 = cdata;
  return SDL_PushEvent(&event);
}
