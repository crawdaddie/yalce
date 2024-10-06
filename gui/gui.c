#include <SDL2/SDL.h>
#include <stdbool.h>
#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

SDL_Window *windows[MAX_WINDOWS];
SDL_Renderer *renderers[MAX_WINDOWS];
int window_count = 0;

bool create_window() {
  if (window_count >= MAX_WINDOWS) {
    printf("Maximum number of windows reached.\n");
    return false;
  }

  windows[window_count] = SDL_CreateWindow(
      "New Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!windows[window_count]) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  renderers[window_count] =
      SDL_CreateRenderer(windows[window_count], -1, SDL_RENDERER_ACCELERATED);
  if (!renderers[window_count]) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(windows[window_count]);
    return false;
  }

  window_count++;
  return true;
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_n) {
        create_window();
      }
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        for (int i = 0; i < window_count; i++) {
          if (SDL_GetWindowID(windows[i]) == event.window.windowID) {
            SDL_DestroyRenderer(renderers[i]);
            SDL_DestroyWindow(windows[i]);
            // Shift remaining windows and renderers
            for (int j = i; j < window_count - 1; j++) {
              windows[j] = windows[j + 1];
              renderers[j] = renderers[j + 1];
            }
            window_count--;
            break;
          }
        }
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

  if (!create_window()) {
    SDL_Quit();
    return 1;
  }

  // printf("Press 'N' to create a new window. Close a window to destroy
  // it.\n");

  while (true) {
    handle_events();

    for (int i = 0; i < window_count; i++) {
      SDL_SetRenderDrawColor(renderers[i], 0, 0, 0, 255);
      SDL_RenderClear(renderers[i]);
      SDL_RenderPresent(renderers[i]);
    }

    SDL_Delay(16); // Cap at roughly 60 fps
  }

  return 0;
}
