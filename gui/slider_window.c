#include "./slider_window.h"

#define SLIDER_COUNT 4
#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 20
#define SLIDER_PADDING 40

// Add this to your existing WindowType enum
// enum WindowType {
//     // ... existing types ...
//     WINDOW_TYPE_SLIDER
// };
//

void draw_slider_window(Window *window) {
  SDL_Renderer *renderer = window->renderer;
  _slider_window_data *data = (_slider_window_data *)window->data;

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  SDL_Color text_color = {0, 0, 0, 255};

  for (int i = 0; i < data->slider_count; i++) {
    int y = SLIDER_PADDING + i * (SLIDER_HEIGHT + SLIDER_PADDING);

    // Draw slider background
    SDL_Rect bg_rect = {(window->width - SLIDER_WIDTH) / 2, y, SLIDER_WIDTH,
                        SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &bg_rect);

    // Draw slider handle
    int handle_x = (window->width - SLIDER_WIDTH) / 2 +
                   (int)(data->values[i] * SLIDER_WIDTH);
    SDL_Rect handle_rect = {handle_x - 5, y - 5, 10, SLIDER_HEIGHT + 10};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &handle_rect);

    // Draw slider label
    render_text(data->labels[i], 10, y, text_color, renderer, window->font);

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

  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      int mouse_y = event->button.y;
      for (int i = 0; i < SLIDER_COUNT; i++) {
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
      int slider_x = (window->width - SLIDER_WIDTH) / 2;
      float new_value = (float)(mouse_x - slider_x) / SLIDER_WIDTH;
      new_value = fmaxf(0.0f, fminf(1.0f, new_value));
      data->values[data->active_slider] = new_value;
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
