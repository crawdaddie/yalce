#include "gui.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL_syswm.h>
#include <VSTPlugin.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Add these includes at the top of your file
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <string.h>

#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

TTF_Font *DEFAULT_FONT;

typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(void *window, SDL_Renderer *renderer);
typedef bool (*GLWindowInitFn)(void *state);

typedef struct window_creation_data {
  void *handle_event;
  void *render_fn;
  void *data;
  GLWindowInitFn init_gl;
} window_creation_data;

bool _create_window(window_creation_data *data);

bool _create_opengl_window(window_creation_data *data);

typedef struct Window {
  SDL_Window *window;
  SDL_Renderer *renderer;
  void *data;
  int width;
  int height;
  EventHandler handle_event; // Function pointer for event handling
  WindowRenderFn render_fn;
  int num_children;
  struct Window *children;
} Window;

Window windows[MAX_WINDOWS];

int window_count = 0;

struct _String {
  int32_t length;
  char *chars;
};

Uint32 CREATE_WINDOW_EVENT;
Uint32 CREATE_OPENGL_WINDOW_EVENT;

int init_gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  // Set OpenGL attributes BEFORE creating any windows
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  if (TTF_Init() == -1) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    return 1;
  }
  DEFAULT_FONT = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 16);
  if (!DEFAULT_FONT) {
    fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    return 1;
  }

  CREATE_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_WINDOW_EVENT == (Uint32)-1) {
    printf("Failed to register custom event\n");
    return 1;
  }
  // Register OpenGL window creation event
  CREATE_OPENGL_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_OPENGL_WINDOW_EVENT == (Uint32)-1) {
    printf("Failed to register OpenGL window event\n");
    return 1;
  }

  return 0;
}
Window *get_window(SDL_Event event) {

  for (int i = 0; i < window_count; i++) {
    if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
      return windows + i;
    }
  }
  return NULL;
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == CREATE_WINDOW_EVENT) {
      // handle window creation
      //
      _create_window(event.user.data1);
    } else if (event.type == CREATE_OPENGL_WINDOW_EVENT) {
      // handle OpenGL window creation
      _create_opengl_window(event.user.data1);
    } else {

      for (int i = 0; i < window_count; i++) {
        if (SDL_GetWindowID(windows[i].window) == event.window.windowID &&
            windows[i].handle_event) {
          if (event.type == SDL_WINDOWEVENT &&
              event.window.event == SDL_WINDOWEVENT_CLOSE) {
            // Check if this is an OpenGL window
            if (windows[i].renderer == NULL) {
              // Clean up OpenGL resources
              SDL_GL_DeleteContext(SDL_GL_GetCurrentContext());
            } else {
              SDL_DestroyRenderer(windows[i].renderer);
            }

            SDL_DestroyWindow(windows[i].window);
            free(windows[i].data);
            for (int j = i; j < window_count - 1; j++) {
              windows[j] = windows[j + 1];
            }
            window_count--;

          } else {
            windows[i].handle_event(windows[i].data, &event);
          }
        }
      }
    }
  }
}

int gui_loop() {
  while (true) {

    handle_events();

    for (int i = 0; i < window_count; i++) {

      if (windows[i].render_fn) {
        if (windows[i].renderer) {
          SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
          SDL_RenderClear(windows[i].renderer);

          windows[i].render_fn(windows[i].data, windows[i].renderer);
          SDL_RenderPresent(windows[i].renderer);
        } else {
          // OpenGL window
          SDL_Window *current_window = windows[i].window;
          SDL_GLContext current_context = SDL_GL_GetCurrentContext();

          // Make sure we have the right context current
          SDL_GL_MakeCurrent(current_window, current_context);

          // Call the render function (it will handle OpenGL directly)
          windows[i].render_fn(windows[i].data, NULL);

          // Swap buffers
          SDL_GL_SwapWindow(current_window);
        }
      }
    }

    SDL_Delay(16); // Cap at roughly 60 fps
  }

  return 0;
}

bool _create_window(window_creation_data *data) {
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }
  int win_idx = window_count;
  window_count++;

  windows[win_idx].width = WINDOW_WIDTH;
  windows[win_idx].height = WINDOW_HEIGHT;

  windows[win_idx].render_fn = data->render_fn;
  windows[win_idx].handle_event = data->handle_event;
  windows[win_idx].data = data->data;

  windows[win_idx].window =
      SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       windows[win_idx].width, windows[win_idx].height,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                           SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FOREIGN);

  windows[win_idx].renderer =
      SDL_CreateRenderer(windows[win_idx].window, -1, SDL_RENDERER_ACCELERATED);

  // free(data);
  return true;
}

// Function to push a create window event to the SDL event queue
int create_window(void *data, void *renderer, void *event_handler) {

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_WINDOW_EVENT;

  window_creation_data *cdata = malloc(sizeof(window_creation_data));
  cdata->render_fn = renderer;
  cdata->handle_event = event_handler;
  cdata->data = data;
  event.user.data1 = cdata;

  return SDL_PushEvent(&event);
}

SDL_Renderer *render_text(const char *text, int x, int y,
                          SDL_Renderer *renderer, SDL_Color text_color) {
  if (!text || *text == '\0') {
    return renderer;
  }

  SDL_Surface *surface = TTF_RenderText_Blended(DEFAULT_FONT, text, text_color);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect = {x, y, surface->w, surface->h};

  SDL_RenderCopy(renderer, texture, NULL, &rect);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  return renderer;
}

#define BUFFER_SIZE 8192 // Size of the ring buffer (in frames)
#define LINE_THICKNESS 2 // Thickness of oscilloscope line
#define MAX_CHANNELS 8   // Maximum number of supported channels

// Colors for the different channels
static const SDL_Color CHANNEL_COLORS[MAX_CHANNELS] = {
    {0, 255, 0, 255},    // Green
    {0, 127, 255, 255},  // Light blue
    {255, 0, 0, 255},    // Red
    {255, 255, 0, 255},  // Yellow
    {255, 0, 255, 255},  // Magenta
    {0, 255, 255, 255},  // Cyan
    {255, 165, 0, 255},  // Orange
    {255, 255, 255, 255} // White
};

typedef struct scope_state {
  double *buf; // Current input signal buffer (interleaved)
  int layout;  // Number of channels (1=mono, 2=stereo, etc.)
  int size;    // Size of the input buffer (in frames)

  // Ring buffer for the oscilloscope (interleaved format)
  double *ring_buffer;  // Interleaved ring buffer
  int ring_buffer_size; // Total size of ring buffer in frames
  int ring_buffer_pos;  // Current position in ring buffer (in frames)

  // Display settings
  double vertical_scale;   // Vertical scaling factor
  double horizontal_scale; // Horizontal scaling factor
  double trigger_level;    // Trigger level for stable display
  int trigger_channel;     // Which channel to use for triggering
  bool draw_grid;          // Whether to draw the grid
  int trigger_mode;        // 0=auto, 1=normal, 2=single

  // Last window size for resize handling
  int last_width;
  int last_height;
} scope_state;

// Forward declarations
void append_to_ring_buffer(scope_state *state);
bool find_trigger_point(scope_state *state, int *trigger_index);
void draw_grid(SDL_Renderer *renderer, int width, int height);

// Function to append new data to the ring buffer
void append_to_ring_buffer(scope_state *state) {
  if (!state || !state->buf || !state->ring_buffer)
    return;

  // Copy the current buffer to the ring buffer at current position
  // Both are in interleaved format [ch0_frame0, ch1_frame0, ..., chN_frame0,
  // ch0_frame1, ...]
  int samples_to_copy = state->size * state->layout;
  int start_index = state->ring_buffer_pos * state->layout;

  for (int i = 0; i < samples_to_copy; i++) {
    int ring_idx =
        (start_index + i) % (state->ring_buffer_size * state->layout);
    state->ring_buffer[ring_idx] = state->buf[i];
  }

  state->ring_buffer_pos =
      (state->ring_buffer_pos + state->size) % state->ring_buffer_size;
}

bool find_trigger_point(scope_state *state, int *trigger_index) {
  if (!state || !state->ring_buffer ||
      state->trigger_channel >= state->layout) {
    *trigger_index = 0;
    return false;
  }

  int channel = state->trigger_channel;
  if (channel < 0 || channel >= state->layout)
    channel = 0;

  for (int i = state->size; i < state->ring_buffer_size - 1; i++) {
    int frame_idx = (state->ring_buffer_pos - i + state->ring_buffer_size) %
                    state->ring_buffer_size;
    int next_frame_idx = (frame_idx + 1) % state->ring_buffer_size;

    double current = state->ring_buffer[frame_idx * state->layout + channel];
    double next = state->ring_buffer[next_frame_idx * state->layout + channel];

    if (current <= state->trigger_level && next > state->trigger_level) {
      *trigger_index = next_frame_idx;
      return true;
    }
  }

  // If in auto mode and no trigger found, just use the oldest data
  if (state->trigger_mode == 0) {
    *trigger_index = (state->ring_buffer_pos + 1) % state->ring_buffer_size;
    return true;
  }

  return false;
}

void draw_grid(SDL_Renderer *renderer, int width, int height) {
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

  for (int i = 0; i <= 8; i++) {
    int y = (height * i) / 8;
    SDL_RenderDrawLine(renderer, 0, y, width, y);
  }

  for (int i = 0; i <= 10; i++) {
    int x = (width * i) / 10;
    SDL_RenderDrawLine(renderer, x, 0, x, height);
  }

  SDL_SetRenderDrawColor(renderer, 75, 75, 75, 255);
  SDL_RenderDrawLine(renderer, 0, height / 2, width, height / 2);
  SDL_RenderDrawLine(renderer, width / 2, 0, width / 2, height);
}

SDL_Renderer *scope_renderer(scope_state *state, SDL_Renderer *renderer) {
  if (!state || !renderer || !state->ring_buffer)
    return renderer;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  if (width != state->last_width || height != state->last_height) {
    state->last_width = width;
    state->last_height = height;
  }

  SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
  SDL_RenderClear(renderer);

  append_to_ring_buffer(state);

  if (state->draw_grid) {
    draw_grid(renderer, width, height);
  }

  // Find trigger point for stable waveform
  int trigger_frame = 0;
  bool triggered = find_trigger_point(state, &trigger_frame);

  if (!triggered && state->trigger_mode != 0) {
    return renderer;
  }

  int channel_height = height / state->layout;

  for (int ch = 0; ch < state->layout && ch < MAX_CHANNELS; ch++) {
    int center_y = (ch * channel_height) + (channel_height / 2);
    int half_height = channel_height / 2;

    SDL_Color color = CHANNEL_COLORS[ch % MAX_CHANNELS];
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    char label[20];
    sprintf(label, "CH%d", ch + 1);
    render_text(label, 10, center_y - half_height + 5, renderer, color);

    for (int thickness = 0; thickness < LINE_THICKNESS; thickness++) {
      double sample = state->ring_buffer[trigger_frame * state->layout + ch];

      int prev_x = 0;
      int prev_y =
          center_y - (int)(sample * half_height * 0.8 * state->vertical_scale);

      for (int x = 1; x < width; x++) {
        int frame_idx = (trigger_frame + (int)(x / state->horizontal_scale)) %
                        state->ring_buffer_size;

        sample = state->ring_buffer[frame_idx * state->layout + ch];

        int y = center_y -
                (int)(sample * half_height * 0.8 * state->vertical_scale);

        if (y < ch * channel_height)
          y = ch * channel_height;
        if (y > (ch + 1) * channel_height)
          y = (ch + 1) * channel_height;

        SDL_RenderDrawLine(renderer, prev_x, prev_y + thickness, x,
                           y + thickness);
        prev_x = x;
        prev_y = y;
      }
    }

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawLine(renderer, 0, (ch + 1) * channel_height, width,
                       (ch + 1) * channel_height);
  }

  if (state->trigger_mode != 0) {
    int ch = state->trigger_channel;
    if (ch >= 0 && ch < state->layout) {
      int center_y = (ch * channel_height) + (channel_height / 2);
      int trigger_y = center_y - (int)(state->trigger_level * channel_height /
                                       2 * 0.8 * state->vertical_scale);

      SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange
      SDL_RenderDrawLine(renderer, 0, trigger_y, 20, trigger_y);
    }
  }

  char info[50];
  sprintf(info, "V: %.1fx  H: %.1fx  Trig: %s", state->vertical_scale,
          state->horizontal_scale,
          state->trigger_mode == 0   ? "Auto"
          : state->trigger_mode == 1 ? "Normal"
                                     : "Single");
  render_text(info, width - 200, 10, renderer, (SDL_Color){255, 255, 255, 255});

  return renderer;
}

void scope_event_handler(scope_state *state, SDL_Event *event) {
  if (!state || !event)
    return;

  // Handle key presses
  if (event->type == SDL_KEYDOWN) {
    switch (event->key.keysym.sym) {
    case SDLK_UP:
      // Increase vertical scale
      state->vertical_scale *= 1.2;
      if (state->vertical_scale > 10.0)
        state->vertical_scale = 10.0;
      break;

    case SDLK_DOWN:
      // Decrease vertical scale
      state->vertical_scale /= 1.2;
      if (state->vertical_scale < 0.1)
        state->vertical_scale = 0.1;
      break;

    case SDLK_LEFT:
      // Decrease horizontal scale (zoom in)
      state->horizontal_scale /= 1.2;
      if (state->horizontal_scale < 0.1)
        state->horizontal_scale = 0.1;
      break;

    case SDLK_RIGHT:
      state->horizontal_scale *= 1.2;
      if (state->horizontal_scale > 10.0)
        state->horizontal_scale = 10.0;
      break;

    case SDLK_g:
      state->draw_grid = !state->draw_grid;
      break;

    case SDLK_t:
      state->trigger_mode = (state->trigger_mode + 1) % 3;
      break;

    case SDLK_c:
      state->trigger_channel = (state->trigger_channel + 1) % state->layout;
      break;

    case SDLK_PAGEUP:
      state->trigger_level += 0.05;
      if (state->trigger_level > 1.0)
        state->trigger_level = 1.0;
      break;

    case SDLK_PAGEDOWN:
      state->trigger_level -= 0.05;
      if (state->trigger_level < -1.0)
        state->trigger_level = -1.0;
      break;
    }
  }
}

int create_scope(double *signal, int layout, int size) {
  scope_state *state =
      malloc(sizeof(scope_state) + BUFFER_SIZE * layout * sizeof(double));

  if (!state) {
    fprintf(stderr, "Failed to allocate scope state\n");
    return -1;
  }

  state->layout = layout;
  state->size = size;
  state->buf = signal;
  state->ring_buffer_size = BUFFER_SIZE;
  state->ring_buffer_pos = 0;

  if (state->layout > MAX_CHANNELS) {
    fprintf(stderr, "Warning: Clamping channels from %d to %d\n", state->layout,
            MAX_CHANNELS);
    state->layout = MAX_CHANNELS;
  }

  state->ring_buffer = (double *)(state + 1);

  state->vertical_scale = 1.0;
  state->horizontal_scale = 1.0;
  state->trigger_level = 0.0;
  state->trigger_channel = 0;
  state->draw_grid = false;
  state->trigger_mode = 0; // Auto trigger
  state->last_width = 0;
  state->last_height = 0;

  return create_window(state, scope_renderer, scope_event_handler);
}

#define PLOT_PADDING 40
#define AXIS_COLOR 0xFFAAAAAA
#define GRID_COLOR 0xFF666666
#define BACKGROUND_COLOR 0xFFF0F0F0 // Light grey background

typedef struct plot_state {
  double *buf; // Input signal buffer
  int layout;  // Number of channels (1=mono, 2=stereo, etc.)
  int size;    // Size of the input buffer (in frames)

  // Plot settings
  double vertical_scale;
  double horizontal_scale;
  double horizontal_offset;
  double y_min;
  double y_max;
  bool draw_grid;
  bool draw_axis;
  const char *title;

  // Colors for each channel (up to 8 channels supported)
  uint32_t channel_colors[8];

  // SDL related
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *plot_texture;
  int window_width;
  int window_height;
  bool needs_redraw;
} plot_state;

// Forward declarations
static int plot_event_handler(void *state, SDL_Event *event);
static void create_plot_texture_stacked(plot_state *state,
                                        SDL_Renderer *renderer);
SDL_Renderer *plot_renderer(plot_state *state, SDL_Renderer *renderer);

/**
 * Create a static plot of an array of doubles
 *
 * @param signal Pointer to the signal data (array of doubles)
 * @param layout Number of channels in the signal (1=mono, 2=stereo, etc.)
 * @param size Number of samples per channel
 * @param title Title of the plot (optional, can be NULL)
 * @return 0 on success, -1 on failure
 */
int create_static_plot(int layout, int size, double *signal) {

  // printf("create static plot %d %d\n", layout, size);
  // Allocate plot state
  plot_state *state = malloc(sizeof(plot_state));

  if (!state) {
    fprintf(stderr, "Failed to allocate plot state\n");
    SDL_Quit();
    return -1;
  }

  // Initialize state
  state->buf = signal;
  state->layout = layout;
  state->size = size;
  state->vertical_scale = 1.0;
  state->horizontal_scale = 1.0;
  state->horizontal_offset = 0.0;
  state->draw_grid = true;
  state->draw_axis = true;
  state->window_width = WINDOW_WIDTH;
  state->window_height = WINDOW_HEIGHT;
  state->plot_texture = NULL;
  state->needs_redraw = true;

  state->title = "plot";
  // Set default title if none provided

  // Analyze signal to set y_min and y_max
  state->y_min = 0.0;
  state->y_max = 0.0;

  if (signal != NULL && size > 0) {
    state->y_min = signal[0];
    state->y_max = signal[0];

    for (int i = 0; i < size * layout; i++) {
      if (signal[i] < state->y_min)
        state->y_min = signal[i];
      if (signal[i] > state->y_max)
        state->y_max = signal[i];
    }
  }

  // Add 10% padding to y range
  double range = state->y_max - state->y_min;
  if (range <= 0.0) {
    // If signal is flat or empty, create some range
    state->y_min = -1.0;
    state->y_max = 1.0;
  } else {
    state->y_min -= range * 0.1;
    state->y_max += range * 0.1;
  }

  // Set default colors for each channel
  uint32_t default_colors[8] = {
      0xFFFF0000, // red
      0xFF00FF00, // Green
      0xFF0000FF, // blue
      0xFFFF00FF, // Magenta
      0xFFFFFF00, // Yellow
      0xFF00FFFF, // Cyan
      0xFFFF8000, // Orange
      0xFF8000FF  // Purple
  };

  for (int i = 0; i < 8; i++) {
    state->channel_colors[i] = default_colors[i];
  }

  return create_window(state, plot_renderer, plot_event_handler);
}

/**
 * Render a frame (just copies the texture to the screen)
 */
SDL_Renderer *plot_renderer(plot_state *state, SDL_Renderer *renderer) {

  if (state->needs_redraw) {
    create_plot_texture_stacked(state, renderer);
  }

  if (state->plot_texture) {
    SDL_RenderCopy(renderer, state->plot_texture, NULL, NULL);
  }

  return renderer;
}

static void create_plot_texture_stacked(plot_state *state,
                                        SDL_Renderer *renderer) {
  SDL_GetRendererOutputSize(renderer, &state->window_width,
                            &state->window_height);

  if (state->plot_texture) {
    SDL_DestroyTexture(state->plot_texture);
  }

  state->plot_texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
      state->window_width, state->window_height);

  if (!state->plot_texture) {
    fprintf(stderr, "Failed to create plot texture: %s\n", SDL_GetError());
    return;
  }

  SDL_SetRenderTarget(renderer, state->plot_texture);

  SDL_SetRenderDrawColor(renderer, (BACKGROUND_COLOR >> 16) & 0xFF,
                         (BACKGROUND_COLOR >> 8) & 0xFF,
                         BACKGROUND_COLOR & 0xFF, 255);
  SDL_RenderClear(renderer);

  // Overall plot area
  int plot_x = PLOT_PADDING;
  int plot_y = PLOT_PADDING;
  int plot_width = state->window_width - 2 * PLOT_PADDING;
  int plot_height = state->window_height - 2 * PLOT_PADDING;

  // Calculate individual channel row height
  int channel_count = state->layout > 0 ? state->layout : 1;
  int row_gap = 10; // Gap between channel rows
  int total_gaps = channel_count - 1;
  int row_height = (plot_height - (total_gaps * row_gap)) / channel_count;

  // Ensure minimum row height
  if (row_height < 30) {
    row_height = 30;
    // We could adjust padding here if needed
  }

  // Draw each channel in its own row
  for (int ch = 0; ch < state->layout; ch++) {
    // Calculate this channel's row position
    int row_y = plot_y + ch * (row_height + row_gap);

    // Draw channel background/border
    SDL_Rect row_rect = {
        plot_x - 5,      // Left edge with slight padding
        row_y - 5,       // Top edge with slight padding
        plot_width + 10, // Width with slight extension
        row_height + 10  // Height with slight extension
    };

    // Draw slightly darker background for this row
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light grey (F0F0F0)
    SDL_RenderFillRect(renderer, &row_rect);

    // Draw row border
    SDL_SetRenderDrawColor(renderer, (GRID_COLOR >> 16) & 0xFF,
                           (GRID_COLOR >> 8) & 0xFF, GRID_COLOR & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &row_rect);

    // Draw grid for this row if enabled
    if (state->draw_grid) {
      SDL_SetRenderDrawColor(renderer, (GRID_COLOR >> 16) & 0xFF,
                             (GRID_COLOR >> 8) & 0xFF, GRID_COLOR & 0xFF, 128);

      // Vertical grid lines
      for (int i = 1; i < 10; i++) {
        int x = plot_x + (i * plot_width) / 10;
        SDL_RenderDrawLine(renderer, x, row_y, x, row_y + row_height);
      }

      // Horizontal center line
      int center_y = row_y + row_height / 2;
      SDL_RenderDrawLine(renderer, plot_x, center_y, plot_x + plot_width,
                         center_y);

      // Quarter lines (optional, for taller rows)
      if (row_height > 60) {
        int quarter_y1 = row_y + row_height / 4;
        int quarter_y2 = row_y + 3 * row_height / 4;
        SDL_RenderDrawLine(renderer, plot_x, quarter_y1, plot_x + plot_width,
                           quarter_y1);
        SDL_RenderDrawLine(renderer, plot_x, quarter_y2, plot_x + plot_width,
                           quarter_y2);
      }
    }

    // Draw channel data if available
    if (state->buf && state->size > 0) {
      uint32_t color = state->channel_colors[ch % 8];

      SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF,
                             (color >> 8) & 0xFF, color & 0xFF, 255);

      // Calculate the visible sample range based on horizontal scale and offset
      double visible_range = 1.0 / state->horizontal_scale;

      double center_offset = state->horizontal_offset;

      // Position in normalized coordinate space (0.0 to 1.0)
      double start_pos =
          center_offset - (visible_range / (double)state->layout);
      double end_pos = center_offset + (visible_range / (double)state->layout);

      // Clamp to valid range
      if (start_pos < 0.0)
        start_pos = 0.0;

      if (end_pos > 1.0)
        end_pos = 1.0;

      // Convert to sample indices
      int start_sample = (int)(start_pos * (state->size - 1));
      int end_sample = (int)(end_pos * (state->size - 1));
      printf("start %d %f end %d %f\n", start_sample, start_pos, end_sample,
             end_pos);

      // Ensure we have at least one sample to display
      if (start_sample == end_sample && start_sample < state->size - 1)
        end_sample++;
      if (start_sample == end_sample && start_sample > 0)
        start_sample--;

      // Draw only the visible samples
      for (int i = start_sample; i < end_sample; i++) {
        // Get sample values
        double val1 = state->buf[ch + i * state->layout];
        double val2 = state->buf[ch + (i + 1) * state->layout];

        // Apply vertical scale
        val1 *= state->vertical_scale;
        val2 *= state->vertical_scale;

        // Scale to fit this row's height
        double range = state->y_max - state->y_min;

        // Calculate normalized position (-1 to 1 range)
        double normalized1, normalized2;
        if (range > 0) {
          normalized1 = 2.0 * (val1 - state->y_min) / range - 1.0;
          normalized2 = 2.0 * (val2 - state->y_min) / range - 1.0;
        } else {
          normalized1 = 0;
          normalized2 = 0;
        }

        // Clamp to visible range
        normalized1 =
            normalized1 < -1.0 ? -1.0 : (normalized1 > 1.0 ? 1.0 : normalized1);
        normalized2 =
            normalized2 < -1.0 ? -1.0 : (normalized2 > 1.0 ? 1.0 : normalized2);

        // Map from sample indices to pixels
        double sample_pos1 = (double)i / (state->size - 1);
        double sample_pos2 = (double)(i + 1) / (state->size - 1);

        // Normalize to visible range
        double normalized_pos1 =
            (sample_pos1 - start_pos) / (end_pos - start_pos);
        double normalized_pos2 =
            (sample_pos2 - start_pos) / (end_pos - start_pos);

        // Calculate point positions
        int x1 = plot_x + (int)(normalized_pos1 * plot_width);
        int y1 = row_y + row_height / 2 - (int)(normalized1 * row_height / 2);

        int x2 = plot_x + (int)(normalized_pos2 * plot_width);
        int y2 = row_y + row_height / 2 - (int)(normalized2 * row_height / 2);

        // Draw the line segment if both points are within the plot area
        if (x1 >= plot_x && x1 <= plot_x + plot_width && x2 >= plot_x &&
            x2 <= plot_x + plot_width) {
          SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
      }

      // Use the channel's color for the indicator
      SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF,
                             (color >> 8) & 0xFF, color & 0xFF, 255);

      SDL_Color text_color = {(color >> 16) & 0xFF, (color >> 8) & 0xFF,
                              color & 0xFF, 255};

      char label[20];
      sprintf(label, "CH%d", ch);

      SDL_Surface *surface =
          TTF_RenderText_Blended(DEFAULT_FONT, label, text_color);

      SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
      // Draw channel indicator/label
      SDL_Rect label_rect = {plot_x - 15, // Left of the plot area
                             row_y + 5,   // Near top of row
                             surface->w, surface->h};

      SDL_RenderCopy(renderer, texture, NULL, &label_rect);
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
  state->needs_redraw = false;
}
static int plot_event_handler(void *userdata, SDL_Event *event) {
  plot_state *state = (plot_state *)userdata;

  switch (event->type) {
  case SDL_QUIT:
    return 1; // Exit

  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    case SDLK_ESCAPE:
      return 1; // Exit

    case SDLK_g:
      // Toggle grid
      state->draw_grid = !state->draw_grid;
      state->needs_redraw = true;
      break;

    case SDLK_a:
      // Toggle axes
      state->draw_axis = !state->draw_axis;
      state->needs_redraw = true;
      break;

    case SDLK_PLUS:
    case SDLK_EQUALS: {
      // Zoom in - center stays the same
      double old_scale = state->horizontal_scale;
      state->horizontal_scale *= 1.1;

      // Adjust offset to maintain center point
      double center_point = state->horizontal_offset;
      state->horizontal_offset = center_point;

      state->needs_redraw = true;
      break;
    }

    case SDLK_MINUS: {
      // Zoom out - center stays the same
      double old_scale = state->horizontal_scale;
      state->horizontal_scale /= 1.1;

      // Adjust offset to maintain center point
      double center_point = state->horizontal_offset;
      state->horizontal_offset = center_point;

      state->needs_redraw = true;
      break;
    }

    case SDLK_RIGHT: {
      // Move right - pan the view by a percentage of the visible range
      // The step size is constant in screen space but varies in data space
      // based on zoom When zoomed in (large horizontal_scale), we move by a
      // smaller amount in data space
      //
      double visible_range = 1.0 / state->horizontal_scale;

      double step = visible_range * 0.05; // Move 5% of the visible range

      state->horizontal_offset += step;

      // Clamp to valid range (0.0 to 1.0)
      if (state->horizontal_offset > 1.0) {
        state->horizontal_offset = 1.0;
      }

      state->needs_redraw = true;
      break;
    }

    case SDLK_LEFT: {
      // Move left - pan the view by a percentage of the visible range
      double visible_range = 1.0 / state->horizontal_scale;
      double step = visible_range * 0.05; // Move 5% of the visible range

      state->horizontal_offset -= step;

      // Clamp to valid range (0.0 to 1.0)
      if (state->horizontal_offset < 0.0) {
        state->horizontal_offset = 0.0;
      }

      state->needs_redraw = true;
      break;
    }
    }
    break;

  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
      // Window size changed, need to recreate texture
      state->needs_redraw = true;
    }
    break;
  }

  return 0; // Continue running
}

typedef void (*EnvEditCb)(int32_t size, double *data);
typedef struct env_edit_state {
  int size;
  double *data;
  int selected_point;
  bool dragging;
  SDL_Point drag_start;
  double view_min_y;
  double view_max_y;
  double view_min_x;
  double view_max_x;
  double scale_x;
  double scale_y;
  int num_points;
  EnvEditCb cb;
} env_edit_state;

// Define margins
#define ENV_MARGIN 30
const int margin_left = ENV_MARGIN;
const int margin_right = ENV_MARGIN;
const int margin_top = ENV_MARGIN;
const int margin_bottom = ENV_MARGIN;

void print_env(env_edit_state *state) {
  int i;

  printf("\n#-----\n#Env:\n");
  printf("[|\n");
  for (i = 0; i < state->num_points - 1; i++) {
    printf("  %f, %f, %f,\n", state->data[i * 3], state->data[i * 3 + 1],
           state->data[i * 3 + 2]);
  }

  printf("  %f\n", state->data[i * 3]);

  printf("|];\n");
}

static SDL_Point data_to_screen(env_edit_state *state, double x, double y,
                                int width, int height) {
  SDL_Point point;

  // Calculate the actual plotting area dimensions
  const int plot_width = width - margin_left - margin_right;
  const int plot_height = height - margin_top - margin_bottom;

  double normalized_x =
      (x - state->view_min_x) / (state->view_max_x - state->view_min_x);
  double normalized_y =
      1.0 - (y - state->view_min_y) / (state->view_max_y - state->view_min_y);

  point.x = margin_left + (int)(normalized_x * plot_width);
  point.y = margin_top + (int)(normalized_y * plot_height);

  return point;
}

// Convert screen coordinates to data coordinates
static void screen_to_data(env_edit_state *state, int screen_x, int screen_y,
                           int width, int height, double *data_x,
                           double *data_y) {

  const int plot_width = width - margin_left - margin_right;
  const int plot_height = height - margin_top - margin_bottom;

  double normalized_x = (double)(screen_x - margin_left) / plot_width;
  double normalized_y = (double)(screen_y - margin_top) / plot_height;

  normalized_y = 1.0 - normalized_y;

  *data_x = state->view_min_x +
            normalized_x * (state->view_max_x - state->view_min_x);
  *data_y = state->view_min_y +
            normalized_y * (state->view_max_y - state->view_min_y);
}

double *env_val_ptr(env_edit_state *state, int point_idx) {
  return state->data + (point_idx * 3);
}

double *env_time_ptr(env_edit_state *state, int point_idx) {
  return state->data + (point_idx * 3) + 1;
}

double *env_curve_ptr(env_edit_state *state, int point_idx) {
  return state->data + (point_idx * 3) + 2;
}

static int find_closest_point(env_edit_state *state, int mouse_x, int mouse_y,
                              int width, int height) {
  int closest_point = -1;
  int min_distance = INT_MAX;

  int num_points = state->num_points;

  double t = 0.;
  for (int i = 0; i < num_points; i++) {
    // Get the x and y coordinates of the point
    double point_x;
    double point_y;

    if (i == 0) {
      point_x = 0.0;
      point_y = state->data[0];
    } else if (i == (num_points - 1)) {
      point_x = t + *env_time_ptr(state, i - 1);
      point_y = *env_val_ptr(state, i);
      t = point_x;
    } else {
      // For subsequent points, calculate x by summing all time intervals
      point_x = t + *env_time_ptr(state, i - 1);
      point_y = *env_val_ptr(state, i);
      t = point_x;
    }

    // Convert to screen coordinates
    SDL_Point screen_point =
        data_to_screen(state, point_x, point_y, width, height);

    // Calculate distance (squared) to mouse position
    int dx = screen_point.x - mouse_x;
    int dy = screen_point.y - mouse_y;
    int distance = dx * dx + dy * dy;

    if (distance < min_distance) {
      min_distance = distance;
      closest_point = i;
    }
  }

  const int MAX_DISTANCE_THRESHOLD = 400; // 20 pixels squared
  if (min_distance > MAX_DISTANCE_THRESHOLD) {
    return -1; // No point is close enough
  }

  return closest_point;
}

// Helper function to calculate x position for a specific point index
static double get_point_x(env_edit_state *state, int point_index) {
  if (point_index == 0) {
    return 0.0; // First point is at x=0
  }

  double x = 0.0;
  // Sum up all the time intervals before this point
  for (int i = 0; i < point_index; i++) {
    // Time value is at index 3*i + 1 (except for the last point)
    x += *env_time_ptr(state, i);
  }

  return x;
}
static int env_edit_event_handler(void *userdata, SDL_Event *event) {
  SDL_Window *win = get_window(*event)->window;
  env_edit_state *state = (env_edit_state *)userdata;
  int width, height;
  SDL_GetRendererOutputSize(SDL_GetRenderer(win), &width, &height);

  int window_w, window_h, drawable_w, drawable_h;
  SDL_GetWindowSize(win, &window_w, &window_h);
  SDL_GL_GetDrawableSize(win, &drawable_w, &drawable_h);

  double scale_x = (double)drawable_w / window_w;
  double scale_y = (double)drawable_h / window_h;

  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      int distance = 200;
      int point_index =
          find_closest_point(state, scale_x * event->button.x,
                             scale_x * event->button.y, width, height);

      if (point_index >= 0) {
        state->selected_point = point_index;
        state->dragging = true;
        state->drag_start.x = scale_x * event->button.x;
        state->drag_start.y = scale_y * event->button.y;
      }
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT && state->dragging) {
      state->dragging = false;
      return 1; // Event handled
    }
    break;

  case SDL_MOUSEMOTION: {
    if (state->dragging && state->selected_point >= 0) {
      double x = scale_x * event->motion.x;
      double y = scale_y * event->motion.y;
      double data_x = 0.;
      double data_y = 0.;

      screen_to_data(state, x, y, width, height, &data_x, &data_y);

      printf("dragging to (%f,%f) -> (%f,%f) (dx: %f)\n", x, y, data_x, data_y,
             data_x - get_point_x(state, state->selected_point));
      *env_val_ptr(state, state->selected_point) = fmax(0., fmin(1., data_y));
      if (state->selected_point > 0 &&
          state->selected_point < (state->num_points - 1)) {
        double *prev_dt = env_time_ptr(state, state->selected_point - 1);
        double *next_dt = env_time_ptr(state, state->selected_point);

        double current_x = get_point_x(state, state->selected_point);

        double delta_x = data_x - current_x;

        double new_prev_dt = *prev_dt + delta_x;
        double new_next_dt = *next_dt - delta_x;

        const double MIN_TIME_INTERVAL = 0.0;

        if (new_prev_dt >= MIN_TIME_INTERVAL &&
            new_next_dt >= MIN_TIME_INTERVAL) {
          *prev_dt = new_prev_dt;
          *next_dt = new_next_dt;
        } else {
          if (new_prev_dt < MIN_TIME_INTERVAL) {
            delta_x = MIN_TIME_INTERVAL - *prev_dt;

            // Update intervals with adjusted delta
            *prev_dt = MIN_TIME_INTERVAL;
            *next_dt = *next_dt - delta_x;
          } else {
            delta_x = *next_dt - MIN_TIME_INTERVAL;

            *prev_dt = *prev_dt + delta_x;
            *next_dt = MIN_TIME_INTERVAL;
          }
        }

        printf("Time intervals updated: prev=%.3f, next=%.3f\n", *prev_dt,
               *next_dt);
      }

      if (state->cb != NULL) {
        state->cb(state->size, state->data);
      }

      return 1; // Event handled
    }
    break;
  }

  case SDL_KEYDOWN: {
    // Adjust the curve shape with arrow keys when a point is selected
    if (state->selected_point > 0 &&
        state->selected_point < state->num_points) {
      int curve_index = 3 * (state->selected_point - 1) + 2;

      if (event->key.keysym.sym == SDLK_UP) {
        state->data[curve_index] += 0.1;
        return 1;
      } else if (event->key.keysym.sym == SDLK_DOWN) {
        state->data[curve_index] -= 0.1;
        return 1;
      } else if (event->key.keysym.sym == SDLK_0) {
        state->data[curve_index] = 0.0; // Reset to linear
        return 1;
      }
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    if (state->selected_point > 0) {
      // printf("mouse wheel %d %d\n",state->selected_point, event->wheel.y);
      double *curve_ptr = env_curve_ptr(state, state->selected_point - 1);
      if (event->wheel.y > 0) {
        *curve_ptr -= 0.1;
      } else {
        *curve_ptr += 0.1;
      }
    }
  } break;
  }

  return 0; // Event not handled
}

#define SET_RED SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
#define SET_GREY SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255)
static SDL_Renderer *draw_points(env_edit_state *state,
                                 SDL_Renderer *renderer) {

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  int i;
  double t;
  double x;
  const int radius = 6;
  for (i = 0; i < state->num_points - 1; i++) {
    double val = *env_val_ptr(state, i);
    x = t;
    SDL_Point p = data_to_screen(state, x, val, width, height);
    state->selected_point == i ? SET_RED : SET_GREY;

    SDL_RenderFillRect(
        renderer,
        &(SDL_Rect){p.x - (radius / 2), p.y - (radius / 2), radius, radius});

    t += *env_time_ptr(state, i);
  }

  double val = *env_val_ptr(state, i);
  x = t;
  SDL_Point p = data_to_screen(state, x, val, width, height);

  state->selected_point == i ? SET_RED : SET_GREY;

  SDL_RenderFillRect(renderer, &(SDL_Rect){p.x - (radius / 2),
                                           p.y - (radius / 2), radius, radius});

  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  return renderer;
}

// Helper function to interpolate between two points based on curve parameter
static double interpolate_value(double t, double y1, double y2, double curve) {
  // Linear interpolation if curve is close to zero
  if (fabs(curve) < 0.001) {
    return y1 + t * (y2 - y1);
  } else {
    // Exponential curve
    double sign = curve > 0 ? 1.0 : -1.0;
    double k = exp(sign * fabs(curve) * 3.0); // Scale the curve effect
    double curve_t = (exp(sign * fabs(curve) * 3.0 * t) - 1) / (k - 1);
    return y1 + curve_t * (y2 - y1);
  }
}

static SDL_Renderer *draw_curves(env_edit_state *state,
                                 SDL_Renderer *renderer) {
  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  int num_points = state->num_points;

  // Set line color
  SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255); // Yellow-orange

  // Number of segments to sample for each curve
  const int CURVE_SEGMENTS = 30;

  for (int i = 0; i < state->num_points - 1; i++) {
    // Get start point
    double x1 = get_point_x(state, i);
    double y1 = *env_val_ptr(state, i);

    // Get end point
    double x2 = get_point_x(state, i + 1);
    double y2 = *env_val_ptr(state, i + 1);

    // Get curve parameter
    double curve = *env_curve_ptr(state, i);

    // Draw a straight line if curve is near zero, otherwise draw a curved line
    if (fabs(curve) < 0.001) {
      // Straight line - just draw from point to point
      SDL_Point p1 = data_to_screen(state, x1, y1, width, height);
      SDL_Point p2 = data_to_screen(state, x2, y2, width, height);
      SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
    } else {
      // Curved line - sample points along the curve
      SDL_Point prev_point = data_to_screen(state, x1, y1, width, height);

      for (int j = 1; j <= CURVE_SEGMENTS; j++) {
        // Calculate parameter t in [0,1]
        double t = (double)j / CURVE_SEGMENTS;

        // Calculate x position (linear interpolation)
        double x = x1 + t * (x2 - x1);

        // Calculate y position (using curve parameter)
        double y = interpolate_value(t, y1, y2, curve);

        // Convert to screen coordinates
        SDL_Point point = data_to_screen(state, x, y, width, height);

        // Draw line segment
        SDL_RenderDrawLine(renderer, prev_point.x, prev_point.y, point.x,
                           point.y);

        // Update previous point
        prev_point = point;
      }
    }

    // Optionally, indicate the curve type with a small marker
    // double mid_x = (x1 + x2) / 2.0;
    // double mid_y;

    // Calculate the actual mid-point y value based on the curve
    // mid_y = interpolate_value(0.5, y1, y2, curve);

    // SDL_Point mid_point = data_to_screen(state, mid_x, mid_y, width, height);

    // SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // Linear - gray

    // // Draw a small square to indicate curve type
    // const int size = 3;
    // SDL_Rect rect = {mid_point.x - size, mid_point.y - size, size * 2, size *
    // 2}; SDL_RenderFillRect(renderer, &rect);
    //
    // // Reset line color for next segment
    // SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
  }

  return renderer;
}

static SDL_Renderer *env_edit_renderer(plot_state *plot,
                                       SDL_Renderer *renderer) {
  env_edit_state *state = (env_edit_state *)plot;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  const int plot_width = width - margin_left - margin_right;
  const int plot_height = height - margin_top - margin_bottom;

  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
  SDL_Rect margin_rect = {0, 0, width, height};
  SDL_RenderFillRect(renderer, &margin_rect);

  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_Rect plot_rect = {margin_left, margin_top, plot_width, plot_height};
  SDL_RenderFillRect(renderer, &plot_rect);

  draw_points(state, renderer);
  draw_curves(state, renderer);

  return renderer;
}

int create_envelope_edit_view(int size, double *data) {
  env_edit_state *state = malloc(sizeof(env_edit_state));

  if (!state) {
    fprintf(stderr, "Failed to allocate env edit state\n");
    return -1;
  }

  state->size = size;
  state->data = data;
  state->selected_point = -1;
  state->dragging = false;
  state->num_points = (size + 2) / 3;

  // Find the data bounds for initial view
  state->view_min_y = 0.0;
  state->view_max_y = 1.0;

  // For x-axis, use the total duration of the envelope
  state->view_min_x = 0.0;
  state->view_max_x =
      get_point_x(state, state->num_points - 1) * 1.05; // Add 5% margin

  // If the view is empty or invalid, set default view
  if (state->view_max_x <= state->view_min_x) {
    state->view_min_x = 0.0;
    state->view_max_x = 1.0;
  }

  if (state->view_max_y <= state->view_min_y) {
    state->view_min_y = 0.0;
    state->view_max_y = 1.0;
  }

  // Calculate scaling factors
  state->scale_x = 1.0 / (state->view_max_x - state->view_min_x);
  state->scale_y = 1.0 / (state->view_max_y - state->view_min_y);

  return create_window(state, env_edit_renderer, env_edit_event_handler);
}

int create_envelope_edit_view_cb(int size, double *data, void *cb) {
  env_edit_state *state = malloc(sizeof(env_edit_state));

  if (!state) {
    fprintf(stderr, "Failed to allocate env edit state\n");
    return -1;
  }

  state->size = size;
  state->data = data;
  state->selected_point = -1;
  state->dragging = false;
  state->num_points = (size + 2) / 3;

  // Find the data bounds for initial view
  state->view_min_y = 0.0;
  state->view_max_y = 1.0;

  // For x-axis, use the total duration of the envelope
  state->view_min_x = 0.0;
  state->view_max_x =
      get_point_x(state, state->num_points - 1) * 1.05; // Add 5% margin

  // If the view is empty or invalid, set default view
  if (state->view_max_x <= state->view_min_x) {
    state->view_min_x = 0.0;
    state->view_max_x = 1.0;
  }

  if (state->view_max_y <= state->view_min_y) {
    state->view_min_y = 0.0;
    state->view_max_y = 1.0;
  }

  // Calculate scaling factors
  state->scale_x = 1.0 / (state->view_max_x - state->view_min_x);
  state->scale_y = 1.0 / (state->view_max_y - state->view_min_y);
  state->cb = cb;

  return create_window(state, env_edit_renderer, env_edit_event_handler);
}
#define MAX_PARAM_NAME_LENGTH 64
#define MAX_VISIBLE_PARAMS 16 // Maximum number of parameters to show at once
#define SLIDER_HEIGHT 20
#define SLIDER_WIDTH 200
#define SLIDER_SPACING 30
#define PARAM_NAME_WIDTH 120

// Expanded structure to store parameter information
typedef struct {
  VSTPluginHandle plugin;
  SDL_Window *window;
  bool editor_open;
  int width;
  int height;

  // Parameters
  int param_count;
  int scroll_offset;   // For scrolling through parameters
  char **param_names;  // Array of parameter names
  float *param_values; // Current parameter values (0.0 to 1.0)
  int dragging_param;  // Currently dragged parameter (-1 if none)
} VSTEditorState;

// Forward declarations
static void update_parameter_values(VSTEditorState *state);
static void draw_parameter_sliders(VSTEditorState *state,
                                   SDL_Renderer *renderer);
static int handle_mouse_param_interaction(VSTEditorState *state, int x, int y,
                                          bool is_down);

static SDL_Renderer *vst_renderer(VSTEditorState *state,
                                  SDL_Renderer *renderer);
void vst_event_handler(VSTEditorState *state, SDL_Event *event);

// Initialize plugin UI window
int create_vst_view(char *plugin_handle) {
  VSTPluginHandle plugin = (VSTPluginHandle)plugin_handle;

  // Create editor state
  VSTEditorState *state = (VSTEditorState *)malloc(sizeof(VSTEditorState));
  if (!state) {
    return -1;
  }

  // Basic initialization
  state->plugin = plugin;
  state->editor_open = false;
  state->width = 400; // Default window size
  state->height = 600;
  state->scroll_offset = 0;
  state->dragging_param = -1;

  // Get parameter count from plugin
  if (vst_get_parameter_count(plugin, &state->param_count) != VST_ERR_OK) {
    printf("Failed to get parameter count from plugin\n");
    free(state);
    return -1;
  }

  printf("Plugin has %d parameters\n", state->param_count);

  // Allocate memory for parameter names and values
  state->param_names = (char **)malloc(state->param_count * sizeof(char *));
  state->param_values = (float *)malloc(state->param_count * sizeof(float));

  if (!state->param_names || !state->param_values) {
    printf("Failed to allocate memory for parameters\n");
    free(state->param_names);
    free(state->param_values);
    free(state);
    return -1;
  }

  // Initialize parameter arrays
  for (int i = 0; i < state->param_count; i++) {
    state->param_names[i] = (char *)malloc(MAX_PARAM_NAME_LENGTH);
    if (!state->param_names[i]) {
      // Handle allocation failure
      for (int j = 0; j < i; j++) {
        free(state->param_names[j]);
      }
      free(state->param_names);
      free(state->param_values);
      free(state);
      return -1;
    }

    // Get parameter name
    if (vst_get_parameter_name(plugin, i, state->param_names[i],
                               MAX_PARAM_NAME_LENGTH) != VST_ERR_OK) {
      strcpy(state->param_names[i], "Unknown");
    }

    // Get parameter value
    if (vst_get_parameter(plugin, i, &state->param_values[i]) != VST_ERR_OK) {
      state->param_values[i] = 0.0f;
    }

    printf("Parameter %d: %s = %.2f\n", i, state->param_names[i],
           state->param_values[i]);
  }

  // Create window with custom size based on parameter count
  return create_window(state, vst_renderer, vst_event_handler);
}

// Render the UI
static SDL_Renderer *vst_renderer(VSTEditorState *state,
                                  SDL_Renderer *renderer) {
  if (!state) {
    return renderer;
  }

  // Clear the background
  SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255); // Dark background
  SDL_RenderClear(renderer);

  // Update parameter values from plugin (in case they've changed)
  update_parameter_values(state);

  // Draw the parameter sliders
  draw_parameter_sliders(state, renderer);

  // Draw title
  SDL_Color text_color = {220, 220, 220, 255}; // Light gray text
  render_text("VST Plugin Parameters", 10, 10, renderer, text_color);

  // Draw scroll indicators if needed
  if (state->param_count > MAX_VISIBLE_PARAMS) {
    if (state->scroll_offset > 0) {
      // Draw up arrow
      SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
      // Triangle pointing up at the top
      int x_center = state->width / 2;
      SDL_RenderDrawLine(renderer, x_center, 40, x_center - 10, 50);
      SDL_RenderDrawLine(renderer, x_center, 40, x_center + 10, 50);
      SDL_RenderDrawLine(renderer, x_center - 10, 50, x_center + 10, 50);
    }

    if (state->scroll_offset + MAX_VISIBLE_PARAMS < state->param_count) {
      // Draw down arrow
      SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
      // Triangle pointing down at the bottom
      int x_center = state->width / 2;
      int y_bottom = state->height - 20;
      SDL_RenderDrawLine(renderer, x_center, y_bottom, x_center - 10,
                         y_bottom - 10);
      SDL_RenderDrawLine(renderer, x_center, y_bottom, x_center + 10,
                         y_bottom - 10);
      SDL_RenderDrawLine(renderer, x_center - 10, y_bottom - 10, x_center + 10,
                         y_bottom - 10);
    }
  }

  return renderer;
}

// Handle SDL events
void vst_event_handler(VSTEditorState *state, SDL_Event *event) {
  if (!state || !event) {
    return;
  }

  // Handle user input
  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      // Check if clicking on a parameter slider
      int param_idx = handle_mouse_param_interaction(state, event->button.x,
                                                     event->button.y, true);
      if (param_idx >= 0) {
        state->dragging_param = param_idx;
      }
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT) {
      state->dragging_param = -1; // Stop dragging
    }
    break;

  case SDL_MOUSEMOTION:
    if (state->dragging_param >= 0) {
      // Update parameter value based on mouse position
      int slider_x = PARAM_NAME_WIDTH + 10;
      int real_param_idx = state->dragging_param + state->scroll_offset;

      // Calculate slider position
      float normalized_pos = (float)(event->motion.x - slider_x) / SLIDER_WIDTH;
      normalized_pos =
          fmax(0.0f, fmin(1.0f, normalized_pos)); // Clamp to 0.0-1.0

      // Set new parameter value
      state->param_values[real_param_idx] = normalized_pos;
      vst_set_parameter(state->plugin, real_param_idx, normalized_pos);
    }
    break;

  case SDL_MOUSEWHEEL:
    // Scroll through parameters
    if (state->param_count > MAX_VISIBLE_PARAMS) {
      state->scroll_offset -= event->wheel.y; // Scroll direction

      // Clamp scroll offset
      if (state->scroll_offset < 0) {
        state->scroll_offset = 0;
      }
      if (state->scroll_offset > state->param_count - MAX_VISIBLE_PARAMS) {
        state->scroll_offset = state->param_count - MAX_VISIBLE_PARAMS;
      }
    }
    break;

  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    case SDLK_UP:
      // Scroll up
      if (state->scroll_offset > 0) {
        state->scroll_offset--;
      }
      break;

    case SDLK_DOWN:
      // Scroll down
      if (state->param_count > MAX_VISIBLE_PARAMS &&
          state->scroll_offset < state->param_count - MAX_VISIBLE_PARAMS) {
        state->scroll_offset++;
      }
      break;

    case SDLK_HOME:
      // Scroll to top
      state->scroll_offset = 0;
      break;

    case SDLK_END:
      // Scroll to bottom
      if (state->param_count > MAX_VISIBLE_PARAMS) {
        state->scroll_offset = state->param_count - MAX_VISIBLE_PARAMS;
      } else {
        state->scroll_offset = 0;
      }
      break;
    }
    break;
  }
}

// Update parameter values from the plugin
static void update_parameter_values(VSTEditorState *state) {
  if (!state)
    return;

  for (int i = 0; i < state->param_count; i++) {
    float value;
    if (vst_get_parameter(state->plugin, i, &value) == VST_ERR_OK) {
      state->param_values[i] = value;
    }
  }
}

// Draw parameter sliders
static void draw_parameter_sliders(VSTEditorState *state,
                                   SDL_Renderer *renderer) {
  if (!state || !renderer)
    return;

  SDL_Color text_color = {200, 200, 200, 255};  // Light gray text
  SDL_Color value_color = {240, 240, 150, 255}; // Yellow-ish for values

  // Start position for parameters
  int y_pos = 60;

  // Determine how many parameters to show
  int display_count =
      fmin(MAX_VISIBLE_PARAMS, state->param_count - state->scroll_offset);

  // Draw each visible parameter
  for (int i = 0; i < display_count; i++) {
    int param_idx = i + state->scroll_offset;

    // Draw parameter name
    render_text(state->param_names[param_idx], 10, y_pos, renderer, text_color);

    // Draw slider background
    int slider_x = PARAM_NAME_WIDTH + 10;
    int slider_y = y_pos;

    SDL_Rect slider_bg = {slider_x, slider_y, SLIDER_WIDTH, SLIDER_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); // Dark gray background
    SDL_RenderFillRect(renderer, &slider_bg);

    // Draw slider value fill
    int fill_width = (int)(state->param_values[param_idx] * SLIDER_WIDTH);
    SDL_Rect slider_fill = {slider_x, slider_y, fill_width, SLIDER_HEIGHT};

    // Different colors for different parameter types (just for visual variety)
    int hue = (param_idx * 15) % 360;

    // Simple HSV to RGB conversion for variety
    float s = 0.6f, v = 0.8f;
    float c = v * s;
    float x = c * (1 - fabs(fmod(hue / 60.0f, 2) - 1));
    float m = v - c;

    float r, g, b;
    if (hue < 60) {
      r = c;
      g = x;
      b = 0;
    } else if (hue < 120) {
      r = x;
      g = c;
      b = 0;
    } else if (hue < 180) {
      r = 0;
      g = c;
      b = x;
    } else if (hue < 240) {
      r = 0;
      g = x;
      b = c;
    } else if (hue < 300) {
      r = x;
      g = 0;
      b = c;
    } else {
      r = c;
      g = 0;
      b = x;
    }

    SDL_SetRenderDrawColor(renderer, (Uint8)((r + m) * 255),
                           (Uint8)((g + m) * 255), (Uint8)((b + m) * 255), 255);
    SDL_RenderFillRect(renderer, &slider_fill);

    // Draw slider border
    SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255); // Light gray border
    SDL_RenderDrawRect(renderer, &slider_bg);

    // Draw value as text
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%.2f",
             state->param_values[param_idx]);
    render_text(value_str, slider_x + SLIDER_WIDTH + 10, y_pos, renderer,
                value_color);

    // Draw display value if available (using vst_get_parameter_display)
    char display_str[32] = {0};
    if (vst_get_parameter_display(state->plugin, param_idx, display_str,
                                  sizeof(display_str)) == VST_ERR_OK &&
        display_str[0] != '\0') {
      // Get parameter label too
      char label_str[16] = {0};
      vst_get_parameter_label(state->plugin, param_idx, label_str,
                              sizeof(label_str));

      // Combine display and label
      char formatted_value[48];
      if (label_str[0] != '\0') {
        snprintf(formatted_value, sizeof(formatted_value), "%s %s", display_str,
                 label_str);
      } else {
        snprintf(formatted_value, sizeof(formatted_value), "%s", display_str);
      }

      render_text(formatted_value, slider_x + SLIDER_WIDTH + 70, y_pos,
                  renderer, value_color);
    }

    // Move to next parameter position
    y_pos += SLIDER_SPACING;
  }
}

// Handle mouse interaction with parameter sliders
static int handle_mouse_param_interaction(VSTEditorState *state, int x, int y,
                                          bool is_down) {
  if (!state)
    return -1;

  // Check if mouse is in the parameter slider area
  int slider_x = PARAM_NAME_WIDTH + 10;

  if (x >= slider_x && x <= slider_x + SLIDER_WIDTH) {
    // Calculate which parameter based on y position
    int y_pos = 60;

    for (int i = 0; i < fmin(MAX_VISIBLE_PARAMS,
                             state->param_count - state->scroll_offset);
         i++) {
      if (y >= y_pos && y < y_pos + SLIDER_HEIGHT) {
        return i; // Return visible parameter index (not the actual parameter
                  // index)
      }
      y_pos += SLIDER_SPACING;
    }
  }

  return -1; // No parameter slider hit
}

// Cleanup function for VST editor
void cleanup_vst_editor(VSTEditorState *state) {
  if (!state)
    return;

  // Free parameter names
  if (state->param_names) {
    for (int i = 0; i < state->param_count; i++) {
      free(state->param_names[i]);
    }
    free(state->param_names);
  }

  // Free parameter values
  free(state->param_values);

  // Free state
  free(state);
}
typedef struct array_edit_state {
  double *data;       // Pointer to the array data
  int size;           // Number of elements in the array
  int selected_index; // Currently selected/active index
  bool dragging;      // Whether the user is currently dragging
  int last_width;     // Last known window width
  int last_height;    // Last known window height
  double value_min;   // Minimum value (typically 0.0)
  double value_max;   // Maximum value (typically 1.0)
  bool display_grid;  // Whether to display grid lines
} array_edit_state;

// Forward declarations
static int array_edit_event_handler(void *state, SDL_Event *event);
static SDL_Renderer *array_edit_renderer(array_edit_state *state,
                                         SDL_Renderer *renderer);

/**
 * Create an interactive array editor
 *
 * @param data Pointer to the array of doubles (values between 0.0 and 1.0)
 * @param size Number of elements in the array
 * @return 0 on success, -1 on failure
 */
int create_array_editor(int size, double *data) {
  // Allocate state
  array_edit_state *state = malloc(sizeof(array_edit_state));

  if (!state) {
    fprintf(stderr, "Failed to allocate array editor state\n");
    return -1;
  }

  // Initialize state
  state->data = data;
  state->size = size;
  state->selected_index = -1;
  state->dragging = false;
  state->last_width = 0;
  state->last_height = 0;
  state->value_min = 0.0;
  state->value_max = 1.0;
  state->display_grid = true;

  return create_window(state, array_edit_renderer, array_edit_event_handler);
}

// Convert array index and value to screen coordinates
static SDL_Point array_point_to_screen(array_edit_state *state, int index,
                                       double value, int width, int height) {
  SDL_Point point;

  // Calculate margins
  const int margin = 40;
  const int plot_width = width - 2 * margin;
  const int plot_height = height - 2 * margin;

  // Calculate x based on index
  double x_ratio = (double)index / (state->size > 1 ? state->size - 1 : 1);
  point.x = margin + (int)(x_ratio * plot_width);

  // Calculate y based on value (flipped, as screen coordinates have y=0 at top)
  double normalized_value =
      (value - state->value_min) / (state->value_max - state->value_min);
  point.y = height - margin - (int)(normalized_value * plot_height);

  return point;
}

// Convert screen coordinates to array value
static void screen_to_array_value(array_edit_state *state, int x, int y,
                                  int width, int height, int *index,
                                  double *value) {
  // Calculate margins
  const int margin = 40;
  const int plot_width = width - 2 * margin;
  const int plot_height = height - 2 * margin;

  // Calculate nearest index
  double x_ratio = (double)(x - margin) / plot_width;
  *index = (int)(x_ratio * (state->size - 1) + 0.5);

  // Clamp index to valid range
  if (*index < 0)
    *index = 0;
  if (*index >= state->size)
    *index = state->size - 1;

  // Calculate value from y coordinate
  double y_ratio = (double)(height - margin - y) / plot_height;
  *value = state->value_min + y_ratio * (state->value_max - state->value_min);

  // Clamp value to valid range
  if (*value < state->value_min)
    *value = state->value_min;
  if (*value > state->value_max)
    *value = state->value_max;
}

// Find the closest point to the given screen coordinates
static int find_closest_array_point(array_edit_state *state, int mouse_x,
                                    int mouse_y, int width, int height) {
  int closest_index = -1;
  int min_distance_squared = INT_MAX;

  for (int i = 0; i < state->size; i++) {
    SDL_Point point =
        array_point_to_screen(state, i, state->data[i], width, height);

    int dx = point.x - mouse_x;
    int dy = point.y - mouse_y;
    int distance_squared = dx * dx + dy * dy;

    if (distance_squared < min_distance_squared) {
      min_distance_squared = distance_squared;
      closest_index = i;
    }
  }

  // Use a threshold to determine if the mouse is close enough to any point
  const int THRESHOLD_SQUARED = 100; // 10 pixels squared
  if (min_distance_squared > THRESHOLD_SQUARED) {
    return -1;
  }

  return closest_index;
}

// Renderer function for the array editor
static SDL_Renderer *array_edit_renderer(array_edit_state *state,
                                         SDL_Renderer *renderer) {
  if (!state || !renderer)
    return renderer;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  // Check if window size has changed
  if (width != state->last_width || height != state->last_height) {
    state->last_width = width;
    state->last_height = height;
  }

  // Clear background
  SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light grey background
  SDL_RenderClear(renderer);

  // Define margin and plot area
  const int margin = 40;
  const int plot_width = width - 2 * margin;
  const int plot_height = height - 2 * margin;
  SDL_Rect plot_area = {margin, margin, plot_width, plot_height};

  // Draw plot background
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
  SDL_RenderFillRect(renderer, &plot_area);

  // Draw grid if enabled
  if (state->display_grid) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light grey grid

    // Vertical grid lines
    for (int i = 0; i <= 10; i++) {
      int x = margin + (i * plot_width) / 10;
      SDL_RenderDrawLine(renderer, x, margin, x, height - margin);
    }

    // Horizontal grid lines
    for (int i = 0; i <= 10; i++) {
      int y = margin + (i * plot_height) / 10;
      SDL_RenderDrawLine(renderer, margin, y, width - margin, y);
    }
  }

  // Draw border around plot area
  SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255); // Darker grey border
  SDL_RenderDrawRect(renderer, &plot_area);

  // Draw array data as bars
  for (int i = 0; i < state->size; i++) {
    // Calculate bar position and dimensions
    double bar_width = (double)plot_width / state->size;
    int x = margin + (int)(i * bar_width);
    int bar_width_pixels = (int)bar_width - 2; // Slight gap between bars
    if (bar_width_pixels < 2)
      bar_width_pixels = 2; // Minimum width

    double value = state->data[i];
    int bar_height = (int)((value - state->value_min) /
                           (state->value_max - state->value_min) * plot_height);

    SDL_Rect bar = {x + 1, // +1 to create slight separation
                    height - margin - bar_height, bar_width_pixels, bar_height};

    // Different color for selected bar
    if (i == state->selected_index) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for selected
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255); // Blue for others
    }

    SDL_RenderFillRect(renderer, &bar);

    // Draw bar outline
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &bar);
  }

  // Draw array data as line
  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); // Dark line

  for (int i = 0; i < state->size - 1; i++) {
    SDL_Point p1 =
        array_point_to_screen(state, i, state->data[i], width, height);
    SDL_Point p2 =
        array_point_to_screen(state, i + 1, state->data[i + 1], width, height);
    SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
  }

  // Draw points at each array position
  const int point_radius = 5;
  for (int i = 0; i < state->size; i++) {
    SDL_Point p =
        array_point_to_screen(state, i, state->data[i], width, height);

    // Draw filled circle for each point
    if (i == state->selected_index) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for selected
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for others
    }

    // Simple filled rect for points
    SDL_Rect point_rect = {p.x - point_radius / 2, p.y - point_radius / 2,
                           point_radius, point_radius};
    SDL_RenderFillRect(renderer, &point_rect);
  }

  // Display array index and value info for selected point
  if (state->selected_index >= 0) {
    char info[64];
    sprintf(info, "Index: %d  Value: %.3f", state->selected_index,
            state->data[state->selected_index]);

    SDL_Color text_color = {0, 0, 0, 255}; // Black text
    render_text(info, margin, 10, renderer, text_color);
  }

  // Display title
  SDL_Color title_color = {0, 0, 0, 255}; // Black text
  render_text("Array Editor", width / 2 - 40, 10, renderer, title_color);

  return renderer;
}
static int array_edit_event_handler(void *userdata, SDL_Event *event) {
  array_edit_state *state = (array_edit_state *)userdata;

  if (!state)
    return 0;

  SDL_Window *win = get_window(*event)->window;
  int width, height;
  SDL_GetRendererOutputSize(SDL_GetRenderer(win), &width, &height);

  // Handle display scaling factors (important for high-DPI displays)
  int window_w, window_h, drawable_w, drawable_h;
  SDL_GetWindowSize(win, &window_w, &window_h);
  SDL_GL_GetDrawableSize(win, &drawable_w, &drawable_h);

  double scale_x = (double)drawable_w / window_w;
  double scale_y = (double)drawable_h / window_h;

  switch (event->type) {
  case SDL_QUIT:
    return 1; // Exit

  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      // Scale mouse coordinates
      int mouse_x = (int)(scale_x * event->button.x);
      int mouse_y = (int)(scale_y * event->button.y);

      // Start dragging
      state->dragging = true;

      // Calculate which array index we're over and set the value
      int index;
      double value;
      screen_to_array_value(state, mouse_x, mouse_y, width, height, &index,
                            &value);

      // Make sure index is valid
      if (index >= 0 && index < state->size) {
        state->selected_index = index;
        state->data[index] = value;
      }
      return 1; // Event handled
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT && state->dragging) {
      state->dragging = false;
      return 1; // Event handled
    }
    break;

  case SDL_MOUSEMOTION:
    if (state->dragging) {
      // Scale mouse coordinates
      int mouse_x = (int)(scale_x * event->motion.x);
      int mouse_y = (int)(scale_y * event->motion.y);

      // Update whichever bar is currently under the mouse
      int index;
      double value;
      screen_to_array_value(state, mouse_x, mouse_y, width, height, &index,
                            &value);

      // Make sure index is valid
      if (index >= 0 && index < state->size) {
        state->selected_index = index; // Update selection to follow mouse
        state->data[index] = value;
      }
      return 1; // Event handled
    }
    break;

  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    case SDLK_ESCAPE:
      return 1; // Exit

    case SDLK_g:
      // Toggle grid
      state->display_grid = !state->display_grid;
      return 1; // Event handled
      break;

    case SDLK_LEFT:
      // Select previous point
      if (state->selected_index > 0) {
        state->selected_index--;
      }
      return 1; // Event handled
      break;

    case SDLK_RIGHT:
      // Select next point
      if (state->selected_index < state->size - 1) {
        state->selected_index++;
      }
      return 1; // Event handled
      break;

    case SDLK_UP:
      // Increase selected point value
      if (state->selected_index >= 0) {
        state->data[state->selected_index] += 0.05;
        if (state->data[state->selected_index] > state->value_max) {
          state->data[state->selected_index] = state->value_max;
        }
      }
      return 1; // Event handled
      break;

    case SDLK_DOWN:
      // Decrease selected point value
      if (state->selected_index >= 0) {
        state->data[state->selected_index] -= 0.05;
        if (state->data[state->selected_index] < state->value_min) {
          state->data[state->selected_index] = state->value_min;
        }
      }
      return 1; // Event handled
      break;

    case SDLK_r:
      // Reset all values to 0.5
      for (int i = 0; i < state->size; i++) {
        state->data[i] = 0.5;
      }
      return 1; // Event handled
      break;

    case SDLK_i:
      // Invert all values
      for (int i = 0; i < state->size; i++) {
        state->data[i] = state->value_max - (state->data[i] - state->value_min);
      }
      return 1; // Event handled
      break;

    case SDLK_s:
      // Smooth the array values (simple 3-point moving average)
      if (state->size > 2) {
        // Create a temporary copy of the data
        double *temp = malloc(state->size * sizeof(double));
        if (temp) {
          memcpy(temp, state->data, state->size * sizeof(double));

          // Apply smoothing (skip first and last points)
          for (int i = 1; i < state->size - 1; i++) {
            state->data[i] = (temp[i - 1] + temp[i] + temp[i + 1]) / 3.0;
          }

          free(temp);
        }
      }
      return 1; // Event handled
      break;
    }
    break;

  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
      // Window size changed
      state->last_width = width;
      state->last_height = height;
      return 1; // Event handled
    }
    break;
  }

  return 0; // Continue running
}

int create_custom_window(void *cb) { return create_window(NULL, cb, NULL); }
// // Add these includes at the top of your file
// #include <GL/glew.h>
// #include <SDL_opengl.h>
// #include <string.h>

// ==================== POINT CLOUD IMPLEMENTATION ====================

// Shader source code
static const char *pointcloud_vertex_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "uniform mat4 mvp;\n"
    "uniform float pointSize;\n"
    "out vec3 vertexColor;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(aPos, 1.0);\n"
    "    gl_PointSize = pointSize;\n"
    "    vertexColor = aColor;\n"
    "}\n";

static const char *pointcloud_fragment_shader_src =
    "#version 330 core\n"
    "in vec3 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    float dist = length(coord);\n"
    "    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);\n"
    "    FragColor = vec4(vertexColor, alpha);\n"
    "}\n";
// "    if (dist > 0.5) discard;\n"

// Point cloud state structure
typedef struct {
  // Point data
  double *points;
  int point_count;

  // OpenGL resources
  GLuint vao;
  GLuint vbo;
  GLuint shader_program;

  // Camera state
  float camera_distance;
  float camera_angle_x;
  float camera_angle_y;
  float camera_target[3];

  // Rendering options
  float point_size;
  int color_mode; // 0=height, 1=distance, 2=uniform

  // Bounds
  float bounds_min[3];
  float bounds_max[3];
  float bounds_center[3];
  float bounds_scale;

  // Mouse interaction
  bool mouse_dragging;
  int last_mouse_x;
  int last_mouse_y;

  // Update flag
  bool needs_buffer_update;
} PointCloudState;

// Matrix operations
static void mat4_identity(float *m) {
  memset(m, 0, 16 * sizeof(float));
  m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat4_multiply(float *result, const float *a, const float *b) {
  float temp[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp[i * 4 + j] = 0;
      for (int k = 0; k < 4; k++) {
        temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
      }
    }
  }
  memcpy(result, temp, 16 * sizeof(float));
}

static void mat4_perspective(float *m, float fovy, float aspect, float near,
                             float far) {
  float f = 1.0f / tanf(fovy * 0.5f);
  mat4_identity(m);
  m[0] = f / aspect;
  m[5] = f;
  m[10] = (far + near) / (near - far);
  m[11] = -1.0f;
  m[14] = (2.0f * far * near) / (near - far);
  m[15] = 0.0f;
}

static void mat4_lookat(float *m, float eye[3], float center[3], float up[3]) {
  float f[3], s[3], u[3];

  // Forward vector
  f[0] = center[0] - eye[0];
  f[1] = center[1] - eye[1];
  f[2] = center[2] - eye[2];
  float len = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
  f[0] /= len;
  f[1] /= len;
  f[2] /= len;

  // Side vector (right)
  s[0] = f[1] * up[2] - f[2] * up[1];
  s[1] = f[2] * up[0] - f[0] * up[2];
  s[2] = f[0] * up[1] - f[1] * up[0];
  len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
  s[0] /= len;
  s[1] /= len;
  s[2] /= len;

  // Up vector
  u[0] = s[1] * f[2] - s[2] * f[1];
  u[1] = s[2] * f[0] - s[0] * f[2];
  u[2] = s[0] * f[1] - s[1] * f[0];

  mat4_identity(m);
  m[0] = s[0];
  m[4] = s[1];
  m[8] = s[2];
  m[1] = u[0];
  m[5] = u[1];
  m[9] = u[2];
  m[2] = -f[0];
  m[6] = -f[1];
  m[10] = -f[2];
  m[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
  m[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
  m[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
}

// Shader compilation
static GLuint compile_shader(const char *source, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info[512];
    glGetShaderInfoLog(shader, 512, NULL, info);
    fprintf(stderr, "Shader compilation failed: %s\n", info);
    return 0;
  }
  return shader;
}

// Initialize OpenGL resources
static bool init_pointcloud_gl(void *_state) {
  PointCloudState *state;
  // Create and compile shaders
  GLuint vs = compile_shader(pointcloud_vertex_shader_src, GL_VERTEX_SHADER);
  GLuint fs =
      compile_shader(pointcloud_fragment_shader_src, GL_FRAGMENT_SHADER);

  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    return false;
  }

  // Create shader program
  state->shader_program = glCreateProgram();
  glAttachShader(state->shader_program, vs);
  glAttachShader(state->shader_program, fs);
  glLinkProgram(state->shader_program);

  // Check linking
  GLint success;
  glGetProgramiv(state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    char info[512];
    glGetProgramInfoLog(state->shader_program, 512, NULL, info);
    fprintf(stderr, "Shader linking failed: %s\n", info);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return false;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  // Create VAO and VBO
  glGenVertexArrays(1, &state->vao);
  glGenBuffers(1, &state->vbo);

  return true;
}

// Update GPU buffers
static void update_pointcloud_buffers(PointCloudState *state) {
  if (!state || state->point_count == 0)
    return;

  // Create vertex data: position (3 floats) + color (3 floats) per point
  float *vertex_data = malloc(state->point_count * 6 * sizeof(float));
  if (!vertex_data)
    return;

  for (int i = 0; i < state->point_count; i++) {
    int vertex_idx = i * 6;
    int point_idx = i * 3;

    // Position (normalized to bounds)
    vertex_data[vertex_idx + 0] =
        ((float)state->points[point_idx + 0] - state->bounds_center[0]) *
        state->bounds_scale;
    vertex_data[vertex_idx + 1] =
        ((float)state->points[point_idx + 1] - state->bounds_center[1]) *
        state->bounds_scale;
    vertex_data[vertex_idx + 2] =
        ((float)state->points[point_idx + 2] - state->bounds_center[2]) *
        state->bounds_scale;

    // Color based on mode
    if (state->color_mode == 0) {
      // Height-based coloring (Y coordinate)
      float normalized_y = (vertex_data[vertex_idx + 1] + 1.0f) * 0.5f;
      vertex_data[vertex_idx + 3] = 1.0f - normalized_y; // Red for low
      vertex_data[vertex_idx + 4] = normalized_y;        // Green for high
      vertex_data[vertex_idx + 5] = 0.5f;                // Blue constant
    } else if (state->color_mode == 1) {
      // Distance from center
      float dx = vertex_data[vertex_idx + 0];
      float dy = vertex_data[vertex_idx + 1];
      float dz = vertex_data[vertex_idx + 2];
      float dist =
          sqrtf(dx * dx + dy * dy + dz * dz) / 1.732f; // Normalize by sqrt(3)
      vertex_data[vertex_idx + 3] = dist;
      vertex_data[vertex_idx + 4] = 1.0f - dist;
      vertex_data[vertex_idx + 5] = 0.5f;
    } else {
      // Uniform color
      vertex_data[vertex_idx + 3] = 0.3f;
      vertex_data[vertex_idx + 4] = 0.7f;
      vertex_data[vertex_idx + 5] = 1.0f;
    }
  }

  // Upload to GPU
  glBindVertexArray(state->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
  glBufferData(GL_ARRAY_BUFFER, state->point_count * 6 * sizeof(float),
               vertex_data, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  free(vertex_data);
  state->needs_buffer_update = false;
}

// Create OpenGL window event (for thread safety)
Uint32 CREATE_OPENGL_WINDOW_EVENT;

// Initialize OpenGL window event (call this in init_gui)
void init_opengl_events() {
  CREATE_OPENGL_WINDOW_EVENT = SDL_RegisterEvents(1);
  if (CREATE_OPENGL_WINDOW_EVENT == (Uint32)-1) {
    fprintf(stderr, "Failed to register OpenGL window event\n");
  }
}

// Create OpenGL window (must be called from main thread)
bool _create_opengl_window(window_creation_data *data) {
  if (window_count >= MAX_WINDOWS) {
    fprintf(stderr, "Maximum number of windows reached.\n");
    return false;
  }

  int win_idx = window_count;
  window_count++;

  windows[win_idx].width = WINDOW_WIDTH;
  windows[win_idx].height = WINDOW_HEIGHT;
  windows[win_idx].render_fn = data->render_fn;
  windows[win_idx].handle_event = data->handle_event;
  windows[win_idx].data = data->data;

  // Set OpenGL attributes BEFORE creating the window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  // Create OpenGL window
  windows[win_idx].window = SDL_CreateWindow(
      "Point Cloud Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      windows[win_idx].width, windows[win_idx].height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!windows[win_idx].window) {
    fprintf(stderr, "Failed to create OpenGL window: %s\n", SDL_GetError());
    window_count--;
    return false;
  }

  // Create OpenGL context
  SDL_GLContext gl_context = SDL_GL_CreateContext(windows[win_idx].window);
  if (!gl_context) {
    fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
    SDL_DestroyWindow(windows[win_idx].window);
    window_count--;
    return false;
  }

  // Make context current
  SDL_GL_MakeCurrent(windows[win_idx].window, gl_context);

  // Initialize GLEW (only once)
  static bool glew_initialized = false;
  if (!glew_initialized) {
    if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      SDL_GL_DeleteContext(gl_context);
      SDL_DestroyWindow(windows[win_idx].window);
      window_count--;
      return false;
    }
    glew_initialized = true;
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
  }

  windows[win_idx].renderer = NULL; // No SDL renderer for OpenGL windows

  // Initialize OpenGL resources for the point cloud
  PointCloudState *state = (PointCloudState *)data->data;

  if (!data->init_gl(state)) {
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(windows[win_idx].window);
    window_count--;
    return false;
  }

  // Free the window creation data
  free(data);
  return true;
}

// Cleanup function (call when destroying window)
void cleanup_pointcloud(PointCloudState *state) {
  if (!state)
    return;

  if (state->vao)
    glDeleteVertexArrays(1, &state->vao);
  if (state->vbo)
    glDeleteBuffers(1, &state->vbo);
  if (state->shader_program)
    glDeleteProgram(state->shader_program);

  free(state);
}
// Calculate bounds with better diagnostics
static void calculate_pointcloud_bounds(PointCloudState *state) {
  if (state->point_count == 0)
    return;

  // Initialize with first point
  for (int i = 0; i < 3; i++) {
    state->bounds_min[i] = state->bounds_max[i] = (float)state->points[i];
  }

  // Find min/max
  for (int i = 1; i < state->point_count; i++) {
    for (int j = 0; j < 3; j++) {
      float val = (float)state->points[i * 3 + j];
      if (val < state->bounds_min[j])
        state->bounds_min[j] = val;
      if (val > state->bounds_max[j])
        state->bounds_max[j] = val;
    }
  }

  // Calculate center and scale
  float max_range = 0;
  for (int i = 0; i < 3; i++) {
    state->bounds_center[i] =
        (state->bounds_min[i] + state->bounds_max[i]) * 0.5f;
    float range = state->bounds_max[i] - state->bounds_min[i];
    if (range > max_range)
      max_range = range;
  }

  // Print bounds for debugging
  printf("Point cloud bounds:\n");
  printf("  X: [%.3f, %.3f] center: %.3f\n", state->bounds_min[0],
         state->bounds_max[0], state->bounds_center[0]);
  printf("  Y: [%.3f, %.3f] center: %.3f\n", state->bounds_min[1],
         state->bounds_max[1], state->bounds_center[1]);
  printf("  Z: [%.3f, %.3f] center: %.3f\n", state->bounds_min[2],
         state->bounds_max[2], state->bounds_center[2]);
  printf("  Max range: %.3f\n", max_range);

  state->bounds_scale = max_range > 0 ? 2.0f / max_range : 1.0f;
}

// Update camera controls for better navigation
static void pointcloud_event_handler(void *userdata, SDL_Event *event) {
  PointCloudState *state = (PointCloudState *)userdata;
  if (!state)
    return;

  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      state->mouse_dragging = true;
      state->last_mouse_x = event->button.x;
      state->last_mouse_y = event->button.y;
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_LEFT) {
      state->mouse_dragging = false;
    }
    break;

  case SDL_MOUSEMOTION:
    if (state->mouse_dragging) {
      int dx = event->motion.x - state->last_mouse_x;
      int dy = event->motion.y - state->last_mouse_y;

      state->camera_angle_y += dx * 0.01f;
      state->camera_angle_x += dy * 0.01f;

      state->last_mouse_x = event->motion.x;
      state->last_mouse_y = event->motion.y;
    }
    break;

  case SDL_MOUSEWHEEL: {
    // More responsive zooming
    float zoom_factor = 1.0f + (event->wheel.y * 0.1f);
    state->camera_distance *= zoom_factor;
    if (state->camera_distance < 0.1f)
      state->camera_distance = 0.1f;
    if (state->camera_distance > 100.0f)
      state->camera_distance = 100.0f;
    break;
  }

  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    // Camera rotation
    case SDLK_w:
      state->camera_angle_x += 0.1f;
      break;
    case SDLK_s:
      state->camera_angle_x -= 0.1f;
      break;
    case SDLK_a:
      state->camera_angle_y -= 0.1f;
      break;
    case SDLK_d:
      state->camera_angle_y += 0.1f;
      break;

    // Camera distance (zoom)
    case SDLK_UP:
    case SDLK_q:
      state->camera_distance *= 0.9f;
      if (state->camera_distance < 0.1f)
        state->camera_distance = 0.1f;
      break;
    case SDLK_DOWN:
    case SDLK_e:
      state->camera_distance *= 1.1f;
      if (state->camera_distance > 100.0f)
        state->camera_distance = 100.0f;
      break;

    // Camera target movement
    case SDLK_LEFT:
      state->camera_target[0] -= 0.1f / state->bounds_scale;
      break;
    case SDLK_RIGHT:
      state->camera_target[0] += 0.1f / state->bounds_scale;
      break;
    case SDLK_PAGEUP:
      state->camera_target[1] += 0.1f / state->bounds_scale;
      break;
    case SDLK_PAGEDOWN:
      state->camera_target[1] -= 0.1f / state->bounds_scale;
      break;

    // Color modes
    case SDLK_1:
      state->color_mode = 0;
      state->needs_buffer_update = true;
      break;
    case SDLK_2:
      state->color_mode = 1;
      state->needs_buffer_update = true;
      break;
    case SDLK_3:
      state->color_mode = 2;
      state->needs_buffer_update = true;
      break;

    // Point size
    case SDLK_PLUS:
    case SDLK_EQUALS:
      state->point_size += 0.5f;
      if (state->point_size > 20.0f)
        state->point_size = 20.0f;
      printf("Point size: %.1f\n", state->point_size);
      break;
    case SDLK_MINUS:
      state->point_size -= 0.5f;
      if (state->point_size < 0.5f)
        state->point_size = 0.5f;
      printf("Point size: %.1f\n", state->point_size);
      break;

    // Reset camera
    case SDLK_r:
      state->camera_angle_x = 0.3f;
      state->camera_angle_y = 0.0f;
      state->camera_distance = 3.0f; // Start farther back
      memcpy(state->camera_target, state->bounds_center, 3 * sizeof(float));
      printf("Camera reset\n");
      break;

    // Print camera info
    case SDLK_i:
      printf("Camera info:\n");
      printf("  Distance: %.2f\n", state->camera_distance);
      printf("  Angles: (%.2f, %.2f)\n", state->camera_angle_x,
             state->camera_angle_y);
      printf("  Target: (%.2f, %.2f, %.2f)\n", state->camera_target[0],
             state->camera_target[1], state->camera_target[2]);
      printf("  Point count: %d\n", state->point_count);
      break;
    }
    break;
  }
}

// Update renderer with better perspective
static SDL_Renderer *pointcloud_renderer(void *userdata,
                                         SDL_Renderer *renderer) {
  PointCloudState *state = (PointCloudState *)userdata;
  if (!state || state->point_count == 0)
    return renderer;

  // Get window size
  int width, height;
  SDL_Window *window = SDL_GL_GetCurrentWindow();
  if (!window)
    return renderer;
  SDL_GL_GetDrawableSize(window, &width, &height);

  // Update buffers if needed
  if (state->needs_buffer_update) {
    update_pointcloud_buffers(state);
  }

  // Clear screen
  glViewport(0, 0, width, height);
  glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Darker background
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Enable depth testing and point rendering
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Calculate camera position
  float cam_x = state->camera_target[0] + state->camera_distance *
                                              cosf(state->camera_angle_x) *
                                              cosf(state->camera_angle_y);
  float cam_y = state->camera_target[1] +
                state->camera_distance * sinf(state->camera_angle_x);
  float cam_z = state->camera_target[2] + state->camera_distance *
                                              cosf(state->camera_angle_x) *
                                              sinf(state->camera_angle_y);

  float eye[3] = {cam_x, cam_y, cam_z};
  float up[3] = {0, 1, 0};

  // Create matrices with wider field of view
  float projection[16], view[16], mvp[16];
  float aspect = (float)width / (float)height;

  // Wider FOV (60 degrees instead of 45)
  mat4_perspective(projection, M_PI * 0.333f, aspect, 0.01f, 1000.0f);
  mat4_lookat(view, eye, state->camera_target, up);
  mat4_multiply(mvp, projection, view);

  // Use shader and set uniforms
  glUseProgram(state->shader_program);

  GLint mvp_loc = glGetUniformLocation(state->shader_program, "mvp");
  GLint point_size_loc =
      glGetUniformLocation(state->shader_program, "pointSize");

  glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp);
  glUniform1f(point_size_loc, state->point_size);

  // Render points
  glBindVertexArray(state->vao);
  glDrawArrays(GL_POINTS, 0, state->point_count);
  glBindVertexArray(0);

  glUseProgram(0);

  return renderer;
}

// Main function with better initialization
int create_pointcloud_window(double *points, int size) {
  if (!points || size <= 0) {
    fprintf(stderr, "Invalid point cloud data\n");
    return -1;
  }

  // Allocate state
  PointCloudState *state = calloc(1, sizeof(PointCloudState));
  if (!state) {
    fprintf(stderr, "Failed to allocate point cloud state\n");
    return -1;
  }

  // Initialize state
  state->points = points;
  state->point_count = size;
  state->point_size = 1.0f; // Larger default point size
  state->color_mode = 0;
  state->camera_distance = 3.0f; // Start farther back
  state->camera_angle_x = 0.3f;
  state->camera_angle_y = 0.785f; // 45 degrees
  state->mouse_dragging = false;
  state->needs_buffer_update = true;

  // Calculate bounds
  calculate_pointcloud_bounds(state);

  // Set camera target to center of point cloud
  memcpy(state->camera_target, state->bounds_center, 3 * sizeof(float));

  // Create window creation data
  window_creation_data *data = malloc(sizeof(window_creation_data));
  if (!data) {
    free(state);
    return -1;
  }

  data->render_fn = pointcloud_renderer;
  data->handle_event = pointcloud_event_handler;
  data->data = state;
  data->init_gl = init_pointcloud_gl;

  // Push an event to create the OpenGL window on the main thread
  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_OPENGL_WINDOW_EVENT;
  event.user.data1 = data;

  printf("Creating point cloud window with %d points\n", size);
  printf("Controls:\n");
  printf("  Mouse drag: Rotate camera\n");
  printf("  Mouse wheel or Q/E: Zoom in/out\n");
  printf("  W/A/S/D: Rotate camera\n");
  printf("  Arrow keys: Move camera target\n");
  printf("  +/-: Adjust point size\n");
  printf("  1/2/3: Change color mode\n");
  printf("  R: Reset camera\n");
  printf("  I: Print camera info\n");

  return SDL_PushEvent(&event);
}

// Initialize OpenGL resources
static bool init_opengl_win(void *_state) {

  // // Create shader program
  // state->shader_program = glCreateProgram();
  // glAttachShader(state->shader_program, vs);
  // glAttachShader(state->shader_program, fs);
  // glLinkProgram(state->shader_program);

  // Check linking
  GLint success;
  // glGetProgramiv(state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    char info[512];
    // glGetProgramInfoLog(state->shader_program, 512, NULL, info);
    fprintf(stderr, "Shader linking failed: %s\n", info);
    // glDeleteShader(vs);
    // glDeleteShader(fs);
    return false;
  }

  // glDeleteShader(vs);
  // glDeleteShader(fs);

  // Create VAO and VBO
  // glGenVertexArrays(1, &state->vao);
  // glGenBuffers(1, &state->vbo);

  return true;
}
int create_opengl_window() {

  // Create window creation data
  window_creation_data *data = malloc(sizeof(window_creation_data));
  data->init_gl = init_opengl_win;

  // Push an event to create the OpenGL window on the main thread
  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_OPENGL_WINDOW_EVENT;
  event.user.data1 = data;

  return SDL_PushEvent(&event);
}
