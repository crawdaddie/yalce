#include "gui.h"
#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/node.h"
#include "../../lang/common.h"
#include <SDL3/SDL.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Provided by libyalce (jit.c) — non-blocking REPL for shared main thread
extern void repl_poll_stdin(void);

#define MAX_WINDOWS 64

// ============================================================================
// Generic window registry
// ============================================================================

#define MAX_WINDOW_TYPES 32

static YLCWindowType window_types[MAX_WINDOW_TYPES];
static int num_window_types = 0;

void ylc_window_type_register(YLCWindowType type) {
  if (num_window_types < MAX_WINDOW_TYPES)
    window_types[num_window_types++] = type;
}

static YLCWindowType *find_type(int id) {
  for (int i = 0; i < num_window_types; i++)
    if (window_types[i].id == id)
      return &window_types[i];
  return NULL;
}

// ============================================================================
// Generic window instance
// ============================================================================

typedef struct {
  int type_id;
  SDL_Window *window;
  SDL_Renderer *renderer;
  void *state;
} GUIWindow;

static GUIWindow windows[MAX_WINDOWS];
static int num_windows = 0;

void ylc_window_open(int type_id, const char *title, int w, int h,
                     void *state) {
  if (num_windows >= MAX_WINDOWS)
    return;
  YLCWindowType *t = find_type(type_id);
  if (!t) {
    fprintf(stderr, "libgui: unknown window type %d\n", type_id);
    return;
  }
  SDL_Window *win =
      SDL_CreateWindow(title ? title : "", w, h, SDL_WINDOW_RESIZABLE);
  if (!win) {
    fprintf(stderr, "libgui: SDL_CreateWindow: %s\n", SDL_GetError());
    return;
  }
  SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);
  if (!ren) {
    fprintf(stderr, "libgui: SDL_CreateRenderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(win);
    return;
  }
  windows[num_windows++] = (GUIWindow){
      .type_id = type_id, .window = win, .renderer = ren, .state = state};
}

// ============================================================================
// Ring buffer — written by audio thread, read by GUI thread (SPSC, lock-free)
// ============================================================================

#define SCOPE_RING_SIZE 8192 // power of 2

typedef struct {
  double data[SCOPE_RING_SIZE];
  _Atomic int write_pos; // monotonically increasing
} ScopeRing;

// ============================================================================
// Tap node — lives in the audio chain, fills the ring as a side effect
// ============================================================================

typedef struct {
  Node *source;
  ScopeRing *ring;
} TapState;

static void *scope_tap_perform(Node *node, void *state_raw, Node *inputs[],
                               int nframes, double spf) {
  (void)state_raw;
  (void)inputs;
  (void)spf;
  TapState *st = (TapState *)((char *)node + sizeof(Node));
  double *in = st->source->output.buf;
  double *out = node->output.buf;
  int wpos = atomic_load_explicit(&st->ring->write_pos, memory_order_acquire);

  for (int i = 0; i < nframes; i++) {
    st->ring->data[(wpos + i) & (SCOPE_RING_SIZE - 1)] = in[i];
    out[i] = in[i];
  }

  atomic_store_explicit(&st->ring->write_pos, wpos + nframes,
                        memory_order_release);
  return NULL;
}

static Node *make_tap(Node *source, ScopeRing *ring) {
  size_t total = sizeof(Node) + sizeof(TapState) + BUF_SIZE * sizeof(double);
  Node *tap = calloc(1, total);
  if (!tap)
    return NULL;

  TapState *st = (TapState *)((char *)tap + sizeof(Node));
  st->source = source;
  st->ring = ring;

  tap->perform = (perform_func_t)scope_tap_perform;
  tap->num_inputs = 0;
  tap->state_size = sizeof(TapState);
  tap->meta = (char *)"scope_tap";
  tap->output = (Signal){
      .layout = 1,
      .size = BUF_SIZE,
      .buf = (double *)((char *)tap + sizeof(Node) + sizeof(TapState)),
  };

  tap->next = source->next;
  source->next = tap;

  return tap;
}

// ============================================================================
// Scope window type (id = YLC_WINDOW_SCOPE = 0)
// ============================================================================

typedef struct {
  ScopeRing *ring;
  float zoom; // samples per pixel (1.0 = 1:1, >1 = zoom out)
} ScopeState;

static void scope_draw(SDL_Window *win, SDL_Renderer *ren, void *state) {
  ScopeState *s = (ScopeState *)state;
  int wi, hi;
  SDL_GetWindowSize(win, &wi, &hi);
  float h = (float)hi;

  SDL_SetRenderDrawColor(ren, 18, 18, 18, 255);
  SDL_RenderClear(ren);
  SDL_SetRenderDrawColor(ren, 0, 220, 80, 255);

  int wpos = atomic_load_explicit(&s->ring->write_pos, memory_order_acquire);
  float zoom = s->zoom > 0.f ? s->zoom : 1.f;
  int nsamples = (int)(wi * zoom);

  if (zoom <= 1.f) {
    // 1:1 — connect adjacent samples with lines
    for (int x = 0; x < wi - 1; x++) {
      int i0 = (wpos - nsamples + x) & (SCOPE_RING_SIZE - 1);
      int i1 = (wpos - nsamples + x + 1) & (SCOPE_RING_SIZE - 1);
      float y0 = h * 0.5f * (1.f - 0.25f * (float)s->ring->data[i0]);
      float y1 = h * 0.5f * (1.f - 0.25f * (float)s->ring->data[i1]);
      SDL_RenderLine(ren, (float)x, y0, (float)(x + 1), y1);
    }
  } else {
    // Multiple samples per pixel — draw min/max bar per column
    for (int x = 0; x < wi; x++) {
      int s0 = (int)(x * zoom);
      int s1 = (int)((x + 1) * zoom);
      if (s1 > nsamples)
        s1 = nsamples;
      float vmin = 1.f, vmax = -1.f;
      for (int j = s0; j < s1; j++) {
        float v =
            (float)s->ring->data[(wpos - nsamples + j) & (SCOPE_RING_SIZE - 1)];
        if (v < vmin)
          vmin = v;
        if (v > vmax)
          vmax = v;
      }
      float ytop = h * 0.5f * (1.f - 0.25f * vmax);
      float ybot = h * 0.5f * (1.f - 0.25f * vmin);
      SDL_RenderLine(ren, (float)x, ytop, (float)x, ybot);
    }
  }

  SDL_RenderPresent(ren);
}

static void scope_on_event(SDL_Event *e, SDL_Window *win, SDL_Renderer *ren,
                           void *state) {
  (void)ren;
  if (e->type != SDL_EVENT_MOUSE_WHEEL && e->type != SDL_EVENT_KEY_DOWN)
    return;
  SDL_WindowID wid =
      (e->type == SDL_EVENT_MOUSE_WHEEL) ? e->wheel.windowID : e->key.windowID;
  if (SDL_GetWindowID(win) != wid)
    return;

  ScopeState *s = (ScopeState *)state;
  if (e->type == SDL_EVENT_MOUSE_WHEEL) {
    s->zoom *= (e->wheel.y > 0) ? 1.25f : 0.8f;
  } else {
    SDL_Keycode k = e->key.key;
    if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS)
      s->zoom *= 1.25f;
    else if (k == SDLK_MINUS || k == SDLK_KP_MINUS)
      s->zoom *= 0.8f;
  }
  if (s->zoom < 1.f)
    s->zoom = 1.f;
  if (s->zoom > SCOPE_RING_SIZE / 4.f)
    s->zoom = SCOPE_RING_SIZE / 4.f;
}

static void scope_destroy(void *state) {
  ScopeState *s = (ScopeState *)state;
  free(s->ring);
  free(s);
}

// ============================================================================
// Array editor window type (id = YLC_WINDOW_ARRAY_EDITOR = 1)
// ============================================================================

typedef struct {
  _DoubleArray data;
  int selected_index;
  bool dragging;
  double value_min;
  double value_max;
} ArrayEditorState;

static double array_editor_clamp(double lo, double hi, double v) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static void array_editor_screen_to_value(ArrayEditorState *s, int x, int y,
                                         int width, int height, int *index_out,
                                         double *value_out) {
  const int margin = 24;
  int plot_w = width - 2 * margin;
  int plot_h = height - 2 * margin;
  if (plot_w <= 1)
    plot_w = 1;
  if (plot_h <= 1)
    plot_h = 1;

  int size = s->data.size > 0 ? s->data.size : 1;
  double x_ratio = (double)(x - margin) / (double)plot_w;
  int index = (int)(x_ratio * (double)(size - 1) + 0.5);
  if (index < 0)
    index = 0;
  if (index >= size)
    index = size - 1;

  double y_ratio = (double)(height - margin - y) / (double)plot_h;
  double value = s->value_min + y_ratio * (s->value_max - s->value_min);
  value = array_editor_clamp(s->value_min, s->value_max, value);

  *index_out = index;
  *value_out = value;
}

static void array_editor_draw(SDL_Window *win, SDL_Renderer *ren, void *state) {
  ArrayEditorState *s = (ArrayEditorState *)state;
  int width = 0, height = 0;
  SDL_GetWindowSize(win, &width, &height);

  SDL_SetRenderDrawColor(ren, 242, 242, 238, 255);
  SDL_RenderClear(ren);

  const int margin = 24;
  const int plot_w = width - 2 * margin;
  const int plot_h = height - 2 * margin;
  SDL_FRect plot = {(float)margin, (float)margin, (float)plot_w, (float)plot_h};

  SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
  SDL_RenderFillRect(ren, &plot);

  SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
  for (int i = 0; i <= 8; i++) {
    float x = (float)margin + ((float)plot_w * (float)i / 8.0f);
    SDL_RenderLine(ren, x, (float)margin, x, (float)(height - margin));
  }
  for (int i = 0; i <= 8; i++) {
    float y = (float)margin + ((float)plot_h * (float)i / 8.0f);
    SDL_RenderLine(ren, (float)margin, y, (float)(width - margin), y);
  }

  SDL_SetRenderDrawColor(ren, 90, 90, 90, 255);
  SDL_RenderRect(ren, &plot);

  if (!s || !s->data.data || s->data.size <= 0) {
    SDL_RenderPresent(ren);
    return;
  }

  for (int i = 0; i < s->data.size; i++) {
    double value = s->data.data[i];
    double normalized = (value - s->value_min) / (s->value_max - s->value_min);
    normalized = array_editor_clamp(0.0, 1.0, normalized);

    double bar_w = (double)plot_w / (double)s->data.size;
    float x = (float)margin + (float)(i * bar_w);
    float w = (float)(bar_w - 2.0);
    if (w < 2.0f)
      w = 2.0f;
    float h = (float)(normalized * (double)plot_h);
    SDL_FRect bar = {x + 1.0f, (float)(height - margin) - h, w, h};

    if (i == s->selected_index) {
      SDL_SetRenderDrawColor(ren, 210, 70, 50, 255);
    } else {
      SDL_SetRenderDrawColor(ren, 40, 130, 190, 255);
    }
    // SDL_RenderFillRect(ren, &bar);
  }

  SDL_SetRenderDrawColor(ren, 35, 35, 35, 255);
  for (int i = 0; i < s->data.size - 1; i++) {
    float x0 =
        (float)margin + ((float)i / (float)(s->data.size - 1)) * (float)plot_w;
    float x1 = (float)margin +
               ((float)(i + 1) / (float)(s->data.size - 1)) * (float)plot_w;
    double n0 =
        (s->data.data[i] - s->value_min) / (s->value_max - s->value_min);
    double n1 =
        (s->data.data[i + 1] - s->value_min) / (s->value_max - s->value_min);
    n0 = array_editor_clamp(0.0, 1.0, n0);
    n1 = array_editor_clamp(0.0, 1.0, n1);
    float y0 = (float)(height - margin) - (float)(n0 * (double)plot_h);
    float y1 = (float)(height - margin) - (float)(n1 * (double)plot_h);
    SDL_RenderLine(ren, x0, y0, x1, y1);
  }

  for (int i = 0; i < s->data.size; i++) {
    float x =
        (float)margin +
        ((s->data.size > 1 ? (float)i / (float)(s->data.size - 1) : 0.0f) *
         (float)plot_w);
    double n = (s->data.data[i] - s->value_min) / (s->value_max - s->value_min);
    n = array_editor_clamp(0.0, 1.0, n);
    float y = (float)(height - margin) - (float)(n * (double)plot_h);
    SDL_FRect p = {x - 3.0f, y - 3.0f, 6.0f, 6.0f};
    if (i == s->selected_index) {
      SDL_SetRenderDrawColor(ren, 210, 70, 50, 255);
    } else {
      SDL_SetRenderDrawColor(ren, 15, 15, 15, 255);
    }
    SDL_RenderFillRect(ren, &p);
  }

  SDL_RenderPresent(ren);
}

static void array_editor_on_event(SDL_Event *e, SDL_Window *win,
                                  SDL_Renderer *ren, void *state) {
  (void)ren;
  ArrayEditorState *s = (ArrayEditorState *)state;
  SDL_WindowID wid = 0;

  switch (e->type) {
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    wid = e->button.windowID;
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    wid = e->button.windowID;
    break;
  case SDL_EVENT_MOUSE_MOTION:
    wid = e->motion.windowID;
    break;
  case SDL_EVENT_KEY_DOWN:
    wid = e->key.windowID;
    break;
  default:
    return;
  }

  if (SDL_GetWindowID(win) != wid || !s || !s->data.data || s->data.size <= 0)
    return;

  int width = 0, height = 0;
  SDL_GetWindowSize(win, &width, &height);

  if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      e->button.button == SDL_BUTTON_LEFT) {
    int index = 0;
    double value = 0.0;
    s->dragging = true;
    array_editor_screen_to_value(s, e->button.x, e->button.y, width, height,
                                 &index, &value);
    s->selected_index = index;
    s->data.data[index] = value;
    return;
  }

  if (e->type == SDL_EVENT_MOUSE_BUTTON_UP &&
      e->button.button == SDL_BUTTON_LEFT) {
    s->dragging = false;
    return;
  }

  if (e->type == SDL_EVENT_MOUSE_MOTION && s->dragging) {
    int index = 0;
    double value = 0.0;
    array_editor_screen_to_value(s, e->motion.x, e->motion.y, width, height,
                                 &index, &value);
    s->selected_index = index;
    s->data.data[index] = value;
    return;
  }

  if (e->type == SDL_EVENT_KEY_DOWN && s->selected_index >= 0) {
    double step = (s->value_max - s->value_min) * 0.05;
    if (step <= 0.0)
      step = 1.0;
    if (e->key.key == SDLK_LEFT && s->selected_index > 0)
      s->selected_index--;
    else if (e->key.key == SDLK_RIGHT && s->selected_index < s->data.size - 1)
      s->selected_index++;
    else if (e->key.key == SDLK_UP)
      s->data.data[s->selected_index] = array_editor_clamp(
          s->value_min, s->value_max, s->data.data[s->selected_index] + step);
    else if (e->key.key == SDLK_DOWN)
      s->data.data[s->selected_index] = array_editor_clamp(
          s->value_min, s->value_max, s->data.data[s->selected_index] - step);
  }
}

static void array_editor_destroy(void *state) { free(state); }

// ============================================================================
// Public scope API
// ============================================================================

static void scope_enqueue(ScopeRing *ring) {
  ScopeState *s = calloc(1, sizeof(ScopeState));
  if (!s) {
    free(ring);
    return;
  }
  s->ring = ring;
  s->zoom = 1.f;
  ylc_window_open(YLC_WINDOW_SCOPE, "scope", 800, 300, s);
}

Node *ylc_scope_tap(Node *source) {
  ScopeRing *ring = calloc(1, sizeof(ScopeRing));
  if (!ring)
    return source;

  Node *tap = make_tap(source, ring);
  if (!tap) {
    free(ring);
    return source;
  }

  scope_enqueue(ring);
  return tap;
}

void ylc_scope_open(Node *node) {
  ScopeRing *ring = calloc(1, sizeof(ScopeRing));
  if (!ring)
    return;

  if (!make_tap(node, ring)) {
    free(ring);
    return;
  }

  scope_enqueue(ring);
}

void ylc_array_editor_open(_DoubleArray data, double min_value,
                           double max_value) {

  if (!data.data || data.size <= 0 || max_value <= min_value) {
    return;
  }

  ArrayEditorState *s = calloc(1, sizeof(ArrayEditorState));
  if (!s) {
    return;
  }
  s->data = data;
  s->selected_index = -1;
  s->dragging = false;
  s->value_min = min_value;
  s->value_max = max_value;

  ylc_window_open(YLC_WINDOW_ARRAY_EDITOR, "array editor", 800, 320, s);
  return;
}

// ============================================================================
// Main SDL event loop — runs on the main thread
// ============================================================================

static void sdl_main_loop(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "libgui: SDL_Init failed: %s\n", SDL_GetError());
    return;
  }

  while (true) {
    // Process events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        goto quit;

      if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        for (int i = 0; i < num_windows; i++) {
          if (SDL_GetWindowID(windows[i].window) != e.window.windowID)
            continue;
          YLCWindowType *t = find_type(windows[i].type_id);
          if (t && t->destroy)
            t->destroy(windows[i].state);
          SDL_DestroyRenderer(windows[i].renderer);
          SDL_DestroyWindow(windows[i].window);
          windows[i] = windows[--num_windows];
          break;
        }
      }

      // Dispatch event to each window's type handler
      for (int i = 0; i < num_windows; i++) {
        YLCWindowType *t = find_type(windows[i].type_id);
        if (t && t->on_event)
          t->on_event(&e, windows[i].window, windows[i].renderer,
                      windows[i].state);
      }
    }

    // Draw all windows
    for (int i = 0; i < num_windows; i++) {
      YLCWindowType *t = find_type(windows[i].type_id);
      if (t && t->draw)
        t->draw(windows[i].window, windows[i].renderer, windows[i].state);
    }

    repl_poll_stdin();
    SDL_Delay(8);
  }

quit:
  for (int i = 0; i < num_windows; i++) {
    YLCWindowType *t = find_type(windows[i].type_id);
    if (t && t->destroy)
      t->destroy(windows[i].state);
    SDL_DestroyRenderer(windows[i].renderer);
    SDL_DestroyWindow(windows[i].window);
  }
  SDL_Quit();
}

// ============================================================================
// Library constructor — register built-in types and hook into REPL
// ============================================================================

__attribute__((constructor)) static void ylc_gui_init(void) {
  ylc_window_type_register((YLCWindowType){
      .id = YLC_WINDOW_SCOPE,
      .draw = scope_draw,
      .on_event = scope_on_event,
      .destroy = scope_destroy,
  });
  ylc_window_type_register((YLCWindowType){
      .id = YLC_WINDOW_ARRAY_EDITOR,
      .draw = array_editor_draw,
      .on_event = array_editor_on_event,
      .destroy = array_editor_destroy,
  });

  __set_break_repl_flag(true);
  __set_break_repl_cb(sdl_main_loop);

  fprintf(stderr, "libgui: ready\n");
}
