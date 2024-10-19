#include "./slider_window.h"

#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 20
#define SLIDER_PADDING 20

// #define SLIDER_L_START (window->width - SLIDER_WIDTH) / 4
#define SLIDER_L_START 10
//
static inline int min(int a, int b) { return a > b ? b : a; }

void draw_slider_window(Window *window) {
  SDL_Renderer *renderer = window->renderer;
  _slider_window_data *data = (_slider_window_data *)window->data;

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  SDL_Color text_color = {0, 0, 0, 255};
  int slider_width = min(SLIDER_WIDTH, window->width - 20);
  for (int i = 0; i < data->slider_count; i++) {
    int y = SLIDER_PADDING + i * (SLIDER_HEIGHT + SLIDER_PADDING);

    // Draw slider background
    SDL_Rect bg_rect = {SLIDER_L_START, y, slider_width, SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &bg_rect);

    // Draw slider label
    render_text(data->labels[i], SLIDER_L_START, y, text_color, renderer,
                window->font);
    // Draw slider handle
    int handle_x = SLIDER_L_START + (int)(data->values[i] * slider_width);
    SDL_Rect handle_rect = {handle_x, y, 1, SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 255, 0, 144, 255);
    SDL_RenderFillRect(renderer, &handle_rect);

    // Draw slider value
    char value_text[10];
    snprintf(value_text, sizeof(value_text), "%.2f", data->values[i]);
    render_text(value_text, window->width - 50, y, text_color, renderer,
                window->font);
  }

  SDL_RenderPresent(renderer);
}

void handle_slider_window_events(Window *window, SDL_Event *event) {
  _slider_window_data *data = (_slider_window_data *)window->data;

  int slider_width = min(SLIDER_WIDTH, window->width - 20);

  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      int mouse_y = event->button.y;
      for (int i = 0; i < data->slider_count; i++) {
        int slider_y = SLIDER_PADDING + i * (SLIDER_HEIGHT + SLIDER_PADDING);
        if (mouse_y >= slider_y && mouse_y < slider_y + SLIDER_HEIGHT) {
          data->active_slider = i;
          break;
        }
      }
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT) {
      data->active_slider = -1;
    }
    break;

  case SDL_MOUSEMOTION:
    if (data->active_slider != -1) {
      int mouse_x = event->motion.x;
      int slider_x = SLIDER_L_START;
      float new_value = (float)(mouse_x - slider_x) / slider_width;
      new_value = fmaxf(0.0f, fminf(1.0f, new_value));
      data->values[data->active_slider] = new_value;
      if (data->on_update) {
        data->on_update(data->active_slider, new_value);
      }
    }
    break;

  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
      window->width = event->window.data1;
      window->height = event->window.data2;
    }
    break;
  }
}
