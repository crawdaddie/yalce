#include "debug.h"
#include "plugin_internal.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool ylc_point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
  return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static double ylc_gui_clamp(double v, double lo, double hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

#define YLC_SF_EDITOR_H 120
#define YLC_SF_EDITOR_GAP 12

static void ylc_gui_draw_soundfile(ylc_plugin_t *self, ylc_soundfile_t *sf,
                                   int x0, int y0, int w, int h);

static bool ylc_gui_array_is_env(const ylc_ui_slot_t *s) {
  return s && s->kind == YLC_UI_ENV && s->array_values && s->array_count >= 4 &&
         ((s->array_count - 1) % 3) == 0;
}

static bool ylc_gui_array_is_adsr(const ylc_ui_slot_t *s) {
  return s && s->kind == YLC_UI_ADSR && s->array_values && s->array_count == 4;
}

static int ylc_gui_env_point_count(const ylc_ui_slot_t *s) {
  return ylc_gui_array_is_env(s) ? (int)((s->array_count + 2) / 3) : 0;
}

static double *ylc_gui_env_val_ptr(ylc_ui_slot_t *s, int p) {
  return s->array_values + (p * 3);
}

static double *ylc_gui_env_time_ptr(ylc_ui_slot_t *s, int seg) {
  return s->array_values + (seg * 3) + 1;
}

static double *ylc_gui_env_curve_ptr(ylc_ui_slot_t *s, int seg) {
  return s->array_values + (seg * 3) + 2;
}

static double ylc_gui_env_point_x(ylc_ui_slot_t *s, int p) {
  double x = 0.0;
  for (int i = 0; i < p; ++i) {
    x += *ylc_gui_env_time_ptr(s, i);
  }
  return x;
}

static double ylc_gui_env_total_x(ylc_ui_slot_t *s) {
  int pts = ylc_gui_env_point_count(s);
  return pts > 0 ? ylc_gui_env_point_x(s, pts - 1) : 0.0;
}

static double ylc_gui_env_interp(double t, double y0, double y1, double curve) {
  t = ylc_gui_clamp(t, 0.0, 1.0);
  if (fabs(curve) < 0.001) {
    return y0 + (y1 - y0) * t;
  }
  double sign = curve > 0.0 ? 1.0 : -1.0;
  double amount = fabs(curve) * 3.0;
  double k = exp(sign * amount);
  double denom = k - 1.0;
  if (fabs(denom) < 1e-12) {
    return y0 + (y1 - y0) * t;
  }
  double ct = (exp(sign * amount * t) - 1.0) / denom;
  return y0 + (y1 - y0) * ct;
}

static void ylc_gui_env_to_screen(ylc_ui_slot_t *s, int p, double total, int x0,
                                  int y0, int w, int h, int *sx, int *sy) {
  double px = ylc_gui_env_point_x(s, p);
  double py = *ylc_gui_env_val_ptr(s, p);
  *sx = x0 + (int)((px / (total > 0 ? total : 1.0)) * w);
  *sy = y0 + (int)((1.0 - ylc_gui_clamp(py, 0.0, 1.0)) * h);
}

static void ylc_gui_draw_env(ylc_plugin_t *self, ylc_ui_slot_t *slot,
                             uint32_t index, int x0, int y0, int w, int h) {
  if (!ylc_gui_array_is_env(slot)) {
    XDrawString(self->display, self->gui_window, self->gc, x0, y0 + 12,
                "EnvArrayUI expects [value, dur, curve, value, ...]", 47);
    return;
  }

  int points = ylc_gui_env_point_count(slot);
  double total = ylc_gui_env_total_x(slot) * 1.05;
  if (total <= 0.0) {
    total = 1.0;
  }

  const unsigned long dark =
      BlackPixel(self->display, DefaultScreen(self->display));
  const unsigned long green = (16 << 16) | (180 << 8) | (80);
  const unsigned long yellow = (200 << 16) | (200 << 8) | (80);

  XSetForeground(self->display, self->gc, dark);
  XDrawRectangle(self->display, self->gui_window, self->gc, x0, y0, w, h);

  int pad = 8;
  int cx = x0 + pad, cy = y0 + pad;
  int cw = w - 2 * pad, ch = h - 2 * pad;

  for (int i = 0; i < points - 1; ++i) {
    double x0d = ylc_gui_env_point_x(slot, i);
    double y0d = *ylc_gui_env_val_ptr(slot, i);
    double x1d = ylc_gui_env_point_x(slot, i + 1);
    double y1d = *ylc_gui_env_val_ptr(slot, i + 1);
    double curve = *ylc_gui_env_curve_ptr(slot, i);
    int px, py, nx, ny;
    ylc_gui_env_to_screen(slot, i, total, cx, cy, cw, ch, &px, &py);
    const int segs = 24;
    for (int j = 1; j <= segs; ++j) {
      double t = (double)j / segs;
      double xd = x0d + (x1d - x0d) * t;
      double yd = ylc_gui_env_interp(t, y0d, y1d, curve);
      int sx = cx + (int)((xd / total) * cw);
      int sy = cy + (int)((1.0 - ylc_gui_clamp(yd, 0.0, 1.0)) * ch);
      XSetForeground(self->display, self->gc, green);
      XDrawLine(self->display, self->gui_window, self->gc, px, py, sx, sy);
      px = sx;
      py = sy;
    }
  }

  for (int i = 0; i < points; ++i) {
    int sx, sy;
    ylc_gui_env_to_screen(slot, i, total, cx, cy, cw, ch, &sx, &sy);
    bool selected = ((int)index == self->gui_selected_array &&
                     i == self->gui_selected_point);
    XSetForeground(self->display, self->gc, selected ? yellow : yellow);
    XFillArc(self->display, self->gui_window, self->gc, sx - 4, sy - 4, 8, 8, 0,
             360 * 64);
  }
}

static void ylc_gui_draw_adsr(ylc_plugin_t *self, ylc_ui_slot_t *slot,
                              uint32_t index, int x0, int y0, int w, int h) {
  if (!ylc_gui_array_is_adsr(slot)) {
    XDrawString(self->display, self->gui_window, self->gc, x0, y0 + 12,
                "ADSRArrayUI expects [attack, decay, sustain, release]", 50);
    return;
  }
  const char *labels[4] = {"A", "D", "S", "R"};
  const double ranges[4] = {5.0, 5.0, 1.0, 5.0};
  int bar_w = w / 4 - 4;
  for (int i = 0; i < 4; ++i) {
    int bx = x0 + i * (bar_w + 4);
    int bh = (int)(ylc_gui_clamp(slot->array_values[i] / ranges[i], 0.0, 1.0) *
                   (h - 20));
    bool selected = ((int)index == self->gui_selected_array &&
                     i == self->gui_selected_point);
    const unsigned long bar_color = selected ? ((200 << 16) | (200 << 8) | 80)
                                             : ((80 << 16) | (180 << 8) | 120);
    XSetForeground(self->display, self->gc, bar_color);
    XFillRectangle(self->display, self->gui_window, self->gc, bx,
                   y0 + h - bh - 16, bar_w, bh);
    XSetForeground(self->display, self->gc,
                   BlackPixel(self->display, DefaultScreen(self->display)));
    XDrawRectangle(self->display, self->gui_window, self->gc, bx, y0, bar_w,
                   h - 16);
    XDrawString(self->display, self->gui_window, self->gc, bx + 4, y0 + 12,
                labels[i], 1);
  }
}

static void ylc_gui_draw_ui_elements(ylc_plugin_t *self) {
  if (!self || self->ui_count == 0) {
    return;
  }
  int y = YLC_ARRAY_EDITOR_Y;
  for (uint32_t i = 0; i < self->ui_count; ++i) {
    ylc_ui_slot_t *slot = &self->ui_slots[i];
    int h, gap;
    switch (slot->kind) {
    case YLC_UI_ADSR:
    case YLC_UI_ENV:
      h = YLC_ARRAY_EDITOR_H;
      gap = YLC_ARRAY_EDITOR_GAP;
      slot->x = YLC_ARRAY_EDITOR_X;
      slot->y = y;
      slot->w = YLC_ARRAY_EDITOR_W;
      slot->h = h;
      if (slot->kind == YLC_UI_ADSR) {
        ylc_gui_draw_adsr(self, slot, i, slot->x, slot->y, slot->w, h);
      } else {
        ylc_gui_draw_env(self, slot, i, slot->x, slot->y, slot->w, h);
      }
      break;
    case YLC_UI_SOUNDFILE:
      h = YLC_SF_EDITOR_H;
      gap = YLC_SF_EDITOR_GAP;
      slot->x = YLC_ARRAY_EDITOR_X;
      slot->y = y;
      slot->w = YLC_ARRAY_EDITOR_W;
      slot->h = h;
      if (slot->soundfile) {
        ylc_gui_draw_soundfile(self, slot->soundfile, slot->x, slot->y, slot->w,
                               h);
      }
      break;
    default:
      h = 0;
      gap = 0;
      break;
    }
    y += h + gap;
  }
}

static void ylc_gui_handle_array_button(ylc_plugin_t *self,
                                        XButtonEvent *event) {
  if (!self || self->ui_count == 0) {
    return;
  }
  for (uint32_t i = 0; i < self->ui_count; ++i) {
    ylc_ui_slot_t *slot = &self->ui_slots[i];
    if (slot->kind != YLC_UI_ADSR && slot->kind != YLC_UI_ENV) {
      continue;
    }
    if (!ylc_point_in_rect(event->x, event->y, slot->x, slot->y, slot->w,
                           slot->h)) {
      continue;
    }
    if (slot->kind == YLC_UI_ADSR) {
      const double ranges[4] = {5.0, 5.0, 1.0, 5.0};
      int bar_w = slot->w / 4 - 4;
      for (int j = 0; j < 4; ++j) {
        int bx = slot->x + j * (bar_w + 4);
        if (event->x >= bx && event->x < bx + bar_w) {
          double frac =
              1.0 - (double)(event->y - slot->y) / (double)(slot->h - 16);
          frac = ylc_gui_clamp(frac, 0.0, 1.0);
          slot->array_values[j] = frac * ranges[j];
          self->gui_selected_array = (int)i;
          self->gui_selected_point = j;
          self->gui_dragging = true;
          ylc_mark_state_dirty(self);
          return;
        }
      }
    } else if (ylc_gui_array_is_env(slot)) {
      int pad = 8;
      int cx = slot->x + pad, cy = slot->y + pad;
      int cw = slot->w - 2 * pad, ch = slot->h - 2 * pad;
      double total = ylc_gui_env_total_x(slot) * 1.05;
      if (total <= 0.0) {
        total = 1.0;
      }
      int points = ylc_gui_env_point_count(slot);
      int best = -1;
      int best_d = 16 * 16;
      for (int p = 0; p < points; ++p) {
        int sx, sy;
        ylc_gui_env_to_screen(slot, p, total, cx, cy, cw, ch, &sx, &sy);
        int dx = event->x - sx, dy = event->y - sy;
        int d = dx * dx + dy * dy;
        if (d < best_d) {
          best_d = d;
          best = p;
        }
      }
      if (best >= 0) {
        self->gui_selected_array = (int)i;
        self->gui_selected_point = best;
        self->gui_dragging = true;
      }
    }
  }
}

static void ylc_gui_handle_array_motion(ylc_plugin_t *self,
                                        XMotionEvent *event) {
  if (!self || !self->gui_dragging || self->gui_selected_array < 0 ||
      (uint32_t)self->gui_selected_array >= self->ui_count) {
    return;
  }
  ylc_ui_slot_t *slot = &self->ui_slots[self->gui_selected_array];

  if (slot->kind == YLC_UI_ADSR) {
    const double ranges[4] = {5.0, 5.0, 1.0, 5.0};
    int bar_w = slot->w / 4 - 4;
    int j = self->gui_selected_point;
    if (j < 0 || j >= 4) {
      return;
    }
    double frac = 1.0 - (double)(event->y - slot->y) / (double)(slot->h - 16);
    frac = ylc_gui_clamp(frac, 0.0, 1.0);
    slot->array_values[j] = frac * ranges[j];
    ylc_mark_state_dirty(self);
  } else if (ylc_gui_array_is_env(slot)) {
    int pad = 8;
    int cx = slot->x + pad, cy = slot->y + pad;
    int cw = slot->w - 2 * pad, ch = slot->h - 2 * pad;
    double total = ylc_gui_env_total_x(slot) * 1.05;
    if (total <= 0.0) {
      total = 1.0;
    }
    int point = self->gui_selected_point;
    int points = ylc_gui_env_point_count(slot);
    if (point < 0 || point >= points) {
      return;
    }
    double data_x =
        ylc_gui_clamp((double)(event->x - cx) / cw, 0.0, 1.0) * total;
    double data_y = 1.0 - ylc_gui_clamp((double)(event->y - cy) / ch, 0.0, 1.0);
    *ylc_gui_env_val_ptr(slot, point) = ylc_gui_clamp(data_y, 0.0, 1.0);
    if (point > 0 && point < points - 1) {
      double *prev_dt = ylc_gui_env_time_ptr(slot, point - 1);
      double *next_dt = ylc_gui_env_time_ptr(slot, point);
      double cur_x = ylc_gui_env_point_x(slot, point);
      double delta = data_x - cur_x;
      double np = *prev_dt + delta, nn = *next_dt - delta;
      if (np >= 0.0 && nn >= 0.0) {
        *prev_dt = np;
        *next_dt = nn;
      }
    }
    ylc_mark_state_dirty(self);
  }
}

static void ylc_gui_handle_scroll(ylc_plugin_t *self, XButtonEvent *event) {
  if (!self || self->gui_selected_array < 0 ||
      (uint32_t)self->gui_selected_array >= self->ui_count) {
    return;
  }
  ylc_ui_slot_t *slot = &self->ui_slots[self->gui_selected_array];
  if (!ylc_gui_array_is_env(slot)) {
    return;
  }
  if (!ylc_point_in_rect(event->x, event->y, slot->x, slot->y, slot->w,
                         slot->h)) {
    return;
  }
  int point = self->gui_selected_point;
  int points = ylc_gui_env_point_count(slot);
  if (point <= 0 || point >= points) {
    return;
  }
  double *curve = ylc_gui_env_curve_ptr(slot, point - 1);
  double delta = (event->button == Button4) ? -0.1 : 0.1;
  *curve = ylc_gui_clamp(*curve + delta, -1.0, 1.0);
  ylc_mark_state_dirty(self);
}

static void ylc_dnd_intern_atoms(ylc_plugin_t *self) {
  Display *dpy = self->display;
  self->dnd_aware = XInternAtom(dpy, "XdndAware", False);
  self->dnd_enter = XInternAtom(dpy, "XdndEnter", False);
  self->dnd_position = XInternAtom(dpy, "XdndPosition", False);
  self->dnd_status = XInternAtom(dpy, "XdndStatus", False);
  self->dnd_drop = XInternAtom(dpy, "XdndDrop", False);
  self->dnd_leave = XInternAtom(dpy, "XdndLeave", False);
  self->dnd_finished = XInternAtom(dpy, "XdndFinished", False);
  self->dnd_selection = XInternAtom(dpy, "XdndSelection", False);
  self->dnd_action_copy = XInternAtom(dpy, "XdndActionCopy", False);
  self->dnd_uri_list = XInternAtom(dpy, "text/uri-list", False);
  self->dnd_property = XInternAtom(dpy, "YLC_XDND_DATA", False);
}

static void ylc_dnd_send(Display *dpy, Window target, Atom msg, long a0,
                         long a1, long a2, long a3, long a4) {
  XClientMessageEvent m;
  memset(&m, 0, sizeof(m));
  m.type = ClientMessage;
  m.display = dpy;
  m.window = target;
  m.message_type = msg;
  m.format = 32;
  m.data.l[0] = a0;
  m.data.l[1] = a1;
  m.data.l[2] = a2;
  m.data.l[3] = a3;
  m.data.l[4] = a4;
  XSendEvent(dpy, target, False, NoEventMask, (XEvent *)&m);
}

static void ylc_dnd_handle_clientmsg(ylc_plugin_t *self,
                                     const XClientMessageEvent *cm) {
  Display *dpy = self->display;
  Atom msg = (Atom)cm->message_type;
  if (msg == (Atom)self->dnd_enter) {
    self->dnd_source = (Window)cm->data.l[0];
  } else if (msg == (Atom)self->dnd_position) {
    self->dnd_source = (Window)cm->data.l[0];
    long pos = cm->data.l[2];
    self->dnd_mouse_x = (int)((pos >> 16) & 0xFFFF);
    self->dnd_mouse_y = (int)(pos & 0xFFFF);
    ylc_dnd_send(dpy, self->dnd_source, (Atom)self->dnd_status,
                 (long)self->gui_window, 1, 0, 0, (long)self->dnd_action_copy);
  } else if (msg == (Atom)self->dnd_drop) {
    self->dnd_source = (Window)cm->data.l[0];
    unsigned long time = (unsigned long)cm->data.l[2];
    XConvertSelection(dpy, (Atom)self->dnd_selection, (Atom)self->dnd_uri_list,
                      (Atom)self->dnd_property, self->gui_window,
                      time ? (Time)time : CurrentTime);
  } else if (msg == (Atom)self->dnd_leave) {
    self->dnd_source = 0;
  }
}

static void ylc_dnd_handle_selection(ylc_plugin_t *self,
                                     const XSelectionEvent *sel) {
  Display *dpy = self->display;
  if (sel->selection != (Atom)self->dnd_selection ||
      sel->target != (Atom)self->dnd_uri_list ||
      sel->property != (Atom)self->dnd_property) {
    return;
  }

  Atom type = 0;
  int fmt = 0;
  unsigned long n = 0, after = 0;
  unsigned char *data = NULL;
  if (XGetWindowProperty(dpy, self->gui_window, (Atom)self->dnd_property, 0,
                         (~0L), False, AnyPropertyType, &type, &fmt, &n, &after,
                         &data) == Success &&
      data) {
    ylc_debug_log(self, "drop received (%lu bytes)", n);
    const char *buf = (const char *)data;
    unsigned long i = 0;
    while (i < n) {
      while (i < n && (buf[i] == '\r' || buf[i] == '\n')) {
        i++;
      }
      unsigned long start = i;
      while (i < n && buf[i] != '\r' && buf[i] != '\n') {
        i++;
      }
      unsigned long len = i - start;
      if (len > 0) {
        char uri[1024];
        if (len >= sizeof(uri)) {
          len = sizeof(uri) - 1;
        }
        memcpy(uri, buf + start, len);
        uri[len] = '\0';
        ylc_debug_log(self, "  dropped: %s", uri);

        int win_x = 0, win_y = 0;
        Window child = 0;
        XTranslateCoordinates(dpy, DefaultRootWindow(dpy), self->gui_window,
                              self->dnd_mouse_x, self->dnd_mouse_y, &win_x,
                              &win_y, &child);
        ylc_soundfile_t *target = NULL;
        for (uint32_t s = 0; s < self->ui_count; ++s) {
          ylc_ui_slot_t *uslot = &self->ui_slots[s];
          if (uslot->kind != YLC_UI_SOUNDFILE || !uslot->soundfile) {
            continue;
          }
          if (win_x >= uslot->x && win_x <= uslot->x + uslot->w &&
              win_y >= uslot->y && win_y <= uslot->y + uslot->h) {
            target = uslot->soundfile;
            break;
          }
        }
        if (target) {
          ylc_soundfile_set_dropped_path(target, uri);
          ylc_debug_log(self, "  applied to soundfile: %s", target->path);
          ylc_mark_state_dirty(self);
        }
      }
    }
    XFree(data);
  }
  XDeleteProperty(dpy, self->gui_window, (Atom)self->dnd_property);
  if (self->dnd_source) {
    ylc_dnd_send(dpy, self->dnd_source, (Atom)self->dnd_finished,
                 (long)self->gui_window, 1, 0, 0, (long)self->dnd_action_copy);
  }
}

static void ylc_gui_draw_soundfile(ylc_plugin_t *self, ylc_soundfile_t *sf,
                                   int x0, int y0, int w, int h) {
  const unsigned long black =
      BlackPixel(self->display, DefaultScreen(self->display));
  const unsigned long white =
      WhitePixel(self->display, DefaultScreen(self->display));
  const unsigned long dark_bg = (24 << 16) | (24 << 8) | 28;
  const unsigned long green = (50 << 16) | (180 << 8) | 100;
  const unsigned long yellow = (255 << 16) | (200 << 8) | 0;

  XSetForeground(self->display, self->gc, dark_bg);
  XFillRectangle(self->display, self->gui_window, self->gc, x0, y0, w, h);
  XSetForeground(self->display, self->gc, black);
  XDrawRectangle(self->display, self->gui_window, self->gc, x0, y0, w, h);

  char label[256];
  if (sf->path[0] != '\0') {
    snprintf(label, sizeof(label), "%s", sf->path);
  } else {
    snprintf(label, sizeof(label), "SoundFile (drop a file here)");
  }
  XSetForeground(self->display, self->gc, white);
  XDrawString(self->display, self->gui_window, self->gc, x0 + 6, y0 + 14, label,
              (int)strlen(label));

  if (sf->loaded && sf->data && sf->frames > 0 && sf->channels > 0) {
    int ch = sf->channels;
    uint64_t total = sf->frames;
    int pad = 4;
    int cx = x0 + pad, cy = y0 + 20;
    int cw = w - 2 * pad, ch_h = h - 24;

    int display_samples = cw;
    double samples_per_px =
        (double)total / (double)(display_samples > 0 ? display_samples : 1);

    int midy = cy + ch_h / 2;
    XSetForeground(self->display, self->gc, green);
    for (int x = 0; x < display_samples; ++x) {
      uint64_t s0 = (uint64_t)((double)x * samples_per_px);
      uint64_t s1 = (uint64_t)((double)(x + 1) * samples_per_px);
      if (s1 <= s0) {
        s1 = s0 + 1;
      }
      if (s1 > total) {
        s1 = total;
      }
      double mn = 1.0, mx = -1.0;
      for (uint64_t s = s0; s < s1; ++s) {
        for (int c = 0; c < ch; ++c) {
          double v = sf->data[s * ch + c];
          if (v < mn) {
            mn = v;
          }
          if (v > mx) {
            mx = v;
          }
        }
      }
      int ymn = midy - (int)(mn * (ch_h / 2));
      int ymx = midy - (int)(mx * (ch_h / 2));
      int xpos = cx + x;
      if (ymn > ymx) {
        int tmp = ymn;
        ymn = ymx;
        ymx = tmp;
      }
      XDrawLine(self->display, self->gui_window, self->gc, xpos, ymn, xpos,
                ymx);
    }

    uint64_t rs = sf->region_start;
    uint64_t re = sf->region_end;
    if (rs >= total) {
      rs = 0;
    }
    if (re > total || re < rs) {
      re = total;
    }
    int rsx = cx + (int)(((double)rs / (double)total) * cw);
    int rex = cx + (int)(((double)re / (double)total) * cw);

    XSetForeground(self->display, self->gc, yellow);
    XDrawLine(self->display, self->gui_window, self->gc, rsx, cy, rsx,
              cy + ch_h);
    XDrawLine(self->display, self->gui_window, self->gc, rex, cy, rex,
              cy + ch_h);
  }
}

static void ylc_gui_handle_sf_button(ylc_plugin_t *self, XButtonEvent *event) {
  if (!self || self->ui_count == 0) {
    return;
  }
  for (uint32_t i = 0; i < self->ui_count; ++i) {
    ylc_ui_slot_t *slot = &self->ui_slots[i];
    if (slot->kind != YLC_UI_SOUNDFILE || !slot->soundfile) {
      continue;
    }
    ylc_soundfile_t *sf = slot->soundfile;
    if (!sf->loaded || sf->frames == 0) {
      continue;
    }
    if (!ylc_point_in_rect(event->x, event->y, slot->x, slot->y, slot->w,
                           slot->h)) {
      continue;
    }
    int pad = 4;
    int cx = slot->x + pad, cy = slot->y + 20;
    int cw = slot->w - 2 * pad;
    uint64_t total = sf->frames;
    int rsx = cx + (int)(((double)sf->region_start / (double)total) * cw);
    int rex = cx + (int)(((double)sf->region_end / (double)total) * cw);
    if (abs(event->x - rsx) < 8) {
      self->sf_dragging_edge = 0;
      self->gui_selected_array = (int32_t)i;
    } else if (abs(event->x - rex) < 8) {
      self->sf_dragging_edge = 1;
      self->gui_selected_array = (int32_t)i;
    } else if ((event->state & Mod1Mask) && event->x > rsx && event->x < rex) {
      self->sf_dragging_edge = 2;
      self->gui_selected_array = (int32_t)i;
      self->sf_drag_start_x = event->x;
      self->sf_drag_start_rs = sf->region_start;
      self->sf_drag_start_re = sf->region_end;
    }
  }
}

static void ylc_gui_handle_sf_motion(ylc_plugin_t *self, XMotionEvent *event) {
  if (!self || self->sf_dragging_edge < 0 || self->gui_selected_array < 0 ||
      (uint32_t)self->gui_selected_array >= self->ui_count) {
    return;
  }
  ylc_ui_slot_t *slot = &self->ui_slots[self->gui_selected_array];
  if (slot->kind != YLC_UI_SOUNDFILE || !slot->soundfile) {
    return;
  }
  ylc_soundfile_t *sf = slot->soundfile;
  if (!sf->loaded || sf->frames == 0) {
    return;
  }
  int x0 = slot->x;
  int w = slot->w;
  int pad = 4;
  int cx = x0 + pad;
  int cw = w - 2 * pad;
  double frac = (double)(event->x - cx) / (double)(cw > 0 ? cw : 1);
  frac = ylc_gui_clamp(frac, 0.0, 1.0);
  uint64_t frame = (uint64_t)(frac * (double)sf->frames);
  if (self->sf_dragging_edge == 2) {
    double px_per_frame = (double)cw / (double)sf->frames;
    int64_t delta_px = (int64_t)event->x - (int64_t)self->sf_drag_start_x;
    int64_t delta_frames =
        (int64_t)((double)delta_px / (px_per_frame > 0 ? px_per_frame : 1));
    int64_t width =
        (int64_t)self->sf_drag_start_re - (int64_t)self->sf_drag_start_rs;
    int64_t new_rs = (int64_t)self->sf_drag_start_rs + delta_frames;
    int64_t new_re = new_rs + width;
    if (new_rs < 0) {
      new_rs = 0;
      new_re = width;
    }
    if (new_re > (int64_t)sf->frames) {
      new_re = (int64_t)sf->frames;
      new_rs = new_re - width;
    }
    if (new_rs < 0) {
      new_rs = 0;
    }
    sf->region_start = (uint64_t)new_rs;
    sf->region_end = (uint64_t)new_re;
  } else if (self->sf_dragging_edge == 0) {
    if (frame < sf->region_end) {
      sf->region_start = frame;
    }
  } else {
    if (frame > sf->region_start) {
      sf->region_end = frame;
    }
  }
  ylc_mark_state_dirty(self);
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

  ylc_gui_draw_ui_elements(self);

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
  if (!self->path_focused) {
    ylc_gui_handle_array_button(self, event);
    ylc_gui_handle_sf_button(self, event);
  }
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
      if (event.xbutton.button == Button4 || event.xbutton.button == Button5) {
        ylc_gui_handle_scroll(self, &event.xbutton);
        ylc_gui_draw(self);
      } else {
        ylc_gui_handle_button(self, &event.xbutton);
      }
      break;
    case ButtonRelease:
      self->gui_dragging = false;
      self->sf_dragging_edge = -1;
      break;
    case MotionNotify:
      ylc_gui_handle_array_motion(self, &event.xmotion);
      ylc_gui_handle_sf_motion(self, &event.xmotion);
      ylc_gui_draw(self);
      break;
    case KeyPress:
      ylc_gui_handle_key(self, &event.xkey);
      break;
    case ClientMessage:
      ylc_dnd_handle_clientmsg(self, &event.xclient);
      break;
    case SelectionNotify:
      ylc_dnd_handle_selection(self, &event.xselection);
      ylc_gui_draw(self);
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
               ExposureMask | ButtonPressMask | ButtonReleaseMask |
                   PointerMotionMask | KeyPressMask | StructureNotifyMask);
  self->gc = XCreateGC(self->display, self->gui_window, 0, NULL);
  if (!self->gc) {
    XDestroyWindow(self->display, self->gui_window);
    self->gui_window = 0;
    return false;
  }

  ylc_dnd_intern_atoms(self);
  int dnd_version = 5;
  XChangeProperty(self->display, self->gui_window, (Atom)self->dnd_aware,
                  XA_ATOM, 32, PropModeReplace, (unsigned char *)&dnd_version,
                  1);

  self->sf_dragging_edge = -1;
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
