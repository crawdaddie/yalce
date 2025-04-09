#include "gui.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

TTF_Font *DEFAULT_FONT;

typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(void *window, SDL_Renderer *renderer);

typedef struct window_creation_data {
  void *handle_event;
  void *render_fn;
  void *data;
} window_creation_data;

bool _create_window(window_creation_data *data);

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
int init_gui() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

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

  return 0;
}

void handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == CREATE_WINDOW_EVENT) {
      // handle window creation
      //
      _create_window(event.user.data1);
    } else {

      for (int i = 0; i < window_count; i++) {
        if (SDL_GetWindowID(windows[i].window) == event.window.windowID &&
            windows[i].handle_event) {
          if (event.type == SDL_WINDOWEVENT &&
              event.window.event == SDL_WINDOWEVENT_CLOSE) {

            SDL_DestroyRenderer(windows[i].renderer);
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

void render_window(Window *window) {
  window->render_fn(window->data, window->renderer);

  for (int i = 0; i < window->num_children; i++) {
    render_window(window->children + i);
  }
}

int gui_loop() {
  while (true) {

    handle_events();

    for (int i = 0; i < window_count; i++) {

      if (windows[i].render_fn) {
        SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
        SDL_RenderClear(windows[i].renderer);

        windows[i].render_fn(windows[i].data, windows[i].renderer);
        SDL_RenderPresent(windows[i].renderer);
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

  windows[win_idx].window = SDL_CreateWindow(
      "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      windows[win_idx].width, windows[win_idx].height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  windows[win_idx].renderer =
      SDL_CreateRenderer(windows[win_idx].window, -1, SDL_RENDERER_ACCELERATED);

  // free(data);
  return true;
}

// Function to push a create window event to the SDL event queue
int create_window(void *data, void *renderer, void *event_handler
                  // ,
                  // int num_children, void *children
) {

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
#define BACKGROUND_COLOR 0xFF000000

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

  printf("create static plot %d %d\n", layout, size);
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
      0xFF0000FF, // Red
      0xFF00FF00, // Green
      0xFFFF0000, // Blue
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
 * Create the plot texture (only called when plot needs to be redrawn)
 */
static void create_plot_texture_overlapped(plot_state *state,
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

  int plot_x = PLOT_PADDING;
  int plot_y = PLOT_PADDING;
  int plot_width = state->window_width - 2 * PLOT_PADDING;
  int plot_height = state->window_height - 2 * PLOT_PADDING;

  if (state->draw_grid) {
    SDL_SetRenderDrawColor(renderer, (GRID_COLOR >> 16) & 0xFF,
                           (GRID_COLOR >> 8) & 0xFF, GRID_COLOR & 0xFF, 255);

    for (int i = 0; i <= 10; i++) {
      int x = plot_x + (i * plot_width) / 10;
      SDL_RenderDrawLine(renderer, x, plot_y, x, plot_y + plot_height);
    }

    for (int i = 0; i <= 10; i++) {
      int y = plot_y + (i * plot_height) / 10;
      SDL_RenderDrawLine(renderer, plot_x, y, plot_x + plot_width, y);
    }
  }

  // Draw axes if enabled
  if (state->draw_axis) {
    SDL_SetRenderDrawColor(renderer, (AXIS_COLOR >> 16) & 0xFF,
                           (AXIS_COLOR >> 8) & 0xFF, AXIS_COLOR & 0xFF, 255);

    // X-axis
    SDL_RenderDrawLine(renderer, plot_x, plot_y + plot_height,
                       plot_x + plot_width, plot_y + plot_height);

    // Y-axis
    SDL_RenderDrawLine(renderer, plot_x, plot_y, plot_x, plot_y + plot_height);
  }

  if (state->buf && state->size > 0) {
    for (int ch = 0; ch < state->layout; ch++) {
      uint32_t color = state->channel_colors[ch % 8];
      SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF,
                             (color >> 8) & 0xFF, color & 0xFF, 100);

      for (int i = 0; i < state->size - 1; i++) {
        double val1 = state->buf[i * state->layout + ch];
        double val2 = state->buf[(i + 1) * state->layout + ch];

        val1 *= state->vertical_scale;
        val2 *= state->vertical_scale;

        // Scale to fit plot area
        double range = state->y_max - state->y_min;
        double normalized1 = (val1 - state->y_min) / range;
        double normalized2 = (val2 - state->y_min) / range;

        // Apply horizontal scale (adjust the spacing)
        int effective_width = (int)(plot_width * state->horizontal_scale);
        int offset_x = (plot_width - effective_width) / 2;

        int x1 = plot_x + offset_x + (i * effective_width) / (state->size - 1);
        int y1 = plot_y + plot_height - (int)(normalized1 * plot_height);

        int x2 =
            plot_x + offset_x + ((i + 1) * effective_width) / (state->size - 1);
        int y2 = plot_y + plot_height - (int)(normalized2 * plot_height);

        // Ensure points are in bounds
        if (x1 >= plot_x && x1 < plot_x + plot_width && x2 >= plot_x &&
            x2 < plot_x + plot_width && y1 >= plot_y &&
            y1 < plot_y + plot_height && y2 >= plot_y &&
            y2 < plot_y + plot_height) {
          SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
      }
    }
  }

  SDL_SetRenderTarget(renderer, NULL);

  state->needs_redraw = false;
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
      double start_pos = center_offset - (visible_range / 2.0);
      double end_pos = center_offset + (visible_range / 2.0);

      // Clamp to valid range
      if (start_pos < 0.0)
        start_pos = 0.0;
      if (end_pos > 1.0)
        end_pos = 1.0;

      // Convert to sample indices
      int start_sample = (int)(start_pos * (state->size - 1));
      int end_sample = (int)(end_pos * (state->size - 1));

      // Ensure we have at least one sample to display
      if (start_sample == end_sample && start_sample < state->size - 1)
        end_sample++;
      if (start_sample == end_sample && start_sample > 0)
        start_sample--;

      // Draw only the visible samples
      for (int i = start_sample; i < end_sample; i++) {
        // Get sample values
        double val1 = state->buf[i * state->layout + ch];
        double val2 = state->buf[(i + 1) * state->layout + ch];

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
