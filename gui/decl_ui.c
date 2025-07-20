#include "./decl_ui.h"
#include "./common.h"
#include <stdlib.h>

typedef SDL_Renderer *(*UIObjRendererFn)(void *state, SDL_Renderer *renderer);

typedef int (*UIObjEventHandler)(void *userdata, SDL_Event *event);
typedef struct {
  void *data;
  UIObjRendererFn render_cb;
  UIObjEventHandler event_handler;
  struct UIObj *next;
  SDL_Rect viewport_rect;
  double bounds[4];
} UIObj;

static UIObj *_dcl_ctx_head = NULL;
static UIObj *_dcl_ctx_tail = NULL;

typedef struct {
  UIObj *head;
  UIObj *tail;
} DeclUIState;

static DeclUIState *_decl_ui_ctx = NULL;

static UIObj *append_obj(DeclUIState *ctx, UIObj obj) {
  if (ctx->head == NULL) {
    UIObj *head = malloc(sizeof(UIObj));
    *head = obj;
    ctx->head = head;
    ctx->tail = head;
    return head;
  }

  UIObj *tail = malloc(sizeof(UIObj));
  *tail = obj;
  ctx->tail->next = tail;
  ctx->tail = tail;
  return tail;
}

void decl_ui_render_fn(DeclUIState *state, SDL_Renderer *renderer) {
  UIObj *o = state->head;

  SDL_Rect viewport_rect = {};

  SDL_RenderGetViewport(renderer, &viewport_rect);
  viewport_rect.x = 0;
  viewport_rect.y = 0;
  SDL_RenderSetViewport(renderer, &viewport_rect);

  while (o) {
    int height = o->bounds[0];
    int width = o->bounds[1];

    // printf("initial vp x: %d y: %d w: %d h: %d\n", viewport_rect.x,
    //        viewport_rect.y, viewport_rect.w, viewport_rect.h);

    o->render_cb(o->data, renderer);

    viewport_rect.w = width;
    viewport_rect.h = height;
    o->viewport_rect = viewport_rect;
    viewport_rect.y += height + 10;
    SDL_RenderSetViewport(renderer, &viewport_rect);
    o = (UIObj *)o->next;
  }
}

void __decl_ui_event_handler(DeclUIState *state, SDL_Event *event) {
  if (!state || !event)
    return;

  // Only handle mouse events for now
  if (event->type != SDL_MOUSEBUTTONDOWN && event->type != SDL_MOUSEBUTTONUP &&
      event->type != SDL_MOUSEMOTION) {
    return;
  }

  int mouse_x, mouse_y;
  if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    mouse_x = event->button.x;
    mouse_y = event->button.y;
  } else if (event->type == SDL_MOUSEMOTION) {
    mouse_x = event->motion.x;
    mouse_y = event->motion.y;
  }

  UIObj *o = state->head;
  while (o) {
    // Check if mouse is within this object's viewport
    if (mouse_x >= o->viewport_rect.x &&
        mouse_x < o->viewport_rect.x + o->viewport_rect.w &&
        mouse_y >= o->viewport_rect.y &&
        mouse_y < o->viewport_rect.y + o->viewport_rect.h) {

      // If this object has an event handler, call it
      if (o->event_handler) {
        // Translate mouse coordinates to be relative to the viewport
        SDL_Event local_event = *event;
        if (event->type == SDL_MOUSEBUTTONDOWN ||
            event->type == SDL_MOUSEBUTTONUP) {
          local_event.button.x = mouse_x - o->viewport_rect.x;
          local_event.button.y = mouse_y - o->viewport_rect.y;
        } else if (event->type == SDL_MOUSEMOTION) {
          local_event.motion.x = mouse_x - o->viewport_rect.x;
          local_event.motion.y = mouse_y - o->viewport_rect.y;
        }

        // Call the object's event handler with local coordinates
        int handled = o->event_handler(o->data, &local_event);
        if (handled) {
          return; // Event was handled, don't process further
        }
      }
    }
    o = o->next;
  }
}
void handle_dpi_mouse_coords(SDL_Event *event, int *mouse_x, int *mouse_y) {

  SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
  if (window) {
    int window_w, window_h, drawable_w, drawable_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_GL_GetDrawableSize(window, &drawable_w, &drawable_h);

    double scale_x = (double)drawable_w / window_w;
    double scale_y = (double)drawable_h / window_h;

    *mouse_x = (int)(*mouse_x * scale_x);
    *mouse_y = (int)(*mouse_y * scale_y);
  }
}
void decl_ui_event_handler(DeclUIState *state, SDL_Event *event) {
  if (!state || !event)
    return;

  if (event->type != SDL_MOUSEBUTTONDOWN && event->type != SDL_MOUSEBUTTONUP &&
      event->type != SDL_MOUSEMOTION) {
    return;
  }

  int mouse_x, mouse_y;
  if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    mouse_x = event->button.x;
    mouse_y = event->button.y;
  } else if (event->type == SDL_MOUSEMOTION) {
    mouse_x = event->motion.x;
    mouse_y = event->motion.y;
  }

  handle_dpi_mouse_coords(event, &mouse_x, &mouse_y);

  UIObj *o = state->head;
  while (o) {

    if (mouse_x >= o->viewport_rect.x &&
        mouse_x < o->viewport_rect.x + o->viewport_rect.w &&
        mouse_y >= o->viewport_rect.y &&
        mouse_y < o->viewport_rect.y + o->viewport_rect.h) {

      if (o->event_handler) {
        SDL_Event local_event = *event;
        int local_x = mouse_x - o->viewport_rect.x;
        int local_y = mouse_y - o->viewport_rect.y;

        if (event->type == SDL_MOUSEBUTTONDOWN ||
            event->type == SDL_MOUSEBUTTONUP) {
          local_event.button.x = local_x;
          local_event.button.y = local_y;
        } else if (event->type == SDL_MOUSEMOTION) {
          local_event.motion.x = local_x;
          local_event.motion.y = local_y;
        }

        int handled = o->event_handler(o->data, &local_event);
        if (handled) {
          return;
        }
      }
    }
    o = o->next;
  }
}

typedef void (*DeclUIInitFn)();

int create_decl_ui(void *cb) {

  DeclUIState *state = calloc(1, sizeof(DeclUIState));
  _decl_ui_ctx = state;

  ((DeclUIInitFn)cb)();

  window_creation_data *data = malloc(sizeof(window_creation_data));

  data->data = state;
  data->handle_event = decl_ui_event_handler;
  data->render_fn = decl_ui_render_fn;

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_WINDOW_EVENT;
  event.user.data1 = data;

  return SDL_PushEvent(&event);
}

typedef struct {
  double x_min, x_max;
  double y_min, y_max;
  bool auto_bounds;

  bool show_grid;
  int margin;
  SDL_Color background_color;
  SDL_Color grid_color;
  SDL_Color axis_color;
  SDL_Color border_color;

  DeclUIState *children;
} PlotData;

static void calculate_plot_bounds(PlotData *plot) {}

static SDL_Point data_to_screen(PlotData *plot, double x, double y, int width,
                                int height) {
  SDL_Point point;

  int plot_width = width - 2 * plot->margin;
  int plot_height = height - 2 * plot->margin;

  // Normalize to 0-1 range using plot bounds
  double x_norm = (x - plot->x_min) / (plot->x_max - plot->x_min);
  double y_norm = (y - plot->y_min) / (plot->y_max - plot->y_min);

  point.x = plot->margin + (int)(x_norm * plot_width);
  point.y = height - plot->margin - (int)(y_norm * plot_height); // Flip Y axis

  return point;
}

static void draw_grid_and_axes(PlotData *plot, SDL_Renderer *renderer,
                               int width, int height) {
  if (!plot->show_grid)
    return;

  SDL_SetRenderDrawColor(renderer, plot->grid_color.r, plot->grid_color.g,
                         plot->grid_color.b, plot->grid_color.a);

  int plot_width = width - 2 * plot->margin;
  int plot_height = height - 2 * plot->margin;

  for (int i = 0; i <= 10; i++) {
    int x = plot->margin + (i * plot_width) / 10;
    SDL_RenderDrawLine(renderer, x, plot->margin, x, height - plot->margin);
  }

  for (int i = 0; i <= 10; i++) {
    int y = plot->margin + (i * plot_height) / 10;
    SDL_RenderDrawLine(renderer, plot->margin, y, width - plot->margin, y);
  }

  SDL_SetRenderDrawColor(renderer, plot->axis_color.r, plot->axis_color.g,
                         plot->axis_color.b, plot->axis_color.a);

  if (plot->y_min <= 0 && plot->y_max >= 0) {
    SDL_Point zero_point = data_to_screen(plot, plot->x_min, 0, width, height);
    SDL_RenderDrawLine(renderer, plot->margin, zero_point.y,
                       width - plot->margin, zero_point.y);
  }

  if (plot->x_min <= 0 && plot->x_max >= 0) {
    SDL_Point zero_point = data_to_screen(plot, 0, plot->y_min, width, height);
    SDL_RenderDrawLine(renderer, zero_point.x, plot->margin, zero_point.x,
                       height - plot->margin);
  }
}

static void draw_axis_labels(PlotData *plot, SDL_Renderer *renderer, int width,
                             int height) {
  char label[32];
  SDL_Color text_color = {0, 0, 0, 255}; // Black text

  for (int i = 0; i <= 5; i++) {
    double x_val = plot->x_min + i * (plot->x_max - plot->x_min) / 5;
    SDL_Point label_pos =
        data_to_screen(plot, x_val, plot->y_min, width, height);
    sprintf(label, "%.2f", x_val);
    render_text(label, label_pos.x - 20, height - plot->margin + 5, renderer,
                text_color);
  }

  for (int i = 0; i <= 5; i++) {
    double y_val = plot->y_min + i * (plot->y_max - plot->y_min) / 5;
    SDL_Point label_pos =
        data_to_screen(plot, plot->x_min, y_val, width, height);
    sprintf(label, "%.2f", y_val);
    render_text(label, 5, label_pos.y - 5, renderer, text_color);
  }
}

static void draw_filled_circle(SDL_Renderer *renderer, int x, int y,
                               int radius) {
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dy * dy <= radius * radius) {
        SDL_RenderDrawPoint(renderer, x + dx, y + dy);
      }
    }
  }
}

static PlotData *_current_plt_ctx;
static PlotData *get_current_plot() { return _current_plt_ctx; }

void render_plt(void *state, SDL_Renderer *renderer) {
  PlotData *plot = (PlotData *)state;
  if (!plot)
    return;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  calculate_plot_bounds(plot);

  SDL_SetRenderDrawColor(renderer, plot->background_color.r,
                         plot->background_color.g, plot->background_color.b,
                         plot->background_color.a);
  SDL_RenderClear(renderer);

  SDL_Rect plot_area = {plot->margin, plot->margin, width - 2 * plot->margin,
                        height - 2 * plot->margin};

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &plot_area);

  SDL_SetRenderDrawColor(renderer, plot->border_color.r, plot->border_color.g,
                         plot->border_color.b, plot->border_color.a);
  SDL_RenderDrawRect(renderer, &plot_area);

  draw_grid_and_axes(plot, renderer, width, height);
  draw_axis_labels(plot, renderer, width, height);

  UIObj *child = plot->children->head;
  _current_plt_ctx = plot;
  while (child) {
    child->render_cb(child->data, renderer);
    child = child->next;
  }
}

void *Plt(double x_min, double x_max, double y_min, double y_max) {
  PlotData *plot_data = malloc(sizeof(PlotData));
  *plot_data =
      (PlotData){.x_min = x_min,
                 .x_max = x_max,
                 .y_min = y_min,
                 .y_max = y_max,
                 .auto_bounds = (x_min == 0 && x_max == 0 && y_min == 0 &&
                                 y_max == 0), // Auto if all zeros
                 .show_grid = true,
                 .margin = 50,
                 .background_color = {240, 240, 240, 255}, // Light gray
                 .grid_color = {200, 200, 200, 255},       // Gray
                 .axis_color = {100, 100, 100, 255},       // Dark gray
                 .border_color = {180, 180, 180, 255},     // Medium gray
                 .children = calloc(1, sizeof(DeclUIState))};

  UIObj obj = {.data = plot_data,
               .render_cb = (UIObjRendererFn)render_plt,
               .bounds = {1000, 2000}};

  return append_obj(_decl_ui_ctx, obj);
}

typedef struct {
  double *x_data;
  double *y_data;
  int size;

  int point_radius;
  SDL_Color point_color;
  SDL_Color selected_color;

  int selected_point;
} ScatterData;

void render_scatter(void *state, SDL_Renderer *renderer) {
  ScatterData *scatter = (ScatterData *)state;
  PlotData *plot = get_current_plot();
  if (!scatter || !scatter->x_data || !scatter->y_data || !plot)
    return;

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  for (int i = 0; i < scatter->size; i++) {
    SDL_Point screen_point = data_to_screen(plot, scatter->x_data[i],
                                            scatter->y_data[i], width, height);

    if (screen_point.x < plot->margin ||
        screen_point.x > width - plot->margin ||
        screen_point.y < plot->margin ||
        screen_point.y > height - plot->margin) {
      continue;
    }

    if (i == scatter->selected_point) {
      SDL_SetRenderDrawColor(
          renderer, scatter->selected_color.r, scatter->selected_color.g,
          scatter->selected_color.b, scatter->selected_color.a);
    } else {
      SDL_SetRenderDrawColor(renderer, scatter->point_color.r,
                             scatter->point_color.g, scatter->point_color.b,
                             scatter->point_color.a);
    }

    draw_filled_circle(renderer, screen_point.x, screen_point.y,
                       scatter->point_radius);
  }
}
void *Scatter(void *_plt, int size, double *x, double *y) {
  UIObj *plt = _plt;
  PlotData *plot_data = (PlotData *)plt->data;

  ScatterData *scatter_data = malloc(sizeof(ScatterData));
  *scatter_data = (ScatterData){
      .x_data = x,
      .y_data = y,
      .size = size,
      .point_radius = 4,
      .point_color = {0, 120, 200, 255},  // Blue points
      .selected_color = {255, 0, 0, 255}, // Red when selected
      .selected_point = -1,
  };

  UIObj obj = {.data = scatter_data,
               .render_cb = (UIObjRendererFn)render_scatter};

  return append_obj(plot_data->children, obj);
}

typedef struct {
  double *x_data;
  double *y_data;
  int size;

  SDL_Color line_color;
  int line_thickness;
  bool show_points;      // Whether to draw points at vertices
  int point_radius;      // Size of vertex points if shown
  SDL_Color point_color; // Color of vertex points

  int selected_point; // Selected vertex point
  SDL_Color selected_color;
} LineData;

void render_line_plt(void *state, SDL_Renderer *renderer) {
  LineData *line = (LineData *)state;
  PlotData *plot = get_current_plot();

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  SDL_SetRenderDrawColor(renderer, line->line_color.r, line->line_color.g,
                         line->line_color.b, line->line_color.a);

  for (int i = 0; i < line->size - 1; i++) {
    SDL_Point start_point =
        data_to_screen(plot, line->x_data[i], line->y_data[i], width, height);
    SDL_Point end_point = data_to_screen(plot, line->x_data[i + 1],
                                         line->y_data[i + 1], width, height);

    if ((start_point.x < plot->margin && end_point.x < plot->margin) ||
        (start_point.x > width - plot->margin &&
         end_point.x > width - plot->margin) ||
        (start_point.y < plot->margin && end_point.y < plot->margin) ||
        (start_point.y > height - plot->margin &&
         end_point.y > height - plot->margin)) {
      continue;
    }

    SDL_RenderDrawLine(renderer, start_point.x, start_point.y, end_point.x,
                       end_point.y);
  }
}

void *LinePlt(void *_plt, int size, double *x, double *y) {
  UIObj *plt = _plt;
  PlotData *plot_data = (PlotData *)plt->data;

  LineData *line_data = malloc(sizeof(LineData));
  *line_data = (LineData){
      .x_data = x,
      .y_data = y,
      .size = size,
      .line_color = {200, 0, 0, 255},  // Red line
      .line_thickness = 2,             // 2 pixel thick line
      .show_points = false,            // Don't show vertex points by default
      .point_radius = 3,               // Small vertex points
      .point_color = {150, 0, 0, 255}, // Dark red points
      .selected_point = -1,
      .selected_color = {255, 100, 100, 255} // Light red when selected
  };

  UIObj obj = {.data = line_data,
               .render_cb = (UIObjRendererFn)render_line_plt};

  return append_obj(plot_data->children, obj);
}

typedef struct {
  bool *data; // Pointer to the array data
  int size;   // Number of elements in the array
} CheckBoxesData;

static SDL_Renderer *render_checkboxes(void *_state, SDL_Renderer *renderer) {
  CheckBoxesData *state = _state;
  if (!state || !state->data)
    return renderer;

  SDL_Rect viewport;
  SDL_RenderGetViewport(renderer, &viewport);
  int width = viewport.w;
  int height = viewport.h;

  const int checkbox_size = 20;
  const int spacing = 5;
  const int margin = 5;

  for (int i = 0; i < state->size; i++) {
    int x = margin + i * (checkbox_size + spacing);
    int y = margin;

    SDL_Rect checkbox = {x, y, checkbox_size, checkbox_size};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
    SDL_RenderFillRect(renderer, &checkbox);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black border
    SDL_RenderDrawRect(renderer, &checkbox);

    if (state->data[i]) {
      SDL_Color red_color = {255, 0, 0, 255}; // Red text
      render_text("x", x + 4, y, renderer, red_color);
    }
  }

  return renderer;
}
static int checkbox_event_handler(void *state, SDL_Event *event) {
  CheckBoxesData *cb_data = (CheckBoxesData *)state;
  if (!cb_data || !event)
    return 0;

  if (event->type == SDL_MOUSEBUTTONDOWN &&
      event->button.button == SDL_BUTTON_LEFT) {
    int mouse_x = event->button.x;
    int mouse_y = event->button.y;

    const int checkbox_size = 20;
    const int spacing = 5;
    const int margin = 5;

    for (int i = 0; i < cb_data->size; i++) {
      int x = margin + i * (checkbox_size + spacing);
      int y = margin;

      if (mouse_x >= x && mouse_x < x + checkbox_size && mouse_y >= y &&
          mouse_y < y + checkbox_size) {
        cb_data->data[i] = !cb_data->data[i];
        // printf("Checkbox %d toggled to %s\n", i,
        //        cb_data->data[i] ? "true" : "false");
        return 1;
      }
    }
  }
  return 0;
}
void *CheckBoxes(int size, bool *data) {
  CheckBoxesData *cb_data = malloc(sizeof(CheckBoxesData));
  cb_data->data = data;
  cb_data->size = size;
  UIObj obj = {.data = cb_data,
               .render_cb = render_checkboxes,
               .event_handler = checkbox_event_handler,
               .bounds = {40, 440}};
  return append_obj(_decl_ui_ctx, obj);
}
