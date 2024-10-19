#include "gui.h"
#include "common.h"
#include "edit_graph.h"
#include "slider_window.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

Window windows[MAX_WINDOWS];

int window_count = 0;

#define SAMPLE_COUNT 512
#define DECAY_RATE 0.95f
float calculate_rms(double *data, int channel, int sample_count) {
  float sum = 0.0f;
  for (int i = 0; i < sample_count; i++) {
    float sample = (float)data[i * 2 + channel];
    sum += sample * sample;
  }
  return sqrtf(sum / sample_count);
}

// Define thresholds for color changes
#define RMS_YELLOW_THRESHOLD 0.5f
#define RMS_RED_THRESHOLD 0.8f

void set_color_by_level(SDL_Renderer *renderer, double level) {
  if (level >= RMS_RED_THRESHOLD) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Red
  } else if (level >= RMS_YELLOW_THRESHOLD) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100); // Yellow
  } else {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100); // Green
  }
}

void render_rms_meter(SDL_Renderer *renderer, int x, int y, int width,
                      int height, double rms, double *peak) {
  // Update peak value with decay
  *peak = fmaxf(rms, *peak * DECAY_RATE);

  // Draw meter background
  SDL_Rect meter_bg = {x, y, width, height};
  SDL_SetRenderDrawColor(renderer, 0, 100, 0, 100);
  SDL_RenderFillRect(renderer, &meter_bg);

  // Draw RMS level with color based on intensity
  int level_height = (int)(rms * height);
  SDL_Rect level_rect = {x, y + height - level_height, width, level_height};
  set_color_by_level(renderer, rms);
  SDL_RenderFillRect(renderer, &level_rect);

  // Draw peak indicator
  int peak_y = y + height - (int)(*peak * height);
  set_color_by_level(renderer, *peak);
  SDL_RenderDrawLine(renderer, x, peak_y, x + width, peak_y);
}

void render_oscilloscope(Window *window) {
  _scope_win_data *win_data = window->data;

  // Set the background color to RGB(224, 224, 224)
  SDL_SetRenderDrawColor(window->renderer, 224, 224, 224, 255);
  SDL_RenderClear(window->renderer);

  int half_height = window->height / 2;
  int quarter_height = window->height / 4;

  double *buf = (double *)win_data->stereo_buf;

  // Calculate RMS values
  float rms_left = calculate_rms(buf, 0, SAMPLE_COUNT);
  float rms_right = calculate_rms(buf, 1, SAMPLE_COUNT);

  // Render RMS meters
  int meter_width = 30;
  render_rms_meter(window->renderer, 0, 0, meter_width, half_height, rms_left,
                   &win_data->rms_peak_left);
  render_rms_meter(window->renderer, 0, half_height, meter_width, half_height,
                   rms_right, &win_data->rms_peak_right);

  // Draw left channel (black) in the top half
  SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);
  for (int i = 0; i < SAMPLE_COUNT - 1; i++) {
    int x1 = i * window->width / SAMPLE_COUNT;
    int x2 = (i + 1) * window->width / SAMPLE_COUNT;
    int y1 = quarter_height + (int)(buf[i * 2] * quarter_height);
    int y2 = quarter_height + (int)(buf[(i + 1) * 2] * quarter_height);

    SDL_RenderDrawLine(window->renderer, x1, y1, x2, y2);
  }

  // Draw right channel (black) in the bottom half
  SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);
  for (int i = 0; i < SAMPLE_COUNT - 1; i++) {
    int x1 = i * window->width / SAMPLE_COUNT;
    int x2 = (i + 1) * window->width / SAMPLE_COUNT;
    int y1 = 3 * quarter_height + (int)(buf[i * 2 + 1] * quarter_height);
    int y2 = 3 * quarter_height + (int)(buf[(i + 1) * 2 + 1] * quarter_height);

    SDL_RenderDrawLine(window->renderer, x1, y1, x2, y2);
  }

  SDL_RenderPresent(window->renderer);
}

typedef struct _array {
  int32_t size;
  double *data;
} _array;

Uint32 CREATE_WINDOW_EVENT;
bool create_window(WindowType type, void *data) {

  printf("new window %p", data);
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }

  Window *new_window = &windows[window_count];
  new_window->type = type;
  new_window->width = WINDOW_WIDTH;
  new_window->height = WINDOW_HEIGHT;

  const char *wname;

  switch (type) {
  case WINDOW_TYPE_ARRAY_EDITOR: {
    wname = "Array Editor";
    break;
  }

  case WINDOW_TYPE_OSCILLOSCOPE: {
    wname = "Scope";
    break;
  }

  case WINDOW_TYPE_SLIDER: {
    wname = "Edit Values";
    break;
  }
  }
  new_window->window =
      SDL_CreateWindow(wname, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       new_window->width, new_window->height,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!new_window->window) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  new_window->renderer =
      SDL_CreateRenderer(new_window->window, -1, SDL_RENDERER_ACCELERATED);

  if (!new_window->renderer) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(new_window->window);
    return false;
  }

  new_window->font = DEFAULT_FONT;

  switch (type) {
  case WINDOW_TYPE_ARRAY_EDITOR: {
    // _array *arr = data;
    // printf("create array editor window for array %d\n", arr->size);
    printf("edit graph window\n");
    new_window->data = data;
    new_window->render_fn = draw_graph;
    new_window->handle_event = handle_array_editor_events;
    break;
  }

  case WINDOW_TYPE_OSCILLOSCOPE: {
    double *buf = data;
    printf("create oscilloscope window %p\n", buf);
    new_window->render_fn = render_oscilloscope;
    new_window->data = data;
    break;
  }

  case WINDOW_TYPE_SLIDER: {
    new_window->render_fn = draw_slider_window;
    new_window->handle_event = handle_slider_window_events;
    new_window->data = data;
    break;
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

void _handle_events() {
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
            window_count--;
            break;
          }
        }
      } else {
        for (int i = 0; i < window_count; i++) {
          if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
            if (windows[i].handle_event != NULL) {
              windows[i].handle_event(&windows[i], &event);
            }
            break;
          }
        }
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION: {
      SDL_Window *mouse_window = SDL_GetWindowFromID(event.window.windowID);
      for (int i = 0; i < window_count; i++) {
        if (windows[i].window == mouse_window) {
          if (windows[i].handle_event != NULL) {
            windows[i].handle_event(&windows[i], &event);
          }
          break;
        }
      }
    } break;

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
void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_WINDOWEVENT:
      for (int i = 0; i < window_count; i++) {
        if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
          switch (event.window.event) {
          case SDL_WINDOWEVENT_CLOSE:
            SDL_DestroyRenderer(windows[i].renderer);
            SDL_DestroyWindow(windows[i].window);
            for (int j = i; j < window_count - 1; j++) {
              windows[j] = windows[j + 1];
            }
            window_count--;
            break;

          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED:
            windows[i].width = event.window.data1;
            windows[i].height = event.window.data2;
            // Optionally, update logical size if you're using it
            SDL_RenderSetLogicalSize(windows[i].renderer, windows[i].width,
                                     windows[i].height);
            break;

          default:
            if (windows[i].handle_event != NULL) {
              windows[i].handle_event(&windows[i], &event);
            }
            break;
          }
          break; // Break the for loop, we've found our window
        }
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION: {
      SDL_Window *mouse_window = SDL_GetWindowFromID(event.window.windowID);
      for (int i = 0; i < window_count; i++) {
        if (windows[i].window == mouse_window) {
          if (windows[i].handle_event != NULL) {
            windows[i].handle_event(&windows[i], &event);
          }
          break;
        }
      }
    } break;

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

int init_gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  // In your initialization function
  if (TTF_Init() == -1) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    // Handle error appropriately
    return 1;
  }
  DEFAULT_FONT = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 12);
  if (!DEFAULT_FONT) {
    fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    // Handle error appropriately
    return 1;
  }

  // Register custom event
  CREATE_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_WINDOW_EVENT == (Uint32)-1) {
    printf("Failed to register custom event\n");
    return 1;
  }

  return 0;
}

int gui_loop() {
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

int _create_scope(double *output) {
  _scope_win_data *win_data = malloc(sizeof(_scope_win_data));
  win_data->stereo_buf = output;
  win_data->rms_peak_left = 0.0f;
  win_data->rms_peak_right = 0.0f;

  push_create_window_event(WINDOW_TYPE_OSCILLOSCOPE, win_data);
  return 1;
}

int create_array_editor(int32_t size, double *data_ptr) {

  _array_edit_win_data *win_data = malloc(sizeof(_array_edit_win_data));
  win_data->_size = size;
  win_data->data_ptr = data_ptr;

  push_create_window_event(WINDOW_TYPE_ARRAY_EDITOR, win_data);
  return 1;
}

int create_slider_window(int32_t size, double *data_ptr,
                         struct _String *_labels,
                         void (*on_update)(int, double)) {

  _slider_window_data *data = malloc(sizeof(_slider_window_data));
  data->slider_count = size;
  data->values = data_ptr;
  data->labels = malloc(sizeof(char *) * size);
  data->active_slider = -1;

  for (int i = 0; i < size; i++) {
    // printf("_labels[%d]: %s \n", i, _labels[i].chars);
    data->labels[i] = _labels[i].chars;
  }

  data->mins = malloc(sizeof(double) * size);
  for (int i = 0; i < size; i++) {
    data->mins[i] = 0.0;
  }
  data->maxes = malloc(sizeof(double) * size);
  for (int i = 0; i < size; i++) {
    data->maxes[i] = 1.0;
  }
  data->on_update = on_update;

  push_create_window_event(WINDOW_TYPE_SLIDER, data);

  return 1;
}
