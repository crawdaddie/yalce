// clang -o dynamic_windows gui_example.c \
//   -I$(brew --prefix sdl2)/include/SDL2 \    
//   -I$(brew --prefix sdl2_ttf)/include/SDL2 \
//   -L$(brew --prefix sdl2)/lib \    
//   -L$(brew --prefix sdl2_ttf)/lib \
//   -lSDL2 -lSDL2_ttf -lm

#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600
#define ARRAY_SIZE 100
#define POINT_RADIUS 2
#define AXIS_PADDING 40
#define LABEL_PADDING 5
#define NUM_Y_LABELS 5

float data[ARRAY_SIZE];
int window_width = INITIAL_WIDTH;
int window_height = INITIAL_HEIGHT;
int last_x = -1, last_y = -1;
float min_value = 0.0f, max_value = 1.0f;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

int get_closest_point(int x);

void init_data() {
  for (int i = 0; i < ARRAY_SIZE; i++) {
    data[i] = sin(i * 0.1) * 0.5 + 0.5;
  }
}

bool init_sdl() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
    printf("Initialization failed: %s\n", SDL_GetError());
    return false;
  }

  window =
      SDL_CreateWindow("Interactive Graph Editor", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, INITIAL_WIDTH, INITIAL_HEIGHT,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return false;
  }

  font = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 12);
  if (!font) {
    printf("Font loading failed: %s\n", TTF_GetError());
    return false;
  }

  return true;
}


void draw_graph() {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  // Draw y-axis labels
  SDL_Color black = {0, 0, 0, 255};
  for (int i = 0; i <= NUM_Y_LABELS; i++) {
    float value = max_value - i * (max_value - min_value) / NUM_Y_LABELS;
    int y =
        AXIS_PADDING + i * (window_height - 2 * AXIS_PADDING) / NUM_Y_LABELS;
    char label[20];
    snprintf(label, sizeof(label), "%.2f", value);
    render_text(label, LABEL_PADDING, y - 7, black);
  }

  // Draw graph
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  for (int i = 0; i < ARRAY_SIZE - 1; i++) {
    int x1 =
        AXIS_PADDING + i * (window_width - 2 * AXIS_PADDING) / (ARRAY_SIZE - 1);
    int y1 = window_height - AXIS_PADDING -
             (int)((data[i] - min_value) / (max_value - min_value) *
                   (window_height - 2 * AXIS_PADDING));
    int x2 = AXIS_PADDING +
             (i + 1) * (window_width - 2 * AXIS_PADDING) / (ARRAY_SIZE - 1);
    int y2 = window_height - AXIS_PADDING -
             (int)((data[i + 1] - min_value) / (max_value - min_value) *
                   (window_height - 2 * AXIS_PADDING));
    // SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    aalineRGBA(renderer, x1, y1, x2, y2, 0, 0, 0, 255);
  }

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  for (int i = 0; i < ARRAY_SIZE; i++) {
    int x =
        AXIS_PADDING + i * (window_width - 2 * AXIS_PADDING) / (ARRAY_SIZE - 1);
    int y = window_height - AXIS_PADDING -
            (int)((data[i] - min_value) / (max_value - min_value) *
                  (window_height - 2 * AXIS_PADDING));
    SDL_Rect rect = {x - POINT_RADIUS, y - POINT_RADIUS, POINT_RADIUS * 2,
                     POINT_RADIUS * 2};
    SDL_RenderFillRect(renderer, &rect);
  }

  // Draw cursor value if editing
  if (last_x != -1 && last_y != -1) {
    int index = get_closest_point(last_x);
    char value_str[20];
    snprintf(value_str, sizeof(value_str), "%.2f", data[index]);
    render_text(value_str, last_x + 10, last_y - 20, black);
  }

  SDL_RenderPresent(renderer);
}

int get_closest_point(int x) {
  return (x - AXIS_PADDING) * (ARRAY_SIZE - 1) /
         (window_width - 2 * AXIS_PADDING);
}

void update_data_point(int x, int y) {
  int index = get_closest_point(x);
  if (index >= 0 && index < ARRAY_SIZE) {
    float new_value = max_value - (y - AXIS_PADDING) * (max_value - min_value) /
                                      (window_height - 2 * AXIS_PADDING);
    data[index] = fmaxf(min_value, fminf(max_value, new_value));
    // printf("[ ");
    // for (int i = 0; i < ARRAY_SIZE; i++) {
    //   printf("%f,", data[i]);
    // }
    // printf("]\n");
  }
}

void interpolate_line(int x1, int y1, int x2, int y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    update_data_point(x1, y1);

    if (x1 == x2 && y1 == y2)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        window_width = event.window.data1;
        window_height = event.window.data2;
        SDL_RenderSetLogicalSize(renderer, window_width, window_height);
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT) {
        last_x = event.button.x;
        last_y = event.button.y;
        update_data_point(last_x, last_y);
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        last_x = -1;
        last_y = -1;
      }
      break;
    case SDL_MOUSEMOTION:
      if (event.motion.state & SDL_BUTTON_LMASK) {
        int current_x = event.motion.x;
        int current_y = event.motion.y;
        if (last_x != -1 && last_y != -1) {
          interpolate_line(last_x, last_y, current_x, current_y);
        }
        last_x = current_x;
        last_y = current_y;
      }
      break;
    }
  }
}

int main() {
  init_data();

  if (!init_sdl()) {
    return 1;
  }

  while (true) {
    handle_events();
    draw_graph();
    SDL_Delay(16);
  }

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();

  return 0;
}
