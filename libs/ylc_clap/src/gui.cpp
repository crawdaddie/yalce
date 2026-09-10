#include <algorithm>
#include <cmath>
#include "plugin_internal.h"
#undef min
#undef max
#include "debug.h"

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

typedef GLXContext (*ylc_pfn_glXCreateContextAttribsARB)(Display *, GLXFBConfig,
                                                         GLXContext, Bool,
                                                         const int *);

static double ylc_gui_clamp(double v, double lo, double hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

static bool ylc_gui_array_is_env(const ylc_array_ui_slot_t *s) {
  return s && s->kind == YLC_ARRAY_UI_ENV && s->values && s->count >= 4 &&
         ((s->count - 1) % 3) == 0;
}

static bool ylc_gui_array_is_adsr(const ylc_array_ui_slot_t *s) {
  return s && s->kind == YLC_ARRAY_UI_ADSR && s->values && s->count == 4;
}

static int ylc_gui_env_point_count(const ylc_array_ui_slot_t *s) {
  return ylc_gui_array_is_env(s) ? (int)((s->count + 2) / 3) : 0;
}

static double *ylc_gui_env_val_ptr(ylc_array_ui_slot_t *s, int p) {
  return s->values + (p * 3);
}

static double *ylc_gui_env_time_ptr(ylc_array_ui_slot_t *s, int seg) {
  return s->values + (seg * 3) + 1;
}

static double *ylc_gui_env_curve_ptr(ylc_array_ui_slot_t *s, int seg) {
  return s->values + (seg * 3) + 2;
}

static double ylc_gui_env_point_x(ylc_array_ui_slot_t *s, int p) {
  double x = 0.0;
  for (int i = 0; i < p; ++i) {
    x += *ylc_gui_env_time_ptr(s, i);
  }
  return x;
}

static double ylc_gui_env_total_x(ylc_array_ui_slot_t *s) {
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
        XTranslateCoordinates(dpy, DefaultRootWindow(dpy),
                              self->gui_window, self->dnd_mouse_x,
                              self->dnd_mouse_y, &win_x, &win_y, &child);
        ylc_soundfile_t *target = NULL;
        for (uint32_t s = 0; s < self->soundfile_ui_count; ++s) {
          if (!self->soundfile_ui_slots[s]) {
            continue;
          }
          float rx = self->sf_rects[s].x;
          float ry = self->sf_rects[s].y;
          float rw = self->sf_rects[s].w;
          float rh = self->sf_rects[s].h;
          if ((float)win_x >= rx && (float)win_x <= rx + rw &&
              (float)win_y >= ry && (float)win_y <= ry + rh) {
            target = self->soundfile_ui_slots[s];
            break;
          }
        }
        if (target) {
          ylc_soundfile_set_dropped_path(target, uri);
          ylc_debug_log(self, "  applied to soundfile: %s", target->path);
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

static ImGuiKey ylc_keysym_to_imgui_key(KeySym k) {
  switch (k) {
  case XK_Tab:
    return ImGuiKey_Tab;
  case XK_Left:
    return ImGuiKey_LeftArrow;
  case XK_Right:
    return ImGuiKey_RightArrow;
  case XK_Up:
    return ImGuiKey_UpArrow;
  case XK_Down:
    return ImGuiKey_DownArrow;
  case XK_Prior:
    return ImGuiKey_PageUp;
  case XK_Next:
    return ImGuiKey_PageDown;
  case XK_Home:
    return ImGuiKey_Home;
  case XK_End:
    return ImGuiKey_End;
  case XK_Insert:
    return ImGuiKey_Insert;
  case XK_Delete:
    return ImGuiKey_Delete;
  case XK_BackSpace:
    return ImGuiKey_Backspace;
  case XK_space:
    return ImGuiKey_Space;
  case XK_Return:
    return ImGuiKey_Enter;
  case XK_Escape:
    return ImGuiKey_Escape;
  case XK_Caps_Lock:
    return ImGuiKey_CapsLock;
  case XK_Shift_L:
    return ImGuiKey_LeftShift;
  case XK_Shift_R:
    return ImGuiKey_RightShift;
  case XK_Control_L:
    return ImGuiKey_LeftCtrl;
  case XK_Control_R:
    return ImGuiKey_RightCtrl;
  case XK_Alt_L:
    return ImGuiKey_LeftAlt;
  case XK_Alt_R:
    return ImGuiKey_RightAlt;
  case XK_Super_L:
    return ImGuiKey_LeftSuper;
  case XK_Super_R:
    return ImGuiKey_RightSuper;
  case XK_Menu:
    return ImGuiKey_Menu;
  case XK_F1:
    return ImGuiKey_F1;
  case XK_F2:
    return ImGuiKey_F2;
  case XK_F3:
    return ImGuiKey_F3;
  case XK_F4:
    return ImGuiKey_F4;
  case XK_F5:
    return ImGuiKey_F5;
  case XK_F6:
    return ImGuiKey_F6;
  case XK_F7:
    return ImGuiKey_F7;
  case XK_F8:
    return ImGuiKey_F8;
  case XK_F9:
    return ImGuiKey_F9;
  case XK_F10:
    return ImGuiKey_F10;
  case XK_F11:
    return ImGuiKey_F11;
  case XK_F12:
    return ImGuiKey_F12;
  case '0':
    return ImGuiKey_0;
  case '1':
    return ImGuiKey_1;
  case '2':
    return ImGuiKey_2;
  case '3':
    return ImGuiKey_3;
  case '4':
    return ImGuiKey_4;
  case '5':
    return ImGuiKey_5;
  case '6':
    return ImGuiKey_6;
  case '7':
    return ImGuiKey_7;
  case '8':
    return ImGuiKey_8;
  case '9':
    return ImGuiKey_9;
  case 'A':
    return ImGuiKey_A;
  case 'B':
    return ImGuiKey_B;
  case 'C':
    return ImGuiKey_C;
  case 'D':
    return ImGuiKey_D;
  case 'E':
    return ImGuiKey_E;
  case 'F':
    return ImGuiKey_F;
  case 'G':
    return ImGuiKey_G;
  case 'H':
    return ImGuiKey_H;
  case 'I':
    return ImGuiKey_I;
  case 'J':
    return ImGuiKey_J;
  case 'K':
    return ImGuiKey_K;
  case 'L':
    return ImGuiKey_L;
  case 'M':
    return ImGuiKey_M;
  case 'N':
    return ImGuiKey_N;
  case 'O':
    return ImGuiKey_O;
  case 'P':
    return ImGuiKey_P;
  case 'Q':
    return ImGuiKey_Q;
  case 'R':
    return ImGuiKey_R;
  case 'S':
    return ImGuiKey_S;
  case 'T':
    return ImGuiKey_T;
  case 'U':
    return ImGuiKey_U;
  case 'V':
    return ImGuiKey_V;
  case 'W':
    return ImGuiKey_W;
  case 'X':
    return ImGuiKey_X;
  case 'Y':
    return ImGuiKey_Y;
  case 'Z':
    return ImGuiKey_Z;
  default:
    return ImGuiKey_None;
  }
}

static bool ylc_is_key_repeat(Display *dpy, XEvent *ev) {
  if (XEventsQueued(dpy, QueuedAfterReading) > 0) {
    XEvent next;
    XPeekEvent(dpy, &next);
    if (next.type == KeyPress && next.xkey.time == ev->xkey.time &&
        next.xkey.keycode == ev->xkey.keycode) {
      return true;
    }
  }
  return false;
}

static void imgui_x11_init(void) {
  ImGuiIO &io = ImGui::GetIO();
  io.BackendPlatformName = "imgui_impl_x11 (ylc)";
}

static double g_last_time = 0.0;

static void imgui_x11_new_frame(ylc_plugin_t *self) {
  ImGuiIO &io = ImGui::GetIO();
  int w = self->gui_width > 0 ? self->gui_width : YLC_GUI_WIDTH;
  int h = self->gui_height > 0 ? self->gui_height : YLC_GUI_HEIGHT;
  io.DisplaySize = ImVec2((float)w, (float)h);
  io.DisplayFramebufferScale = ImVec2(1, 1);
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double now = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
  io.DeltaTime = (float)(now - g_last_time);
  if (g_last_time == 0.0 || io.DeltaTime <= 0.0f || io.DeltaTime > 1.0f) {
    io.DeltaTime = 1.0f / 60.0f;
  }
  g_last_time = now;
}

static void imgui_x11_handle_event(ylc_plugin_t *self, XEvent *ev) {
  ImGuiIO &io = ImGui::GetIO();
  switch (ev->type) {
  case MotionNotify:
    io.AddMousePosEvent((float)ev->xmotion.x, (float)ev->xmotion.y);
    break;
  case ButtonPress: {
    int b = ev->xbutton.button;
    if (b == Button1) {
      io.AddMouseButtonEvent(0, true);
    } else if (b == Button3) {
      io.AddMouseButtonEvent(1, true);
    } else if (b == Button2) {
      io.AddMouseButtonEvent(2, true);
    } else if (b == Button4) {
      io.AddMouseWheelEvent(0.0f, +1.0f);
    } else if (b == Button5) {
      io.AddMouseWheelEvent(0.0f, -1.0f);
    }
    break;
  }
  case ButtonRelease: {
    int b = ev->xbutton.button;
    if (b == Button1) {
      io.AddMouseButtonEvent(0, false);
    } else if (b == Button3) {
      io.AddMouseButtonEvent(1, false);
    } else if (b == Button2) {
      io.AddMouseButtonEvent(2, false);
    }
    break;
  }
  case KeyPress: {
    char buf[32];
    KeySym ks = 0;
    int n = XLookupString(&ev->xkey, buf, sizeof(buf), &ks, NULL);
    ImGuiKey key = ylc_keysym_to_imgui_key(ks);
    if (key != ImGuiKey_None) {
      io.AddKeyEvent(key, true);
    }
    if (n > 0) {
      buf[n] = 0;
      io.AddInputCharactersUTF8(buf);
    }
    break;
  }
  case KeyRelease: {
    if (ylc_is_key_repeat(self->display, ev)) {
      break;
    }
    KeySym ks = XLookupKeysym(&ev->xkey, 0);
    ImGuiKey key = ylc_keysym_to_imgui_key(ks);
    if (key != ImGuiKey_None) {
      io.AddKeyEvent(key, false);
    }
    break;
  }
  case FocusIn:
    io.AddFocusEvent(true);
    break;
  case FocusOut:
    io.AddFocusEvent(false);
    break;
  case LeaveNotify:
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    break;
  case ConfigureNotify:
    self->gui_width = ev->xconfigure.width;
    self->gui_height = ev->xconfigure.height;
    break;
  case ClientMessage:
    ylc_dnd_handle_clientmsg(self, &ev->xclient);
    break;
  case SelectionNotify:
    ylc_dnd_handle_selection(self, &ev->xselection);
    break;
  default:
    break;
  }
}

static GLXContext ylc_create_gl_context(Display *dpy, GLXFBConfig fbc) {
  ylc_pfn_glXCreateContextAttribsARB pfn =
      (ylc_pfn_glXCreateContextAttribsARB)glXGetProcAddressARB(
          (const GLubyte *)"glXCreateContextAttribsARB");
  if (pfn) {
    int attr[] = {GLX_CONTEXT_MAJOR_VERSION_ARB,
                  3,
                  GLX_CONTEXT_MINOR_VERSION_ARB,
                  3,
                  GLX_CONTEXT_PROFILE_MASK_ARB,
                  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                  None};
    GLXContext ctx = pfn(dpy, fbc, 0, True, attr);
    if (ctx) {
      return ctx;
    }
  }
  return glXCreateNewContext(dpy, fbc, GLX_RGBA_TYPE, 0, True);
}

static bool ylc_gui_create_window(ylc_plugin_t *self) {
  if (!self || !self->display || self->gui_window || !self->parent_window) {
    return self && self->gui_window != 0;
  }

  Display *dpy = self->display;
  int fbcount = 0;
  int fbattr[] = {GLX_DOUBLEBUFFER,
                  True,
                  GLX_RED_SIZE,
                  8,
                  GLX_GREEN_SIZE,
                  8,
                  GLX_BLUE_SIZE,
                  8,
                  GLX_ALPHA_SIZE,
                  8,
                  GLX_DEPTH_SIZE,
                  24,
                  None};
  GLXFBConfig *fbc =
      glXChooseFBConfig(dpy, DefaultScreen(dpy), fbattr, &fbcount);
  if (!fbc || fbcount == 0) {
    ylc_debug_log(self, "glXChooseFBConfig failed");
    return false;
  }
  GLXFBConfig config = fbc[0];
  XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, config);
  if (!vi) {
    XFree(fbc);
    ylc_debug_log(self, "glXGetVisualFromFBConfig failed");
    return false;
  }

  XSetWindowAttributes wa;
  wa.colormap =
      XCreateColormap(dpy, self->parent_window, vi->visual, AllocNone);
  wa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
                  PointerMotionMask | ButtonMotionMask | KeyPressMask |
                  KeyReleaseMask | StructureNotifyMask | FocusChangeMask |
                  EnterWindowMask | LeaveWindowMask;
  wa.border_pixel = 0;
  self->gui_window =
      XCreateWindow(dpy, self->parent_window, 0, 0, YLC_GUI_WIDTH,
                    YLC_GUI_HEIGHT, 0, vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask | CWBorderPixel, &wa);
  XFree(vi);
  if (!self->gui_window) {
    XFree(fbc);
    ylc_debug_log(self, "XCreateWindow failed");
    return false;
  }
  self->gui_width = YLC_GUI_WIDTH;
  self->gui_height = YLC_GUI_HEIGHT;

  ylc_dnd_intern_atoms(self);
  int dndver = 5;
  XChangeProperty(dpy, self->gui_window, (Atom)self->dnd_aware, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&dndver, 1);

  GLXContext ctx = ylc_create_gl_context(dpy, config);
  XFree(fbc);
  if (!ctx) {
    ylc_debug_log(self, "GL context creation failed");
    XDestroyWindow(dpy, self->gui_window);
    self->gui_window = 0;
    return false;
  }
  self->glctx = (void *)ctx;
  glXMakeCurrent(dpy, self->gui_window, ctx);

  ImGuiContext *ic = ImGui::CreateContext();
  self->imgui_ctx = (void *)ic;
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = NULL;
  ImGui::StyleColorsDark();
  imgui_x11_init();
  bool gl_init_ok = ImGui_ImplOpenGL3_Init("#version 150");
  ylc_debug_log(self, "gui window + GL + ImGui ready (gl_init=%d)",
                gl_init_ok ? 1 : 0);
  if (!gl_init_ok) {
    ylc_debug_log(self, "ImGui_ImplOpenGL3_Init failed");
  }
  return true;
}

static void ylc_gui_build_env(ylc_plugin_t *self, ylc_array_ui_slot_t *slot,
                              uint32_t index) {
  if (!ylc_gui_array_is_env(slot)) {
    ImGui::TextWrapped(
        "EnvArrayUI expects [value, duration, curve, value, ...]");
    return;
  }
  int points = ylc_gui_env_point_count(slot);
  double total = ylc_gui_env_total_x(slot) * 1.05;
  if (total <= 0.0) {
    total = 1.0;
  }
  ImVec2 canvas = ImVec2(ImGui::GetContentRegionAvail().x, 140.0f);
  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(origin, ImVec2(origin.x + canvas.x, origin.y + canvas.y),
                    IM_COL32(32, 32, 32, 255));
  dl->AddRect(origin, ImVec2(origin.x + canvas.x, origin.y + canvas.y),
              IM_COL32(110, 110, 110, 255));
  float px = origin.x + 12, py = origin.y + 12;
  float pw = canvas.x - 24, ph = canvas.y - 24;
  auto to_s = [&](double x, double y) -> ImVec2 {
    return ImVec2(px + (float)(x / total) * pw,
                  py + (float)(1.0 - ylc_gui_clamp(y, 0.0, 1.0)) * ph);
  };
  for (int i = 0; i < points - 1; ++i) {
    double x0 = ylc_gui_env_point_x(slot, i);
    double y0 = *ylc_gui_env_val_ptr(slot, i);
    double x1 = ylc_gui_env_point_x(slot, i + 1);
    double y1 = *ylc_gui_env_val_ptr(slot, i + 1);
    double curve = *ylc_gui_env_curve_ptr(slot, i);
    ImVec2 a = to_s(x0, y0);
    const int seg = 24;
    for (int j = 1; j <= seg; ++j) {
      double t = (double)j / seg;
      ImVec2 b = to_s(x0 + (x1 - x0) * t, ylc_gui_env_interp(t, y0, y1, curve));
      dl->AddLine(a, b, IM_COL32(80, 200, 120, 255), 2.0f);
      a = b;
    }
  }
  for (int i = 0; i < points; ++i) {
    ImVec2 q =
        to_s(ylc_gui_env_point_x(slot, i), *ylc_gui_env_val_ptr(slot, i));
    ImU32 col = ((int)index == self->gui_selected_array &&
                 i == self->gui_selected_point)
                    ? IM_COL32(255, 255, 0, 255)
                    : IM_COL32(200, 200, 80, 255);
    dl->AddCircleFilled(q, 5.0f, col);
  }
  ImGui::InvisibleButton("##envcanvas", canvas);
  ImGuiIO &io = ImGui::GetIO();
  if (ImGui::IsItemHovered()) {
    if (ImGui::IsMouseClicked(0)) {
      float mx = io.MousePos.x, my = io.MousePos.y;
      float bd = 16.0f * 16.0f;
      int best = -1;
      for (int i = 0; i < points; ++i) {
        ImVec2 q =
            to_s(ylc_gui_env_point_x(slot, i), *ylc_gui_env_val_ptr(slot, i));
        float dx = q.x - mx, dy = q.y - my;
        float d = dx * dx + dy * dy;
        if (d < bd) {
          bd = d;
          best = i;
        }
      }
      if (best >= 0) {
        self->gui_selected_array = (int)index;
        self->gui_selected_point = best;
      }
    }
    if (io.MouseWheel != 0.0f && self->gui_selected_array == (int)index &&
        self->gui_selected_point > 0) {
      double *curve = ylc_gui_env_curve_ptr(slot, self->gui_selected_point - 1);
      *curve += io.MouseWheel > 0 ? -0.1 : 0.1;
      ylc_mark_state_dirty(self);
    }
  }
  if (self->gui_selected_array == (int)index && self->gui_selected_point >= 0 &&
      self->gui_selected_point < points && ImGui::IsItemActive()) {
    int point = self->gui_selected_point;
    double nx = (double)(io.MousePos.x - px) / (double)pw;
    double ny = (double)(io.MousePos.y - py) / (double)ph;
    double data_x = ylc_gui_clamp(nx, 0.0, 1.0) * total;
    double data_y = 1.0 - ylc_gui_clamp(ny, 0.0, 1.0);
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
  ImGui::Spacing();
}

static void ylc_gui_build_adsr(ylc_plugin_t *self, ylc_array_ui_slot_t *slot,
                               uint32_t index) {
  if (!ylc_gui_array_is_adsr(slot)) {
    ImGui::TextWrapped("ADSRArrayUI expects [attack, decay, sustain, release]");
    return;
  }
  const char *labels[4] = {"Attack", "Decay", "Sustain", "Release"};
  const double ranges[4] = {5.0, 5.0, 1.0, 5.0};
  for (int i = 0; i < 4; ++i) {
    double lo = 0.0, hi = ranges[i];
    if (ImGui::SliderScalar(labels[i], ImGuiDataType_Double, &slot->values[i],
                            &lo, &hi, "%.3g")) {
      ylc_mark_state_dirty(self);
      self->gui_selected_array = (int)index;
      self->gui_selected_point = i;
    }
  }
}

static void ylc_gui_build_arrays(ylc_plugin_t *self) {
  if (!self || self->array_ui_count == 0) {
    return;
  }
  for (uint32_t i = 0; i < self->array_ui_count; ++i) {
    ylc_array_ui_slot_t *slot = &self->array_ui_slots[i];
    ImGui::PushID((int)i);
    ImGui::Text("%s %u", slot->kind == YLC_ARRAY_UI_ADSR ? "ADSR" : "Envelope",
                i + 1);
    if (slot->kind == YLC_ARRAY_UI_ADSR) {
      ylc_gui_build_adsr(self, slot, i);
    } else {
      ylc_gui_build_env(self, slot, i);
    }
    ImGui::PopID();
    ImGui::Separator();
  }
}

static void ylc_gui_build_soundfile(ylc_plugin_t *self,
                                     ylc_soundfile_t *sf, uint32_t index) {
  ImGui::PushID((int)(index | 0x80000000));

  if (sf->path[0] != '\0') {
    ImGui::Text("SoundFile: %s", sf->path);
  } else {
    ImGui::Text("SoundFile %u: (drop a file here)", index + 1);
  }
  if (sf->loaded && sf->frames > 0) {
    ImGui::SameLine();
    ImGui::TextDisabled("(%lu frames, %d ch, %d Hz)",
                        (unsigned long)sf->frames, sf->channels,
                        sf->samplerate);
  }

  float canvas_h = 120.0f;
  ImVec2 canvas = ImVec2(ImGui::GetContentRegionAvail().x, canvas_h);
  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImDrawList *dl = ImGui::GetWindowDrawList();

  if (index < YLC_SOUNDFILE_UI_MAX_SLOTS) {
    self->sf_rects[index].x = origin.x;
    self->sf_rects[index].y = origin.y;
    self->sf_rects[index].w = canvas.x;
    self->sf_rects[index].h = canvas_h;
  }

  dl->AddRectFilled(origin, ImVec2(origin.x + canvas.x, origin.y + canvas.y),
                    IM_COL32(24, 24, 28, 255));
  dl->AddRect(origin, ImVec2(origin.x + canvas.x, origin.y + canvas.y),
              IM_COL32(90, 90, 90, 255));

  if (sf->loaded && sf->data && sf->frames > 0 && sf->channels > 0) {
    int ch = sf->channels;
    uint64_t total = sf->frames;
    uint64_t start = sf->region_start;
    uint64_t end = sf->region_end;
    if (start >= total) start = 0;
    if (end > total || end < start) end = total;

    float px = origin.x + 4, py = origin.y + 4;
    float pw = canvas.x - 8, ph = canvas_h - 8;
    float midy = py + ph * 0.5f;

    int display_samples = (int)(pw < 1 ? 1 : pw);
    double samples_per_px = (double)total / (double)display_samples;

    for (int x = 0; x < display_samples; ++x) {
      uint64_t s0 = (uint64_t)((double)x * samples_per_px);
      uint64_t s1 = (uint64_t)((double)(x + 1) * samples_per_px);
      if (s1 <= s0) s1 = s0 + 1;
      if (s1 > total) s1 = total;

      double mn = 1.0, mx = -1.0;
      for (uint64_t s = s0; s < s1; ++s) {
        for (int c = 0; c < ch; ++c) {
          double v = sf->data[s * ch + c];
          if (v < mn) mn = v;
          if (v > mx) mx = v;
        }
      }
      float ypos_mn = midy - (float)mn * (ph * 0.5f);
      float ypos_mx = midy - (float)mx * (ph * 0.5f);
      float xpos = px + (float)x;
      dl->AddLine(ImVec2(xpos, ypos_mn), ImVec2(xpos, ypos_mx),
                  IM_COL32(80, 180, 120, 255));
    }

    float rsx = px + (float)((double)start / (double)total) * pw;
    float rex = px + (float)((double)end / (double)total) * pw;
    dl->AddRectFilled(ImVec2(rsx, py), ImVec2(rex, py + ph),
                      IM_COL32(255, 200, 0, 40));
    dl->AddLine(ImVec2(rsx, py), ImVec2(rsx, py + ph),
                IM_COL32(255, 200, 0, 255), 2.0f);
    dl->AddLine(ImVec2(rex, py), ImVec2(rex, py + ph),
                IM_COL32(255, 200, 0, 255), 2.0f);
  }

  ImGui::InvisibleButton("##sfcanvas", canvas);
  ImGuiIO &io = ImGui::GetIO();
  if (ImGui::IsItemHovered() && sf->loaded && sf->frames > 0) {
    float mx = io.MousePos.x - origin.x - 4;
    float pw = canvas.x - 8;
    double frac = (double)(mx < 0 ? 0 : (mx > pw ? pw : mx)) / (double)(pw < 1 ? 1 : pw);
    uint64_t frame = (uint64_t)(frac * (double)sf->frames);

    if (ImGui::IsMouseClicked(0)) {
      float rsx = (float)((double)sf->region_start / (double)sf->frames) * pw;
      float rex = (float)((double)sf->region_end / (double)sf->frames) * pw;
      if (fabsf(mx - rsx) < 8.0f) {
        self->gui_dragging = true;
        self->gui_selected_array = (int32_t)index;
        self->gui_selected_point = 0;
      } else if (fabsf(mx - rex) < 8.0f) {
        self->gui_dragging = true;
        self->gui_selected_array = (int32_t)index;
        self->gui_selected_point = 1;
      }
    }
    if (self->gui_dragging && self->gui_selected_array == (int32_t)index &&
        ImGui::IsMouseDown(0)) {
      if (self->gui_selected_point == 0) {
        if (frame < sf->region_end) sf->region_start = frame;
      } else {
        if (frame > sf->region_start) sf->region_end = frame;
      }
    }
  }
  if (ImGui::IsMouseReleased(0)) {
    self->gui_dragging = false;
  }

  ImGui::PopID();
  ImGui::Separator();
}

static void ylc_gui_build_soundfiles(ylc_plugin_t *self) {
  if (!self || self->soundfile_ui_count == 0) {
    return;
  }
  for (uint32_t i = 0; i < self->soundfile_ui_count; ++i) {
    if (self->soundfile_ui_slots[i]) {
      ylc_gui_build_soundfile(self, self->soundfile_ui_slots[i], i);
    }
  }
}

static void ylc_gui_build_widgets(ylc_plugin_t *self) {
  const ImGuiViewport *vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  ImGui::Begin("YLC", NULL, flags);

  ImGui::Text("Instance %u", self->instance_id);
  ImGui::Separator();

  ImGui::Text("Script path");
  ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::InputText("##path", self->script_path, sizeof(self->script_path),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    ylc_setup_script_watcher(self);
    ylc_spawn_editor(self);
  }
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    ylc_mark_state_dirty(self);
  }
  ImGui::PopItemWidth();

  if (ImGui::Button("Open in nvim")) {
    ylc_setup_script_watcher(self);
    ylc_spawn_editor(self);
  }
  ImGui::SameLine();
  if (ImGui::Button("Follow log")) {
    ylc_spawn_log_follower(self);
  }

  bool reload_pending =
      atomic_load_explicit(&self->script_reload_pending, memory_order_acquire);
  const char *status =
      reload_pending
          ? "Script changed: reload pending"
          : (self->debug_log_path[0] != '\0'
                 ? "Watching script file; log follower available"
                 : "Watching script file; set YLC_DEBUG_LOG for log");
  ImGui::TextWrapped("%s", status);
  ImGui::Separator();

  ylc_gui_build_arrays(self);
  ylc_gui_build_soundfiles(self);
  ImGui::End();
}

static void ylc_gui_render(ylc_plugin_t *self) {
  if (!self || !self->display || !self->gui_window || !self->glctx ||
      !self->imgui_ctx) {
    return;
  }
  Display *dpy = self->display;
  glXMakeCurrent(dpy, self->gui_window, (GLXContext)self->glctx);
  ImGui::SetCurrentContext((ImGuiContext *)self->imgui_ctx);
  ImGui_ImplOpenGL3_NewFrame();
  imgui_x11_new_frame(self);
  ImGui::NewFrame();
  ylc_gui_build_widgets(self);
  ImGui::Render();
  int w = self->gui_width > 0 ? self->gui_width : YLC_GUI_WIDTH;
  int h = self->gui_height > 0 ? self->gui_height : YLC_GUI_HEIGHT;
  glViewport(0, 0, w, h);
  glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glXSwapBuffers(dpy, self->gui_window);
}

void ylc_gui_draw(ylc_plugin_t *self) {
  if (!self || !self->gui_visible) {
    return;
  }
  ylc_gui_render(self);
}

void ylc_gui_poll_events(ylc_plugin_t *self) {
  if (!self || !self->display || !self->gui_window) {
    return;
  }
  ImGui::SetCurrentContext((ImGuiContext *)self->imgui_ctx);
  Display *dpy = self->display;
  while (XPending(dpy) > 0) {
    XEvent ev;
    XNextEvent(dpy, &ev);
    if (ev.xany.window == self->gui_window) {
      imgui_x11_handle_event(self, &ev);
    }
  }
  if (self->gui_visible) {
    ylc_gui_render(self);
  }
}

void ylc_gui_close(ylc_plugin_t *self) {
  if (!self || !self->display) {
    return;
  }
  Display *dpy = self->display;
  if (self->imgui_ctx) {
    ImGui::SetCurrentContext((ImGuiContext *)self->imgui_ctx);
    if (self->gui_window && self->glctx) {
      glXMakeCurrent(dpy, self->gui_window, (GLXContext)self->glctx);
      ImGui_ImplOpenGL3_Shutdown();
    }
    ImGui::DestroyContext((ImGuiContext *)self->imgui_ctx);
    self->imgui_ctx = NULL;
  }
  if (self->glctx) {
    glXMakeCurrent(dpy, 0, 0);
    glXDestroyContext(dpy, (GLXContext)self->glctx);
    self->glctx = NULL;
  }
  if (self->gui_window) {
    XDestroyWindow(dpy, self->gui_window);
    self->gui_window = 0;
  }
  if (self->gc) {
    XFreeGC(dpy, self->gc);
    self->gc = 0;
  }
  XCloseDisplay(dpy);
  self->display = NULL;
  self->parent_window = 0;
  self->gui_created = false;
  self->gui_visible = false;
  self->path_focused = false;
  self->gui_width = 0;
  self->gui_height = 0;
  g_last_time = 0.0;
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
  ylc_gui_render(self);
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
