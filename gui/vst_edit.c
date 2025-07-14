#include "./vst_edit.h"
#include <SDL2/SDL.h>
#include <VSTPlugin.h>

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
