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
          windows[i].handle_event(windows[i].data, &event);
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

  free(data);
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
                          SDL_Renderer *renderer) {
  SDL_Color text_color = {0, 0, 0, 255}; // black

  SDL_Surface *surface = TTF_RenderText_Blended(DEFAULT_FONT, text, text_color);

  // Create a texture from the surface
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect = {x, y, surface->w, surface->h};

  SDL_RenderCopy(renderer, texture, NULL, &rect);

  // Clean up
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

  // Update the ring buffer position (in frames)
  state->ring_buffer_pos =
      (state->ring_buffer_pos + state->size) % state->ring_buffer_size;
}

// Find a trigger point for stable waveform display
bool find_trigger_point(scope_state *state, int *trigger_index) {
  if (!state || !state->ring_buffer ||
      state->trigger_channel >= state->layout) {
    *trigger_index = 0;
    return false;
  }

  // Use the first channel for triggering by default, unless specified
  int channel = state->trigger_channel;
  if (channel < 0 || channel >= state->layout)
    channel = 0;

  // Look for a rising edge crossing the trigger level
  // Need to scan through the ring buffer looking at only the trigger channel
  for (int i = state->size; i < state->ring_buffer_size - 1; i++) {
    int frame_idx = (state->ring_buffer_pos - i + state->ring_buffer_size) %
                    state->ring_buffer_size;
    int next_frame_idx = (frame_idx + 1) % state->ring_buffer_size;

    // Get the sample values for this channel at this frame and next frame
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

// Draw the oscilloscope grid
void draw_grid(SDL_Renderer *renderer, int width, int height) {
  // Set grid color (dark gray)
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

  // Draw horizontal lines
  for (int i = 0; i <= 8; i++) {
    int y = (height * i) / 8;
    SDL_RenderDrawLine(renderer, 0, y, width, y);
  }

  // Draw vertical lines
  for (int i = 0; i <= 10; i++) {
    int x = (width * i) / 10;
    SDL_RenderDrawLine(renderer, x, 0, x, height);
  }

  // Draw center lines with slightly brighter color
  SDL_SetRenderDrawColor(renderer, 75, 75, 75, 255);
  SDL_RenderDrawLine(renderer, 0, height / 2, width,
                     height / 2); // Horizontal center
  SDL_RenderDrawLine(renderer, width / 2, 0, width / 2,
                     height); // Vertical center
}

SDL_Renderer *scope_renderer(scope_state *state, SDL_Renderer *renderer) {
  if (!state || !renderer || !state->ring_buffer)
    return renderer;

  // Get window dimensions
  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  // Handle window resize if needed
  if (width != state->last_width || height != state->last_height) {
    state->last_width = width;
    state->last_height = height;
  }

  // Clear the renderer with a dark background
  SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
  SDL_RenderClear(renderer);

  // Append the new data to the ring buffer
  append_to_ring_buffer(state);

  // Draw grid if enabled
  if (state->draw_grid) {
    draw_grid(renderer, width, height);
  }

  // Find trigger point for stable waveform
  int trigger_frame = 0;
  bool triggered = find_trigger_point(state, &trigger_frame);

  // If we couldn't find a trigger point and we're not in auto mode, don't
  // update
  if (!triggered && state->trigger_mode != 0) {
    return renderer;
  }

  // Calculate the height for each channel
  int channel_height = height / state->layout;

  // Draw each channel
  for (int ch = 0; ch < state->layout && ch < MAX_CHANNELS; ch++) {
    // Calculate the vertical center for this channel
    int center_y = (ch * channel_height) + (channel_height / 2);
    int half_height = channel_height / 2;

    // Set the color for this channel
    SDL_Color color = CHANNEL_COLORS[ch % MAX_CHANNELS];
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw the channel label
    char label[20];
    sprintf(label, "CH%d", ch + 1);
    render_text(label, 10, center_y - half_height + 5, renderer);

    // Draw the waveform with the specified thickness
    for (int thickness = 0; thickness < LINE_THICKNESS; thickness++) {
      // Get the first sample
      double sample = state->ring_buffer[trigger_frame * state->layout + ch];

      int prev_x = 0;
      int prev_y =
          center_y - (int)(sample * half_height * 0.8 * state->vertical_scale);

      for (int x = 1; x < width; x++) {
        // Calculate frame index in the ring buffer based on x position and
        // scaling
        int frame_idx = (trigger_frame + (int)(x / state->horizontal_scale)) %
                        state->ring_buffer_size;

        // Get the sample for this channel at this frame
        sample = state->ring_buffer[frame_idx * state->layout + ch];

        // Calculate y position based on the sample value (normalized to -1.0
        // to 1.0) Scale by 80% of half the channel height and flip (negative is
        // up in SDL)
        int y = center_y -
                (int)(sample * half_height * 0.8 * state->vertical_scale);

        // Clip to channel boundaries
        if (y < ch * channel_height)
          y = ch * channel_height;
        if (y > (ch + 1) * channel_height)
          y = (ch + 1) * channel_height;

        // Draw line segment
        SDL_RenderDrawLine(renderer, prev_x, prev_y + thickness, x,
                           y + thickness);
        prev_x = x;
        prev_y = y;
      }
    }

    // Draw horizontal dividing lines between channels
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawLine(renderer, 0, (ch + 1) * channel_height, width,
                       (ch + 1) * channel_height);
  }

  // Draw trigger information
  if (state->trigger_mode != 0) {
    // Draw trigger level indicator on the trigger channel
    int ch = state->trigger_channel;
    if (ch >= 0 && ch < state->layout) {
      int center_y = (ch * channel_height) + (channel_height / 2);
      int trigger_y = center_y - (int)(state->trigger_level * channel_height /
                                       2 * 0.8 * state->vertical_scale);

      SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange
      SDL_RenderDrawLine(renderer, 0, trigger_y, 20, trigger_y);
    }
  }

  // Draw scale information
  char info[50];
  sprintf(info, "V: %.1fx  H: %.1fx  Trig: %s", state->vertical_scale,
          state->horizontal_scale,
          state->trigger_mode == 0   ? "Auto"
          : state->trigger_mode == 1 ? "Normal"
                                     : "Single");
  render_text(info, width - 200, 10, renderer);

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

  // Size in samples: buffer_size (frames) * layout (channels per frame)
  state->ring_buffer = (double *)(state + 1);
  if (!state->ring_buffer) {
    fprintf(stderr, "Failed to allocate ring buffer\n");
    free(state);
    return -1;
  }

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
