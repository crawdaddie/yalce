#include <SDL2/SDL_render.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "common.h"
#include "edit_graph.h"

Window windows[MAX_WINDOWS];

int window_count = 0;

// Custom event type
Uint32 CREATE_WINDOW_EVENT;

bool create_window(WindowType type, void *data) {

  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr,"Maximum number of windows reached.\n");
    return false;
  }

  Window *new_window = &windows[window_count];
  new_window->type = type;
  new_window->width = WINDOW_WIDTH;
  new_window->height = WINDOW_HEIGHT;

  new_window->window = SDL_CreateWindow(
      "New Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      new_window->width, new_window->height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!new_window->window) {
    fprintf(stderr,"Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  new_window->renderer =
      SDL_CreateRenderer(new_window->window, -1, SDL_RENDERER_ACCELERATED);

  if (!new_window->renderer) {
    fprintf(stderr,"Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(new_window->window);
    return false;
  }



  new_window->font = DEFAULT_FONT;

  switch (type) {
    case WINDOW_TYPE_ARRAY_EDITOR: {
      printf("create array editor window\n");
      new_window->render_fn = draw_graph;
      new_window->handle_event = handle_array_editor_events;
    }
  }
  window_count++;
  return true;
}

typedef struct {
  WindowType type;
  void *data;
} WindowCreationData;

// Function to push a create window event to the SDL event queue
int push_create_window_event(WindowType type, void *data) {

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_WINDOW_EVENT;

  WindowCreationData *creation_data = malloc(sizeof(WindowCreationData));
  creation_data->type = type;
  creation_data->data = data;

  event.user.data1 = creation_data;
  return SDL_PushEvent(&event);
}



void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        for (int i = 0; i < window_count; i++) {
          if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
            SDL_DestroyRenderer(windows[i].renderer);
            SDL_DestroyWindow(windows[i].window);
            for (int j = i; j < window_count - 1; j++) {
              windows[j] = windows[j + 1];
            }
            break;
          }
        }
      } else {
        for (int i = 0; i < window_count; i++) {
          if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
            if (windows[i].handle_event != NULL) {
              windows[i].handle_event(windows + i, &event);
            }
            break;
          }
        }

      }
      break;

    default:
      if (event.type == CREATE_WINDOW_EVENT) {
        WindowCreationData *creation_data =
            (WindowCreationData *)event.user.data1;
        create_window(creation_data->type, creation_data->data);
        free(creation_data);
      }
      break;
    }
  }
}




int gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  // Register custom event
  CREATE_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_WINDOW_EVENT == (Uint32)-1) {
    printf("Failed to register custom event\n");
    return 1;
  }

  while (true) {

    handle_events();

    for (int i = 0; i < window_count; i++) {
      if (windows[i].type == WINDOW_TYPE_BASIC) {
        SDL_SetRenderDrawColor(windows[i].renderer, 0, 0, 0, 255);
        SDL_RenderClear(windows[i].renderer);
        SDL_RenderPresent(windows[i].renderer);
      } else if (windows[i].render_fn != NULL) {
        windows[i].render_fn(windows + i);
      }
    }

    SDL_Delay(16); // Cap at roughly 60 fps
  }

  return 0;
}
