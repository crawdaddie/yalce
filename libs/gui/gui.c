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

// ============================================================================
// Pending queue (main-thread only — no mutex needed)
// ============================================================================

typedef struct {
  int type_id;
  char title[64];
  int w, h;
  void *state;
} PendingWindow;

static PendingWindow pending[MAX_WINDOWS];
static int num_pending = 0;

void ylc_window_open(int type_id, const char *title, int w, int h,
                     void *state) {
  if (num_pending >= MAX_WINDOWS)
    return;
  PendingWindow *p = &pending[num_pending++];
  p->type_id = type_id;
  p->w = w;
  p->h = h;
  p->state = state;
  strncpy(p->title, title ? title : "", sizeof(p->title) - 1);
  p->title[sizeof(p->title) - 1] = '\0';
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

// ============================================================================
// Main SDL event loop — runs on the main thread
// ============================================================================

static void sdl_main_loop(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "libgui: SDL_Init failed: %s\n", SDL_GetError());
    return;
  }

  while (true) {
    // Drain pending window requests
    int snap = num_pending;
    num_pending = 0;

    for (int i = 0; i < snap && num_windows < MAX_WINDOWS; i++) {
      PendingWindow *p = &pending[i];
      if (!find_type(p->type_id)) {
        fprintf(stderr, "libgui: unknown window type %d\n", p->type_id);
        continue;
      }
      SDL_Window *win =
          SDL_CreateWindow(p->title, p->w, p->h, SDL_WINDOW_RESIZABLE);
      if (!win) {
        fprintf(stderr, "libgui: SDL_CreateWindow: %s\n", SDL_GetError());
        continue;
      }
      SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);
      if (!ren) {
        fprintf(stderr, "libgui: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        continue;
      }
      windows[num_windows++] = (GUIWindow){
          .type_id = p->type_id,
          .window = win,
          .renderer = ren,
          .state = p->state,
      };
    }

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

  __set_break_repl_flag(true);
  __set_break_repl_cb(sdl_main_loop);
  fprintf(stderr, "libgui: SDL3 scope ready\n");
}
