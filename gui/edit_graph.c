#include "edit_graph.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_surface.h"
#include "SDL2/SDL_ttf.h"
#include "common.h"
#include <stdbool.h>

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600
#define size 100
#define POINT_RADIUS 2
#define AXIS_PADDING 40
#define LABEL_PADDING 5
#define NUM_Y_LABELS 5

int window_width = INITIAL_WIDTH;
int window_height = INITIAL_HEIGHT;
int last_x = -1, last_y = -1;
float min_value = 0.0f, max_value = 1.0f;

int get_closest_point(int x) {
  return (x - AXIS_PADDING) * (size - 1) / (window_width - 2 * AXIS_PADDING);
}

void render_text(const char *text, int x, int y, SDL_Color color,
                 SDL_Renderer *renderer, TTF_Font *font) {
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect = {x, y, surface->w, surface->h};
  SDL_RenderCopy(renderer, texture, NULL, &rect);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

struct ArrayData {
  int length;
  double *data;
};

void draw_graph(Window *window) {
  SDL_Renderer *renderer = window->renderer;
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  struct ArrayData *arr_data = window->data;

  double *data = arr_data->data;
  int length = arr_data->length;

  // Draw y-axis labels
  SDL_Color black = {0, 0, 0, 255};
  for (int i = 0; i <= NUM_Y_LABELS; i++) {
    double value = max_value - i * (max_value - min_value) / NUM_Y_LABELS;
    int y =
        AXIS_PADDING + i * (window_height - 2 * AXIS_PADDING) / NUM_Y_LABELS;
    char label[20];
    snprintf(label, sizeof(label), "%.2f", value);
    render_text(label, LABEL_PADDING, y - 7, black, renderer, DEFAULT_FONT);
  }

  // Draw graph
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  for (int i = 0; i < length - 1; i++) {
    int x1 =
        AXIS_PADDING + i * (window_width - 2 * AXIS_PADDING) / (length - 1);
    int y1 = window_height - AXIS_PADDING -
             (int)((data[i] - min_value) / (max_value - min_value) *
                   (window_height - 2 * AXIS_PADDING));
    int x2 = AXIS_PADDING +
             (i + 1) * (window_width - 2 * AXIS_PADDING) / (length - 1);
    int y2 = window_height - AXIS_PADDING -
             (int)((data[i + 1] - min_value) / (max_value - min_value) *
                   (window_height - 2 * AXIS_PADDING));
    // SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    aalineRGBA(renderer, x1, y1, x2, y2, 0, 0, 0, 255);
  }

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  for (int i = 0; i < length; i++) {
    int x = AXIS_PADDING + i * (window_width - 2 * AXIS_PADDING) / (length - 1);
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
    render_text(value_str, last_x + 10, last_y - 20, black, renderer,
                DEFAULT_FONT);
  }

  SDL_RenderPresent(renderer);
}

void update_data_point(Window *window, int x, int y) {

  struct ArrayData *arr_data = window->data;

  double *data = arr_data->data;
  int length = arr_data->length;

  int index = get_closest_point(x);
  if (index >= 0 && index < length) {
    float new_value = max_value - (y - AXIS_PADDING) * (max_value - min_value) /
                                      (window_height - 2 * AXIS_PADDING);
    data[index] = fmaxf(min_value, fminf(max_value, new_value));
  }
}
void interpolate_line(Window *window, int x1, int y1, int x2, int y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    update_data_point(window, x1, y1);

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

void handle_array_editor_events(Window *window, SDL_Event *event) {
  switch (event->type) {
  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
      window_width = event->window.data1;
      window_height = event->window.data2;
      SDL_RenderSetLogicalSize(window->renderer, window_width, window_height);
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      last_x = event->button.x;
      last_y = event->button.y;
      update_data_point(window, last_x, last_y);
    }
    break;
  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT) {
      last_x = -1;
      last_y = -1;
    }
    break;
  case SDL_MOUSEMOTION:
    if (event->motion.state & SDL_BUTTON_LMASK) {
      int current_x = event->motion.x;
      int current_y = event->motion.y;
      if (last_x != -1 && last_y != -1) {
        interpolate_line(window, last_x, last_y, current_x, current_y);
      }
      last_x = current_x;
      last_y = current_y;
    }
    break;
  }
}
