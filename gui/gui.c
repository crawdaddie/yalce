#include "gui.h"
#include "SDL2/SDL2_gfxPrimitives.h"

#include "clap_gui.h"
#include "common.h"
#include "edit_graph.h"
#include "slider_window.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

Window windows[MAX_WINDOWS];

int window_count = 0;

// Define thresholds for color changes
#define RMS_YELLOW_THRESHOLD 0.5f
#define RMS_RED_THRESHOLD 0.8f
#define DECAY_RATE 0.95f

float calculate_rms(double *data, int channel, int layout, int sample_count) {
  float sum = 0.0f;
  for (int i = 0; i < sample_count; i++) {
    float sample = (float)data[i * layout + channel];
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
  *peak = fmax(rms, *peak * DECAY_RATE);

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

// FFT implementation using Cooley-Tukey algorithm
void fft(double complex *buf, int n) {
  if (n <= 1)
    return;

  // Separate even and odd elements
  double complex even[n / 2], odd[n / 2];
  for (int i = 0; i < n / 2; i++) {
    even[i] = buf[2 * i];
    odd[i] = buf[2 * i + 1];
  }

  // Recursive FFT on even and odd arrays
  fft(even, n / 2);
  fft(odd, n / 2);

  // Combine results
  for (int k = 0; k < n / 2; k++) {
    double complex t = cexp(-2.0 * I * M_PI * k / n) * odd[k];
    buf[k] = even[k] + t;
    buf[k + n / 2] = even[k] - t;
  }
}

void __render_spectrum(Window *window) {
  _scope_win_data *win_data = window->data;

  // Set background
  SDL_SetRenderDrawColor(window->renderer, 224, 224, 224, 255);
  SDL_RenderClear(window->renderer);

  int layout = win_data->layout;
  int size = win_data->size;
  double *buf = win_data->buf;

  int channel_height = window->height / layout;
  int fft_size = 1024; // Should be power of 2

  double complex fft_buf[fft_size];
  // Process and render each channel
  for (int channel = 0; channel < layout; channel++) {
    // Prepare data for FFT

    // Copy and window the data (using Hanning window)
    for (int i = 0; i < fft_size; i++) {
      double window = 0.5 * (1 - cos(2 * M_PI * i / (fft_size - 1)));
      fft_buf[i] = buf[i * layout + channel] * window;
    }

    // Perform FFT
    fft(fft_buf, fft_size);

    // Draw spectrum
    SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);

    int num_bars = window->width / 3; // Width of each bar + 1px gap
    for (int i = 0; i < num_bars; i++) {
      // Calculate magnitude for this frequency bin
      double magnitude = 0;
      int bin_start = (int)((i * fft_size / 2) / (double)num_bars);
      int bin_end = (int)(((i + 1) * fft_size / 2) / (double)num_bars);

      for (int j = bin_start; j < bin_end; j++) {
        magnitude += cabs(fft_buf[j]) / (bin_end - bin_start);
      }
      // printf("magnitude %f\n", magnitude);

      // Scale magnitude logarithmically
      magnitude = log10(1 + magnitude) * channel_height * 0.4;

      // Draw bar
      SDL_Rect bar = {
          i * 3,                                           // x position
          (channel + 1) * channel_height - (int)magnitude, // y position
          2,                                               // width
          (int)magnitude                                   // height
      };
      SDL_RenderFillRect(window->renderer, &bar);
    }

    // // Draw RMS meter (keep this from original code)
    // float rms = calculate_rms(buf, channel, layout, size);
    // render_rms_meter(window->renderer, 0, channel * channel_height, 30,
    //                  channel_height, rms,
    //                  &win_data->rms_channel_peaks[channel]);
  }

  SDL_RenderPresent(window->renderer);
}
void render_spectrum(Window *window) {
    _scope_win_data *win_data = window->data;

    // Set background
    SDL_SetRenderDrawColor(window->renderer, 224, 224, 224, 255);
    SDL_RenderClear(window->renderer);

    int layout = win_data->layout;
    int size = win_data->size;
    double *buf = win_data->buf;

    int channel_height = window->height / layout;
    int fft_size = 1024; // Should be power of 2

    double complex fft_buf[fft_size];
    // Process and render each channel
    for (int channel = 0; channel < layout; channel++) {
        // Clear FFT buffer
        memset(fft_buf, 0, sizeof(fft_buf));

        // Copy and window the data (using Hanning window)
        for (int i = 0; i < fft_size && i < size; i++) {
            double window = 0.5 * (1 - cos(2 * M_PI * i / (fft_size - 1)));
            fft_buf[i] = buf[i * layout + channel] * window;
        }

        // Perform FFT
        fft(fft_buf, fft_size);

        // Draw spectrum
        SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);

        int num_bars = window->width / 3; // Width of each bar + 1px gap
        
        // Only use first half of FFT result (up to Nyquist frequency)
        int usable_bins = fft_size / 2;
        
        for (int i = 0; i < num_bars; i++) {
            // Calculate magnitude for this frequency bin
            double magnitude = 0;
            int bin_start = (int)((i * usable_bins) / (double)num_bars);
            int bin_end = (int)(((i + 1) * usable_bins) / (double)num_bars);
            
            // Ensure we don't exceed usable bins
            bin_end = (bin_end < usable_bins) ? bin_end : usable_bins - 1;
            
            if (bin_start < bin_end) {
                for (int j = bin_start; j < bin_end; j++) {
                    // Calculate magnitude and normalize by FFT size
                    double bin_magnitude = cabs(fft_buf[j]) / (fft_size / 2.0);
                    magnitude += bin_magnitude / (bin_end - bin_start);
                }

                // Apply frequency weighting (optional)
                // This helps emphasize mid-range frequencies
                // double freq = (bin_start + bin_end) / 2.0 / (double)fft_size;
                // magnitude *= sqrt(freq);

                // Scale the magnitude logarithmically
                magnitude = 20 * log10(1 + magnitude);
                
                // Normalize to channel height
                double normalized_height = (magnitude + 60) / 60.0;  // Adjust these values based on your input range
                normalized_height = fmax(0.0, fmin(1.0, normalized_height));
                int bar_height = (int)(normalized_height * channel_height * 0.9);  // 0.9 to leave some margin

                // Draw bar
                SDL_Rect bar = {
                    i * 3,                                           // x position
                    (channel + 1) * channel_height - bar_height,    // y position
                    2,                                              // width
                    bar_height                                      // height
                };
                SDL_RenderFillRect(window->renderer, &bar);
            }
        }
    }

    SDL_RenderPresent(window->renderer);
}

void render_oscilloscope(Window *window) {
  _scope_win_data *win_data = window->data;

  // Set the background color to RGB(224, 224, 224)
  SDL_SetRenderDrawColor(window->renderer, 224, 224, 224, 255);
  SDL_RenderClear(window->renderer);

  int layout = win_data->layout;
  int size = win_data->size;
  double *buf = win_data->buf;

  int channel_height = window->height / layout;

  // Calculate RMS values and render RMS meters for each channel
  int meter_width = 30;
  for (int channel = 0; channel < layout; channel++) {
    float rms = calculate_rms(buf, channel, layout, size);
    render_rms_meter(window->renderer, 0, channel * channel_height, meter_width,
                     channel_height, rms,
                     &win_data->rms_channel_peaks[channel]);
  }

  // Draw oscilloscope for each channel
  for (int channel = 0; channel < layout; channel++) {
    SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);
    for (int i = 0; i < size - 1; i++) {
      int x1 = i * window->width / size;
      int x2 = (i + 1) * window->width / size;
      int y1 = (channel + 0.5) * channel_height +
               (int)(buf[i * layout + channel] * channel_height * 0.5);
      int y2 = (channel + 0.5) * channel_height +
               (int)(buf[(i + 1) * layout + channel] * channel_height * 0.5);

      SDL_RenderDrawLine(window->renderer, x1, y1, x2, y2);
    }
  }

  SDL_RenderPresent(window->renderer);
}

// Window event handler implementation
void scope_event_handler(Window *window, SDL_Event *event) {
  _scope_win_data *win_data = (_scope_win_data *)window->data;

  switch (event->type) {
  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    case SDLK_f: {
      win_data->render_spectrum = !win_data->render_spectrum;
      if (win_data->render_spectrum) {
        window->render_fn = render_spectrum;
      } else {

        window->render_fn = render_oscilloscope;
      }
      break;
    }
    }
    break;
  }
}
typedef struct _array {
  int32_t size;
  double *data;
} _array;

#define LEFT_MARGIN 20
#define RIGHT_MARGIN 20
#define VERTICAL_PADDING 20

void plot_array_window(Window *window) {
  _array_plot_win_data *array_data = window->data;

  SDL_Renderer *renderer = window->renderer;
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  double *data = array_data->data_ptr;
  int length = array_data->_size;
  double _min_value = array_data->min;
  double _max_value = array_data->max;

  // Calculate the actual plotting area
  int plot_width = window->width - LEFT_MARGIN - RIGHT_MARGIN;
  int plot_height = window->height - 2 * VERTICAL_PADDING;

  // Draw graph
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  for (int i = 0; i < length - 1; i++) {
    int x1 = LEFT_MARGIN + i * plot_width / (length - 1);
    int y1 =
        window->height - VERTICAL_PADDING -
        (int)((data[i] - _min_value) / (_max_value - _min_value) * plot_height);

    int x2 = LEFT_MARGIN + (i + 1) * plot_width / (length - 1);
    int y2 = window->height - VERTICAL_PADDING -
             (int)((data[i + 1] - _min_value) / (_max_value - _min_value) *
                   plot_height);

    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    aalineRGBA(renderer, x1, y1, x2, y2, 0, 0, 0, 255);
  }

  SDL_RenderPresent(renderer);
}

Uint32 CREATE_WINDOW_EVENT;
bool create_window(WindowType type, void *data) {
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }
  Window *new_window = &windows[window_count];

  new_window->type = type;
  new_window->width = WINDOW_WIDTH;
  new_window->height = WINDOW_HEIGHT;
  new_window->font = DEFAULT_FONT;
  window_count++;

  if (type == WINDOW_TYPE_CLAP_NATIVE) {
    return init_clap_ui_window(new_window, data);
  }

  const char *wname = "New Window";

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

  case WINDOW_TYPE_PLOT_ARRAY: {
    wname = "Array Plot";
    break;
  }
  }
  new_window->window = SDL_CreateWindow(
      wname, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      new_window->width, new_window->height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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
    new_window->render_fn = render_oscilloscope;
    new_window->handle_event = scope_event_handler;
    new_window->data = data;
    break;
  }

  case WINDOW_TYPE_SLIDER: {
    new_window->render_fn = draw_slider_window;
    new_window->handle_event = handle_slider_window_events;
    new_window->data = data;
    break;
  }

  case WINDOW_TYPE_PLOT_ARRAY: {
    new_window->render_fn = plot_array_window;
    new_window->data = data;
    break;
  }
  }
  return true;
}

typedef struct {
  WindowType type;
  void *data;
} WindowCreationData;

int create_clap_node_slider_window(void *data) {
  push_create_window_event(WINDOW_TYPE_CLAP_SLIDER, data);
  return 1;
}
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

    case SDL_KEYDOWN:
    case SDL_KEYUP: {
      SDL_Window *focused_window = SDL_GetKeyboardFocus();
      if (focused_window != NULL) {
        for (int i = 0; i < window_count; i++) {
          if (windows[i].window == focused_window) {
            if (windows[i].handle_event != NULL) {
              windows[i].handle_event(&windows[i], &event);
            }
            break;
          }
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

int create_scope(double *output, int layout, int size) {
  _scope_win_data *win_data = malloc(sizeof(_scope_win_data));
  win_data->buf = output;
  win_data->rms_channel_peaks = calloc(layout, sizeof(double));
  win_data->layout = layout;
  win_data->size = size;

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

int _create_plot_array_window(int32_t size, double *data_ptr) {

  _array_plot_win_data *win_data = malloc(sizeof(_array_plot_win_data));
  win_data->_size = size;
  win_data->data_ptr = data_ptr;

  double min = 100.;
  double max = -100.;

  for (int i = 0; i < size; i++) {
    double val = data_ptr[i];
    if (val <= min) {
      min = val;
    }
    if (val >= max) {
      max = val;
    }
  }
  win_data->min = min;
  win_data->max = max;
  push_create_window_event(WINDOW_TYPE_PLOT_ARRAY, win_data);
  return 1;
}
