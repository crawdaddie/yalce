#include "oscilloscope.h"

int oscilloscope_view(UserCtx *ctx) {
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
      double y1val = 0;
      double y2val = 0;
      for (int i = 0; i < INITIAL_BUSNUM; i++) {
        y1val += ctx->buses[i][j];
        y1val += ctx->buses[i][(j + 1) % BUF_SIZE];
      };
      int x1 = (int)j * 1000 / 2048;
      y1val = 120 * y1val;
      int y1 = (int)(240 + y1val);

      int x2 = (int)(j + 1) * 1000 / BUF_SIZE;
      y2val = 120 * y2val;
      int y2 = (int)(240 + y2val);
      SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
    SDL_RenderPresent(renderer);
    msleepd(1000 / 48000);
    read = (read + 1) % BUF_SIZE;
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}
