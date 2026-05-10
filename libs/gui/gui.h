#ifndef GUI_H
#define GUI_H

#include "../../engine/node.h"
#include "../../lang/ylc_datatypes.h"
#include <SDL3/SDL.h>

// ============================================================================
// Generic window type registration
// ============================================================================

// Implement these three callbacks to define a window type.
//   draw     — called every frame; clear + present are handled by the loop
//   on_event — called for every SDL event; wid is the window that received it
//              (compare against SDL_GetWindowID(win) to filter)
//   destroy  — called when the window is closed; free state here
typedef struct {
  int id; // unique integer you assign
  void (*draw)(SDL_Window *win, SDL_Renderer *ren, void *state);
  void (*on_event)(SDL_Event *e, SDL_Window *win, SDL_Renderer *ren,
                   void *state);
  void (*destroy)(void *state); // may be NULL
} YLCWindowType;

// Register a window type before opening any windows of that type.
void ylc_window_type_register(YLCWindowType type);

// Open a window of the given type. `state` is passed verbatim to callbacks.
void ylc_window_open(int type_id, const char *title, int w, int h, void *state);

// ============================================================================
// Built-in oscilloscope window  (type id = 0)
// ============================================================================

#define YLC_WINDOW_SCOPE 0
#define YLC_WINDOW_ARRAY_EDITOR 1
#define YLC_WINDOW_SPECTROGRAM 2

// Open an oscilloscope window for `node`. Creates a tap node internally.
void ylc_scope_open(Node *node);

// Like ylc_scope_open but returns the tap node (pass-through).
Node *ylc_scope_tap(Node *source);

// Open a simple array editor window for a YLC double array.
void ylc_array_editor_open(_DoubleArray data, double min_value,
                           double max_value);

// Open a static spectrogram window for frame-major magnitudes and optionally
// overlay transient markers for normalized transient values above threshold.
void ylc_spectrogram_open(_DoubleArray mag, _DoubleArray transient,
                          int num_frames, int num_bins, double db_min,
                          double db_max, double transient_threshold);

#endif
