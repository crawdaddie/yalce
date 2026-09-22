#ifndef GFX_H
#define GFX_H

#include "../../lang/ylc_datatypes.h"
#include <SDL3/SDL_events.h>
#include <stdint.h>

typedef struct {
  int32_t x;
  int32_t y;
} YLCIntIntTuple;

int ylc_gfx_init(void);
int ylc_gfx_run(void);
void ylc_gfx_stop(void);

void gfx_begin_frame(void);
void gfx_end_frame(void);
void gfx_clear(double r, double g, double b, double a);

void *gfx_screen(void);
void *gfx_target(int width, int height);
void gfx_bind_target(void *target);
void *gfx_framebuffer(int width, int height);
void gfx_framebuffer_write(void *framebuffer, _String pixels);
void gfx_framebuffer_draw(void *framebuffer, int scale);

void *gfx_fullscreen_quad(void);
void *gfx_mesh(_DoubleArray vertices, int stride);
void *gfx_points(_DoubleArray vertices, int stride);
void *gfx_lines(_DoubleArray vertices, int stride);
void *gfx_shader(_String vert, _String frag);
void *gfx_compute_shader(_String src);
void gfx_use_shader(void *shader);
void gfx_use_compute_shader(void *shader);
void gfx_uniform1f(void *shader, _String name, double x);
void gfx_uniform2f(void *shader, _String name, double x, double y);
void gfx_uniform_mat4(void *shader, _String name, _DoubleArray matrix);
void gfx_uniform_tex(void *shader, _String name, void *target, int unit);
void gfx_bind_image_texture(void *target, int unit, int access, int format);
void gfx_dispatch_compute(int x, int y, int z);
void gfx_memory_barrier(void);
void gfx_draw_mesh(void *mesh);

double gfx_time(void);
double gfx_dt(void);
YLCIntIntTuple gfx_resolution(void);
void gfx_run(void *cb);

void gfx_register_sdl_event_handler(_String name, void (*cb)(SDL_Event *));
int gfx_event_type(SDL_Event *event);
int gfx_event_key(SDL_Event *event);
int gfx_event_mouse_x(SDL_Event *event);
int gfx_event_mouse_y(SDL_Event *event);
int gfx_event_button(SDL_Event *event);
int gfx_event_wheel_y(SDL_Event *event);
#endif
