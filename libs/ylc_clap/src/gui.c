#include "plugin_internal.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool ylc_point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
  return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void ylc_gui_draw(ylc_plugin_t *self) {
  if (!self || !self->display || !self->gui_window || !self->gc) {
    return;
  }

  const unsigned long white =
      WhitePixel(self->display, DefaultScreen(self->display));
  const unsigned long black =
      BlackPixel(self->display, DefaultScreen(self->display));

  XSetForeground(self->display, self->gc, white);
  XFillRectangle(self->display, self->gui_window, self->gc, 0, 0, YLC_GUI_WIDTH,
                 YLC_GUI_HEIGHT);

  XSetForeground(self->display, self->gc, black);
  char instance_label[96] = {0};
  snprintf(instance_label, sizeof(instance_label), "Instance %u",
           self->instance_id);
  XDrawString(self->display, self->gui_window, self->gc, 448, 24,
              instance_label, (int)strlen(instance_label));

  XDrawString(self->display, self->gui_window, self->gc, 16, 24, "Script path",
              11);
  XDrawRectangle(self->display, self->gui_window, self->gc, YLC_PATH_X,
                 YLC_PATH_Y, YLC_PATH_W, YLC_PATH_H);
  XDrawString(self->display, self->gui_window, self->gc, YLC_PATH_X + 8,
              YLC_PATH_Y + 19, self->script_path,
              (int)strlen(self->script_path));
  if (self->path_focused) {
    const int cursor_x = YLC_PATH_X + 10 + (int)strlen(self->script_path) * 6;
    XDrawLine(self->display, self->gui_window, self->gc, cursor_x,
              YLC_PATH_Y + 7, cursor_x, YLC_PATH_Y + 22);
  }

  XDrawRectangle(self->display, self->gui_window, self->gc, YLC_OPEN_BUTTON_X,
                 YLC_OPEN_BUTTON_Y, YLC_OPEN_BUTTON_W, YLC_OPEN_BUTTON_H);
  XDrawString(self->display, self->gui_window, self->gc, YLC_OPEN_BUTTON_X + 16,
              YLC_OPEN_BUTTON_Y + 20, "Open in nvim", 12);

  XDrawRectangle(self->display, self->gui_window, self->gc, YLC_LOG_BUTTON_X,
                 YLC_LOG_BUTTON_Y, YLC_LOG_BUTTON_W, YLC_LOG_BUTTON_H);
  XDrawString(self->display, self->gui_window, self->gc, YLC_LOG_BUTTON_X + 18,
              YLC_LOG_BUTTON_Y + 20, "Follow log", 10);

  const bool reload_pending =
      atomic_load_explicit(&self->script_reload_pending, memory_order_acquire);
  const char *status =
      reload_pending
          ? "Script changed: reload pending"
          : (self->debug_log_path[0] != '\0'
                 ? "Watching script file for saves; log follower available"
                 : "Watching script file for saves; set YLC_DEBUG_LOG for log");

  XDrawString(self->display, self->gui_window, self->gc, YLC_STATUS_X,
              YLC_STATUS_Y, status, (int)strlen(status));

  XFlush(self->display);
}

static void ylc_gui_append_text(ylc_plugin_t *self, const char *text,
                                int text_len) {
  if (!self || !text || text_len <= 0) {
    return;
  }

  size_t current = strlen(self->script_path);
  size_t available = sizeof(self->script_path) - current - 1;
  if (available == 0) {
    return;
  }

  size_t count = (size_t)text_len;
  if (count > available) {
    count = available;
  }
  memcpy(self->script_path + current, text, count);
  self->script_path[current + count] = '\0';
  ylc_mark_state_dirty(self);
}

static void ylc_gui_handle_key(ylc_plugin_t *self, XKeyEvent *event) {
  if (!self || !event || !self->path_focused) {
    return;
  }

  char text[32] = {0};
  KeySym key = 0;
  const int text_len = XLookupString(event, text, sizeof(text), &key, NULL);

  if (key == XK_BackSpace) {
    size_t len = strlen(self->script_path);
    if (len > 0) {
      self->script_path[len - 1] = '\0';
      ylc_mark_state_dirty(self);
    }
  } else if (key == XK_Return || key == XK_KP_Enter) {
    ylc_setup_script_watcher(self);
    ylc_spawn_editor(self);
  } else if (key == XK_Escape) {
    self->path_focused = false;
  } else if (text_len > 0 && text[0] >= 32 && text[0] < 127) {
    ylc_gui_append_text(self, text, text_len);
  }

  ylc_gui_draw(self);
}

static void ylc_gui_handle_button(ylc_plugin_t *self, XButtonEvent *event) {
  if (!self || !event) {
    return;
  }

  if (ylc_point_in_rect(event->x, event->y, YLC_OPEN_BUTTON_X,
                        YLC_OPEN_BUTTON_Y, YLC_OPEN_BUTTON_W,
                        YLC_OPEN_BUTTON_H)) {
    self->path_focused = false;
    ylc_setup_script_watcher(self);
    ylc_gui_draw(self);
    ylc_spawn_editor(self);
    return;
  }

  if (ylc_point_in_rect(event->x, event->y, YLC_LOG_BUTTON_X, YLC_LOG_BUTTON_Y,
                        YLC_LOG_BUTTON_W, YLC_LOG_BUTTON_H)) {
    self->path_focused = false;
    ylc_gui_draw(self);
    ylc_spawn_log_follower(self);
    return;
  }

  if (self->path_focused &&
      !ylc_point_in_rect(event->x, event->y, YLC_PATH_X, YLC_PATH_Y, YLC_PATH_W,
                         YLC_PATH_H)) {
    ylc_setup_script_watcher(self);
  }

  self->path_focused = ylc_point_in_rect(event->x, event->y, YLC_PATH_X,
                                         YLC_PATH_Y, YLC_PATH_W, YLC_PATH_H);
  ylc_gui_draw(self);
}

void ylc_gui_poll_events(ylc_plugin_t *self) {
  if (!self || !self->display || !self->gui_window) {
    return;
  }

  while (XPending(self->display) > 0) {
    XEvent event;
    XNextEvent(self->display, &event);
    if (event.xany.window != self->gui_window) {
      continue;
    }

    switch (event.type) {
    case Expose:
      ylc_gui_draw(self);
      break;
    case ButtonPress:
      ylc_gui_handle_button(self, &event.xbutton);
      break;
    case KeyPress:
      ylc_gui_handle_key(self, &event.xkey);
      break;
    default:
      break;
    }
  }
}

static bool ylc_gui_create_window(ylc_plugin_t *self) {
  if (!self || !self->display || self->gui_window || !self->parent_window) {
    return self && self->gui_window != 0;
  }

  self->gui_window = XCreateSimpleWindow(
      self->display, self->parent_window, 0, 0, YLC_GUI_WIDTH, YLC_GUI_HEIGHT,
      0, BlackPixel(self->display, DefaultScreen(self->display)),
      WhitePixel(self->display, DefaultScreen(self->display)));
  if (!self->gui_window) {
    return false;
  }

  XSelectInput(self->display, self->gui_window,
               ExposureMask | ButtonPressMask | KeyPressMask |
                   StructureNotifyMask);
  self->gc = XCreateGC(self->display, self->gui_window, 0, NULL);
  if (!self->gc) {
    XDestroyWindow(self->display, self->gui_window);
    self->gui_window = 0;
    return false;
  }

  return true;
}

void ylc_gui_close(ylc_plugin_t *self) {
  if (!self || !self->display) {
    return;
  }

  if (self->gc) {
    XFreeGC(self->display, self->gc);
    self->gc = 0;
  }
  if (self->gui_window) {
    XDestroyWindow(self->display, self->gui_window);
    self->gui_window = 0;
  }
  XCloseDisplay(self->display);
  self->display = NULL;
  self->parent_window = 0;
  self->gui_created = false;
  self->gui_visible = false;
  self->path_focused = false;
}

static bool ylc_gui_is_api_supported(const clap_plugin_t *plugin,
                                     const char *api, bool is_floating) {
  (void)plugin;
  return !is_floating && api && strcmp(api, CLAP_WINDOW_API_X11) == 0;
}

static bool ylc_gui_get_preferred_api(const clap_plugin_t *plugin,
                                      const char **api, bool *is_floating) {
  (void)plugin;
  if (!api || !is_floating) {
    return false;
  }

  *api = CLAP_WINDOW_API_X11;
  *is_floating = false;
  return true;
}

static bool ylc_gui_create(const clap_plugin_t *plugin, const char *api,
                           bool is_floating) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || is_floating ||
      (api && api[0] != '\0' && strcmp(api, CLAP_WINDOW_API_X11) != 0)) {
    return false;
  }

  if (self->display) {
    self->gui_created = true;
    return true;
  }

  self->display = XOpenDisplay(NULL);
  if (!self->display) {
    return false;
  }

  self->gui_created = true;
  return true;
}

static void ylc_gui_destroy(const clap_plugin_t *plugin) {
  ylc_gui_close(ylc_from_plugin(plugin));
}

static bool ylc_gui_set_scale(const clap_plugin_t *plugin, double scale) {
  (void)plugin;
  (void)scale;
  return true;
}

static bool ylc_gui_get_size(const clap_plugin_t *plugin, uint32_t *width,
                             uint32_t *height) {
  (void)plugin;
  if (!width || !height) {
    return false;
  }

  *width = YLC_GUI_WIDTH;
  *height = YLC_GUI_HEIGHT;
  return true;
}

static bool ylc_gui_can_resize(const clap_plugin_t *plugin) {
  (void)plugin;
  return false;
}

static bool ylc_gui_get_resize_hints(const clap_plugin_t *plugin,
                                     clap_gui_resize_hints_t *hints) {
  (void)plugin;
  if (!hints) {
    return false;
  }

  memset(hints, 0, sizeof(*hints));
  return true;
}

static bool ylc_gui_adjust_size(const clap_plugin_t *plugin, uint32_t *width,
                                uint32_t *height) {
  return ylc_gui_get_size(plugin, width, height);
}

static bool ylc_gui_set_size(const clap_plugin_t *plugin, uint32_t width,
                             uint32_t height) {
  (void)plugin;
  return width == YLC_GUI_WIDTH && height == YLC_GUI_HEIGHT;
}

static bool ylc_gui_set_parent(const clap_plugin_t *plugin,
                               const clap_window_t *window) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !window || !window->api ||
      strcmp(window->api, CLAP_WINDOW_API_X11) != 0) {
    return false;
  }

  self->parent_window = window->x11;
  return ylc_gui_create_window(self);
}

static bool ylc_gui_set_transient(const clap_plugin_t *plugin,
                                  const clap_window_t *window) {
  (void)plugin;
  (void)window;
  return false;
}

static void ylc_gui_suggest_title(const clap_plugin_t *plugin,
                                  const char *title) {
  (void)plugin;
  (void)title;
}

static bool ylc_gui_show(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !self->display || !ylc_gui_create_window(self)) {
    return false;
  }

  XMapWindow(self->display, self->gui_window);
  XFlush(self->display);
  self->gui_visible = true;
  ylc_gui_draw(self);

  if (!self->destroying && self->clap_initialized && self->host &&
      self->host->request_callback) {
    self->host->request_callback(self->host);
  }
  return true;
}

static bool ylc_gui_hide(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !self->display || !self->gui_window) {
    return false;
  }

  XUnmapWindow(self->display, self->gui_window);
  XFlush(self->display);
  self->gui_visible = false;
  return true;
}

const clap_plugin_gui_t *ylc_gui_extension(void) {
  static const clap_plugin_gui_t gui = {
      .is_api_supported = ylc_gui_is_api_supported,
      .get_preferred_api = ylc_gui_get_preferred_api,
      .create = ylc_gui_create,
      .destroy = ylc_gui_destroy,
      .set_scale = ylc_gui_set_scale,
      .get_size = ylc_gui_get_size,
      .can_resize = ylc_gui_can_resize,
      .get_resize_hints = ylc_gui_get_resize_hints,
      .adjust_size = ylc_gui_adjust_size,
      .set_size = ylc_gui_set_size,
      .set_parent = ylc_gui_set_parent,
      .set_transient = ylc_gui_set_transient,
      .suggest_title = ylc_gui_suggest_title,
      .show = ylc_gui_show,
      .hide = ylc_gui_hide,
  };

  return &gui;
}
