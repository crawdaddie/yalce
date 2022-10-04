#include "user_ctx.h"
#include <SDL2/SDL.h>
#include <soundio/soundio.h>
#include <time.h>

void sleep_mils(long msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, &ts);
}

int win(UserCtx *ctx) {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Surface *surface;
  SDL_Event event;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s",
                 SDL_GetError());
    return 3;
  }

  if (SDL_CreateWindowAndRenderer(320, 240, SDL_WINDOW_RESIZABLE, &window,
                                  &renderer)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't create window and renderer: %s", SDL_GetError());
    return 3;
  }

  int read = 0;
  while (1) {

    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
      break;
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    for (int i = 0; i < 2048; i++) {
      int j = (read + i) % 2048;
      int x1 = (int)j * 640 / 2048;
      double y1val = 120 * ctx->buses[0][j];
      int y1 = (int)(240 + y1val);

      int x2 = (int)((j + 1) % 2048) * 640 / 2048;
      double y2val = 120 * ctx->buses[0][(j + 1) % 2048];
      int y2 = (int)(240 + y2val);
      SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
    SDL_RenderPresent(renderer);
    sleep_mils(1000 / 48000);
    read = (read + 1) % 2048;
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}
