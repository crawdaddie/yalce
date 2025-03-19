#include "./gui.h"
#include "./common.h"
#include "./ext_lib.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

TTF_Font *DEFAULT_FONT;
typedef struct {
  void *renderer;
  void *event_handler;
} wfuncs;

Window windows[MAX_WINDOWS];

int window_count = 0;

Uint32 CREATE_WINDOW_EVENT;
bool gui_thread_create_window(void *state, wfuncs *funcs) {
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }
  Window *new_window = &windows[window_count];

  new_window->width = WINDOW_WIDTH;
  new_window->height = WINDOW_HEIGHT;
  new_window->font = DEFAULT_FONT;
  window_count++;

  const char *wname = "New Window";

  new_window->window = SDL_CreateWindow(
      wname, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      new_window->width, new_window->height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!new_window->window) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  new_window->renderer =
      SDL_CreateRenderer(new_window->window, -1, SDL_RENDERER_ACCELERATED);

  if (!new_window->renderer) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(new_window->window);
    return false;
  }

  new_window->data = state;
  new_window->render_fn = funcs->renderer;
  new_window->handle_event = funcs->event_handler;

  return true;
}
void render_text(Window *w, const char *text) {
  _render_text(text, 0, 0, (SDL_Color){0, 0, 0, 255}, w->renderer,
               DEFAULT_FONT);
}
// Function to push a create window event to the SDL event queue

int push_create_window_event(void *data, void *renderer, void *event_handler) {

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_WINDOW_EVENT;

  event.user.data1 = data;
  wfuncs *funcs = malloc(sizeof(wfuncs));
  *funcs = (wfuncs){renderer, event_handler};
  event.user.data2 = funcs;
  return SDL_PushEvent(&event);
}

int create_window(void *data, void *renderer, void *event_handler) {
  printf("create window??? %p %p\n", renderer, event_handler);
  push_create_window_event(data, renderer, event_handler);
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_WINDOWEVENT:
      for (int i = 0; i < window_count; i++) {
        if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
          switch (event.window.event) {
          case SDL_WINDOWEVENT_CLOSE:
            SDL_DestroyRenderer(windows[i].renderer);
            SDL_DestroyWindow(windows[i].window);
            for (int j = i; j < window_count - 1; j++) {
              windows[j] = windows[j + 1];
            }
            window_count--;
            break;

          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED:
            windows[i].width = event.window.data1;
            windows[i].height = event.window.data2;
            // Optionally, update logical size if you're using it
            SDL_RenderSetLogicalSize(windows[i].renderer, windows[i].width,
                                     windows[i].height);
            break;

          default:
            if (windows[i].handle_event != NULL) {
              windows[i].handle_event(&windows[i], event);
            }
            break;
          }
          break; // Break the for loop, we've found our window
        }
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION: {
      SDL_Window *mouse_window = SDL_GetWindowFromID(event.window.windowID);
      for (int i = 0; i < window_count; i++) {
        if (windows[i].window == mouse_window) {
          if (windows[i].handle_event != NULL) {
            windows[i].handle_event(&windows[i], event);
          }
          break;
        }
      }
    } break;

    default:
      if (event.type == CREATE_WINDOW_EVENT) {
        void *state = event.user.data1;
        gui_thread_create_window(event.user.data1, event.user.data2);
      }
      break;
    }
  }
}

int init_gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() == -1) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    return 1;
  }
  DEFAULT_FONT = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 12);
  if (!DEFAULT_FONT) {
    fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    return 1;
  }

  CREATE_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_WINDOW_EVENT == (Uint32)-1) {
    fprintf(stderr, "Failed to register custom event\n");
    return 1;
  }

  return 0;
}

int gui_loop() {
  while (true) {

    handle_events();

    for (int i = 0; i < window_count; i++) {
      if (windows[i].render_fn != NULL) {

        SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
        SDL_RenderClear(windows[i].renderer);
        windows[i].render_fn((windows + i));
        SDL_RenderPresent(windows[i].renderer);
      }
    }

    SDL_Delay(16); // Cap at roughly 60 fps
  }

  return 0;
}
