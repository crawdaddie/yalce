#include "./common.h"
#include "gui.h"
#include <stdbool.h>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct array_edit_state {
  double *data;       // Pointer to the array data
  int size;           // Number of elements in the array
  int selected_index; // Currently selected/active index
  bool dragging;      // Whether the user is currently dragging
  int last_width;     // Last known window width
  int last_height;    // Last known window height
  double value_min;   // Fixed minimum value for the range
  double value_max;   // Fixed maximum value for the range
  bool display_grid;  // Whether to display grid lines
} array_edit_state;

double clamp(double min, double max, double val) {
  if (val > max) {
    return max;
  }
  if (val < min) {
    return min;
  }
  return val;
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

  // Calculate y based on value using fixed range
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

  // Calculate value from y coordinate using fixed range
  double y_ratio = (double)(height - margin - y) / plot_height;
  *value =
      clamp(state->value_min, state->value_max,
            state->value_min + y_ratio * (state->value_max - state->value_min));

  // Don't clamp to allow extending the range if user drags beyond
}

// Renderer function for the array editor
static SDL_Renderer *array_edit_renderer(array_edit_state *state,
                                         SDL_Renderer *renderer) {
  if (!state || !renderer)
    return renderer;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  if (width != state->last_width || height != state->last_height) {
    state->last_width = width;
    state->last_height = height;
  }

  SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light grey background
  SDL_RenderClear(renderer);

  const int margin = 40;
  const int plot_width = width - 2 * margin;
  const int plot_height = height - 2 * margin;
  SDL_Rect plot_area = {margin, margin, plot_width, plot_height};

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
  SDL_RenderFillRect(renderer, &plot_area);

  if (state->display_grid) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light grey grid

    for (int i = 0; i <= 10; i++) {
      int x = margin + (i * plot_width) / 10;
      SDL_RenderDrawLine(renderer, x, margin, x, height - margin);
    }

    for (int i = 0; i <= 10; i++) {
      int y = margin + (i * plot_height) / 10;
      SDL_RenderDrawLine(renderer, margin, y, width - margin, y);
    }
  }

  SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
  SDL_RenderDrawRect(renderer, &plot_area);

  for (int i = 0; i < state->size; i++) {
    double bar_width = (double)plot_width / state->size;
    int x = margin + (int)(i * bar_width);
    int bar_width_pixels = (int)bar_width - 2; // Slight gap between bars
    if (bar_width_pixels < 2)
      bar_width_pixels = 2; // Minimum width

    double value = state->data[i];

    // Calculate normalized value - clamp to 0-1 for display
    double normalized_value =
        (value - state->value_min) / (state->value_max - state->value_min);
    if (normalized_value < 0.0)
      normalized_value = 0.0;
    if (normalized_value > 1.0)
      normalized_value = 1.0;

    int bar_height = (int)(normalized_value * plot_height);

    SDL_Rect bar = {x + 1, // +1 to create slight separation
                    height - margin - bar_height, bar_width_pixels, bar_height};

    // Different color for selected bar
    if (i == state->selected_index) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for selected
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255); // Blue for others
    }

    SDL_RenderFillRect(renderer, &bar);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &bar);
  }

  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); // Dark line

  for (int i = 0; i < state->size - 1; i++) {
    SDL_Point p1 =
        array_point_to_screen(state, i, state->data[i], width, height);
    SDL_Point p2 =
        array_point_to_screen(state, i + 1, state->data[i + 1], width, height);
    SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
  }

  const int point_radius = 5;
  for (int i = 0; i < state->size; i++) {
    SDL_Point p =
        array_point_to_screen(state, i, state->data[i], width, height);

    if (i == state->selected_index) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for selected
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for others
    }

    SDL_Rect point_rect = {p.x - point_radius / 2, p.y - point_radius / 2,
                           point_radius, point_radius};
    SDL_RenderFillRect(renderer, &point_rect);
  }

  if (state->selected_index >= 0) {
    char info[64];
    sprintf(info, "Index: %d  Value: %.3f", state->selected_index,
            state->data[state->selected_index]);

    SDL_Color text_color = {0, 0, 0, 255}; // Black text
    render_text(info, margin, 10, renderer, text_color);
  }

  // Display the fixed range being shown
  char range_info[64];
  sprintf(range_info, "Range: %.3f to %.3f", state->value_min,
          state->value_max);
  SDL_Color range_color = {0, 0, 0, 255}; // Black text
  render_text(range_info, margin, height - 30, renderer, range_color);

  SDL_Color title_color = {0, 0, 0, 255}; // Black text
  render_text("Array Editor", width / 2 - 40, 10, renderer, title_color);

  return renderer;
}

// Event handler using fixed range
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

  // Use fixed range for value adjustments
  double range = state->value_max - state->value_min;

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
      // Increase selected point value (use 5% of fixed range)
      if (state->selected_index >= 0) {
        double increment = range * 0.05;
        // state->data[state->selected_index] += increment;

        state->data[state->selected_index] =
            clamp(state->value_min, state->value_max,
                  state->data[state->selected_index] + increment);
      }
      return 1; // Event handled
      break;

    case SDLK_DOWN:
      // Decrease selected point value (use 5% of fixed range)
      if (state->selected_index >= 0) {
        double decrement = range * 0.05;
        state->data[state->selected_index] =
            clamp(state->value_min, state->value_max,
                  state->data[state->selected_index] - decrement);
      }
      return 1; // Event handled
      break;

    case SDLK_r:
      // Reset all values to middle of fixed range
      if (state->size > 0) {
        double middle = (state->value_min + state->value_max) / 2.0;
        for (int i = 0; i < state->size; i++) {
          state->data[i] = middle;
        }
      }
      return 1; // Event handled
      break;

    case SDLK_i:
      // Invert all values relative to fixed range
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

/**
 * Create an interactive array editor
 *
 * @param size Number of elements in the array
 * @param data Pointer to the array of doubles
 * @param min_value Minimum value for the display range
 * @param max_value Maximum value for the display range
 * @return 0 on success, -1 on failure
 */
int create_array_editor(int size, double *data, double min_value,
                        double max_value) {
  // Allocate state
  array_edit_state *state = malloc(sizeof(array_edit_state));

  if (!state) {
    fprintf(stderr, "Failed to allocate array editor state\n");
    return -1;
  }

  // Validate range parameters
  if (max_value <= min_value) {
    fprintf(stderr,
            "Invalid range: max_value must be greater than min_value\n");
    free(state);
    return -1;
  }

  // Initialize state
  state->data = data;
  state->size = size;
  state->selected_index = -1;
  state->dragging = false;
  state->last_width = 0;
  state->last_height = 0;
  state->value_min = min_value; // Use provided minimum value
  state->value_max = max_value; // Use provided maximum value
  state->display_grid = true;

  return create_window(state, array_edit_renderer, array_edit_event_handler);
}
