#include "clap_gui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <clap/ext/gui.h>

//
int push_create_window_event(WindowType type, void *data);

bool set_param_idx_with_event(void *_state, int idx, double value);

clap_plugin_specs *get_specs(void *state);

static void draw_slider_window(Window *window);
static void handle_slider_window_events(Window *window, SDL_Event *event);

double unipolar_scale(double min, double max, double unipolar_input);

// called from lang thread
int clap_ui(void *state_ptr) {
  printf("create clap ui %p\n", state_ptr);
  clap_ui_window_t *data = malloc(sizeof(clap_ui_window_t));
  data->target = state_ptr;
  clap_plugin_specs *specs = get_specs(data->target);
  data->specs = specs;
  int num = specs->num_params;
  data->unit_vals = malloc(sizeof(double) * num);

  for (int i = 0; i < num; i++) {
    data->unit_vals[i] = (specs->param_vals[i] - specs->min_vals[i])/(specs->max_vals[i] - specs->min_vals[i]);
  }

  push_create_window_event(WINDOW_TYPE_CLAP_NATIVE, data);
}

void *init_clap_ui_window(Window *window, clap_ui_window_t *data) {

  // printf("create window for clap ui %s\n", data->specs->name);
  // for (int i = 0; i < data->specs->num_params; i++) {
  //   printf("'%s'\n", data->specs->labels[i]);
  // }
  //
  //
  window->window = SDL_CreateWindow(
      data->specs->name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      window->width, window->height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window->window) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    return NULL;
  }

  window->renderer =
      SDL_CreateRenderer(window->window, -1, SDL_RENDERER_ACCELERATED);

  if (!window->renderer) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window->window);
    return NULL;
  }
  window->data = data;
  window->render_fn = draw_slider_window;
  window->handle_event = handle_slider_window_events;
}

#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 20
#define SLIDER_PADDING 20
#define SLIDER_L_START 10
static inline int min(int a, int b) { return a > b ? b : a; }

static void draw_slider_window(Window *window) {
  SDL_Renderer *renderer = window->renderer;
  clap_ui_window_t *data = window->data;

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  int slider_count = data->specs->num_params;

  SDL_Color text_color = {0, 0, 0, 255};
  int slider_width = min(SLIDER_WIDTH, window->width - 20);
  for (int i = 0; i < slider_count; i++) {
    int y = SLIDER_PADDING + i * (SLIDER_HEIGHT + SLIDER_PADDING);

    // Draw slider background
    SDL_Rect bg_rect = {SLIDER_L_START, y, slider_width, SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &bg_rect);

    // Draw slider label
    render_text(data->specs->labels[i], SLIDER_L_START, y, text_color, renderer,
                window->font);
    // Draw slider handle
    int handle_x =
        SLIDER_L_START + (int)(data->unit_vals[i] * slider_width);
    SDL_Rect handle_rect = {handle_x, y, 1, SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 255, 0, 144, 255);
    SDL_RenderFillRect(renderer, &handle_rect);

    // Draw slider value
    char value_text[10];
    snprintf(value_text, sizeof(value_text), "%.2f",
             data->specs->param_vals[i]);
    render_text(value_text, window->width - 50, y, text_color, renderer,
                window->font);
  }

  SDL_RenderPresent(renderer);
}

static void handle_slider_window_events(Window *window, SDL_Event *event) {
  clap_ui_window_t *data = window->data;

  int slider_count = data->specs->num_params;
  int slider_width = min(SLIDER_WIDTH, window->width - 20);

  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      int mouse_y = event->button.y;
      for (int i = 0; i < slider_count; i++) {
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
      int idx = data->active_slider;
      int mouse_x = event->motion.x;
      int slider_x = SLIDER_L_START;
      double new_value = (double)(mouse_x - slider_x) / slider_width;
      new_value = fmax(0.0f, fmin(1.0f, new_value));
      data->unit_vals[idx] = new_value;
      new_value = unipolar_scale(data->specs->min_vals[idx],
                                 data->specs->max_vals[idx], new_value);
      data->specs->param_vals[idx] = new_value;
      set_param_idx_with_event(data->target, idx, new_value);
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
