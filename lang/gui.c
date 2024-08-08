// #include "gui.h"
// #include "backend_llvm/jit.h"
// #include <SDL2/SDL.h>
// #include <pthread.h>
// #include <stdbool.h>
//
// struct thread_args {
//   int argc;
//   char **argv;
//   int result;
// };
// SDL_Window *create_dynamic_window(const char *title, int x, int y, int w,
//                                   int h) {
//   SDL_Window *window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_SHOWN);
//   if (window == NULL) {
//     fprintf(stderr, "Failed to create dynamic window: %s\n", SDL_GetError());
//     return NULL;
//   }
//   return window;
// }
//
// void *jit_thread(void *args) {
//   struct thread_args *targs = (struct thread_args *)args;
//   int result = jit(targs->argc, targs->argv);
//   targs->result = result;
//   return NULL;
// }
//
// int gui_main(int argc, char **argv) {
//   pthread_t tid;
//   struct thread_args args = {argc, argv, 0};
//
//   if (pthread_create(&tid, NULL, jit_thread, &args) != 0) {
//     fprintf(stderr, "Failed to create JIT thread\n");
//     return 1;
//   }
//
//   // Initialize SDL
//   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//     fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
//     return 1;
//   }
//
//   // Create main window
//   SDL_Window *main_window =
//       SDL_CreateWindow("Main Window", SDL_WINDOWPOS_UNDEFINED,
//                        SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
//   if (main_window == NULL) {
//     fprintf(stderr, "Failed to create main window: %s\n", SDL_GetError());
//     SDL_Quit();
//     return 1;
//   }
//
//   SDL_Renderer *renderer =
//       SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED);
//   if (renderer == NULL) {
//     fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
//     SDL_DestroyWindow(main_window);
//     SDL_Quit();
//     return 1;
//   }
//
//   SDL_Window *dynamic_window = NULL;
//   bool quit = false;
//   SDL_Event event;
//
//   while (!quit) {
//     while (SDL_PollEvent(&event)) {
//       if (event.type == SDL_QUIT) {
//         quit = true;
//       } else if (event.type == SDL_KEYDOWN) {
//         if (event.key.keysym.sym == SDLK_n) {
//           // Create a new dynamic window when 'N' key is pressed
//           if (dynamic_window == NULL) {
//             dynamic_window =
//                 create_dynamic_window("Dynamic Window",
//                 SDL_WINDOWPOS_UNDEFINED,
//                                       SDL_WINDOWPOS_UNDEFINED, 400, 300);
//           }
//         }
//       }
//     }
//
//     // Clear the renderer
//     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//     SDL_RenderClear(renderer);
//
//     // Render your game/application here
//
//     // Present the renderer
//     SDL_RenderPresent(renderer);
//   }
//
//   // Clean up
//   if (dynamic_window != NULL) {
//     SDL_DestroyWindow(dynamic_window);
//   }
//   SDL_DestroyRenderer(renderer);
//   SDL_DestroyWindow(main_window);
//   SDL_Quit();
//
//   // Wait for the JIT thread to finish
//   pthread_join(tid, NULL);
//
//   return args.result;
// }
#include "gui.h"
#include "backend_llvm/jit.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdbool.h>

struct thread_args {
  int argc;
  char **argv;
  int result;
};

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
} WindowPair;

#define MAX_WINDOWS 10
WindowPair windows[MAX_WINDOWS];
int window_count = 0;

WindowPair create_dynamic_window(const char *title, int x, int y, int w,
                                 int h) {
  WindowPair wp = {NULL, NULL};
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached\n");
    return wp;
  }

  wp.window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_SHOWN);
  if (wp.window == NULL) {
    fprintf(stderr, "Failed to create dynamic window: %s\n", SDL_GetError());
    return wp;
  }

  wp.renderer = SDL_CreateRenderer(wp.window, -1, SDL_RENDERER_ACCELERATED);
  if (wp.renderer == NULL) {
    fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(wp.window);
    wp.window = NULL;
    return wp;
  }

  windows[window_count++] = wp;
  return wp;
}

void *jit_thread(void *args) {
  struct thread_args *targs = (struct thread_args *)args;
  int result = jit(targs->argc, targs->argv);
  targs->result = result;
  return NULL;
}

int gui_main(int argc, char **argv) {
  pthread_t tid;
  struct thread_args args = {argc, argv, 0};

  if (pthread_create(&tid, NULL, jit_thread, &args) != 0) {
    fprintf(stderr, "Failed to create JIT thread\n");
    return 1;
  }

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  bool quit = false;
  SDL_Event event;

  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = true;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_n) {
          // Create a new dynamic window when 'N' key is pressed
          WindowPair wp =
              create_dynamic_window("Dynamic Window", SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED, 400, 300);
          if (wp.window == NULL || wp.renderer == NULL) {
            fprintf(stderr, "Failed to create new window\n");
          }
        }
      } else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
          // Handle window close event
          for (int i = 0; i < window_count; i++) {
            if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
              SDL_DestroyRenderer(windows[i].renderer);
              SDL_DestroyWindow(windows[i].window);
              // Shift remaining windows
              for (int j = i; j < window_count - 1; j++) {
                windows[j] = windows[j + 1];
              }
              window_count--;
              break;
            }
          }
        }
      }
    }

    // Render all windows
    for (int i = 0; i < window_count; i++) {
      SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
      SDL_RenderClear(windows[i].renderer);

      // Render your game/application here for each window

      SDL_RenderPresent(windows[i].renderer);
    }

    // Add a small delay to prevent high CPU usage
    SDL_Delay(16); // roughly 60 fps
  }

  // Clean up
  for (int i = 0; i < window_count; i++) {
    SDL_DestroyRenderer(windows[i].renderer);
    SDL_DestroyWindow(windows[i].window);
  }
  SDL_Quit();

  // Wait for the JIT thread to finish
  pthread_join(tid, NULL);

  return args.result;
}
