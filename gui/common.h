#ifndef _LANG_GUI_COMMON_H
#define _LANG_GUI_COMMON_H
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_video.h"
#include <stdbool.h>

// #define DEFAULT_FONT TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 12)

#define MAX_WINDOWS 10
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef enum {
  WINDOW_TYPE_BASIC,
  WINDOW_TYPE_ARRAY_EDITOR,
  WINDOW_TYPE_OSCILLOSCOPE,
  WINDOW_TYPE_SLIDER,
  WINDOW_TYPE_CLAP_SLIDER,
  WINDOW_TYPE_PLOT_ARRAY,
  WINDOW_TYPE_CLAP_NATIVE
} WindowType;
extern TTF_Font *DEFAULT_FONT;
typedef struct Window Window;

typedef void (*EventHandler)(Window *window, SDL_Event *event);
typedef void (*WindowRenderFn)(Window *window);

typedef struct Window {
  SDL_Window *window;
  SDL_Renderer *renderer;
  WindowType type;
  void *data;
  TTF_Font *font;
  int width;
  int height;
  EventHandler handle_event; // Function pointer for event handling
  WindowRenderFn render_fn;
} Window;

typedef struct _scope_win_data {
  double *rms_channel_peaks;
  double *buf;
  int layout;
  int size;
  bool render_spectrum;
} _scope_win_data;

typedef struct _array_edit_win_data {
  int32_t _size;
  double *data_ptr;
} _array_edit_win_data;

typedef struct _array_plot_win_data {
  int32_t _size;
  double *data_ptr;
  double min;
  double max;
} _array_plot_win_data;

void render_text(const char *text, int x, int y, SDL_Color color,
                 SDL_Renderer *renderer, TTF_Font *font);

typedef struct {
  double *values;
  double *mins;
  double *maxes;
  char **labels;
  int active_slider;
  int slider_count;
  void (*on_update)(int, double);
} _slider_window_data;

typedef struct _plot_array_win_data {
  int32_t _size;
  double *data_ptr;
} _plot_array_win_data;

#endif
