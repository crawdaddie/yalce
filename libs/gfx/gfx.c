#include "./gfx.h"

#include "../../lang/common.h"
#define GL_GLEXT_PROTOTYPES
#include "../../lang/ylc_datatypes.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GFX_RGBA_BYTES 4

typedef void (*GfxFrameFn)(void);

typedef struct {
  GLuint vao;
  GLuint vbo;
  int vertex_count;
  GLenum mode;
} GfxMesh;

typedef struct {
  GLuint program;
} GfxShader;

typedef struct {
  GLuint fbo;
  GLuint color_tex;
  int width;
  int height;
  bool is_screen;
} GfxTarget;

typedef struct {
  GLuint texture;
  int width;
  int height;
} GfxFramebuffer;

typedef struct {
  int32_t width;
  int32_t height;
} GfxResolution;

typedef struct EventHandler {
  const char *name;
  void (*fn)(SDL_Event *);
  struct EventHandler *next;
} EventHandler;

typedef struct {
  SDL_Window *window;
  SDL_GLContext gl_context;
  bool initialized;
  bool running;
  Uint64 perf_freq;
  Uint64 last_counter;
  double time;
  double dt;
  GfxFrameFn frame_fn;
  GfxMesh fullscreen_quad;
  bool quad_ready;
  EventHandler *handlers;
} GfxRuntime;

static GfxRuntime gfx_runtime;
static GfxResolution gfx_resolution_value;
static GfxTarget gfx_screen_target = {
    .fbo = 0, .color_tex = 0, .width = 0, .height = 0, .is_screen = true};
static GLuint gfx_framebuffer_shader_program;

static void ylc_gfx_close(void) {
  if (gfx_runtime.gl_context) {
    SDL_GL_DestroyContext(gfx_runtime.gl_context);
    gfx_runtime.gl_context = NULL;
  }
  if (gfx_runtime.window) {
    SDL_DestroyWindow(gfx_runtime.window);
    gfx_runtime.window = NULL;
  }
  if (gfx_runtime.initialized) {
    SDL_Quit();
    gfx_runtime.initialized = false;
  }
}

static void gfx_log_gl_error(const char *where) {
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    fprintf(stderr, "libgfx: GL error 0x%x at %s\n", (unsigned)err, where);
  }
}

static GLuint gfx_compile_shader(const char *src, GLenum type) {
  GLuint shader = glCreateShader(type);
  GLint ok = 0;
  char log[1024];

  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    glGetShaderInfoLog(shader, (GLsizei)sizeof(log), NULL, log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static GLuint gfx_framebuffer_shader(void) {
  static const char *vert =
      "#version 330 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "out vec2 uv;\n"
      "void main() {\n"
      "  uv = vec2(aPos.x * 0.5 + 0.5, 1.0 - (aPos.y * 0.5 + 0.5));\n"
      "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
      "}\n";
  static const char *frag = "#version 330 core\n"
                            "in vec2 uv;\n"
                            "uniform sampler2D uTex;\n"
                            "out vec4 fragColor;\n"
                            "void main() {\n"
                            "  fragColor = texture(uTex, uv);\n"
                            "}\n";
  GLuint vs;
  GLuint fs;
  GLint ok = 0;
  char log[1024];

  if (gfx_framebuffer_shader_program) {
    return gfx_framebuffer_shader_program;
  }

  vs = gfx_compile_shader(vert, GL_VERTEX_SHADER);
  fs = gfx_compile_shader(frag, GL_FRAGMENT_SHADER);
  if (!vs || !fs) {
    if (vs) {
      glDeleteShader(vs);
    }
    if (fs) {
      glDeleteShader(fs);
    }
    return 0;
  }

  gfx_framebuffer_shader_program = glCreateProgram();
  glAttachShader(gfx_framebuffer_shader_program, vs);
  glAttachShader(gfx_framebuffer_shader_program, fs);
  glLinkProgram(gfx_framebuffer_shader_program);
  glGetProgramiv(gfx_framebuffer_shader_program, GL_LINK_STATUS, &ok);
  glDeleteShader(vs);
  glDeleteShader(fs);

  if (!ok) {
    glGetProgramInfoLog(gfx_framebuffer_shader_program, (GLsizei)sizeof(log),
                        NULL, log);
    fprintf(stderr, "libgfx: framebuffer shader link failed: %s\n", log);
    glDeleteProgram(gfx_framebuffer_shader_program);
    gfx_framebuffer_shader_program = 0;
  }

  return gfx_framebuffer_shader_program;
}

static GLuint gfx_texture_id(void *target_ptr) {
  GfxTarget *target = target_ptr;
  if (!target || target->is_screen) {
    return 0;
  }
  return target->color_tex;
}

int ylc_gfx_init(void) {
  if (gfx_runtime.initialized) {
    return 0;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "libgfx: SDL_Init failed: %s\n", SDL_GetError());
    return -1;
  }

  if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) ||
      !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) ||
      !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                           SDL_GL_CONTEXT_PROFILE_CORE) ||
      !SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
      !SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)) {
    fprintf(stderr, "libgfx: failed to configure GL attributes: %s\n",
            SDL_GetError());
    SDL_Quit();
    return -1;
  }

  gfx_runtime.window =
      SDL_CreateWindow("yalce gfx", 1024, 768,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (!gfx_runtime.window) {
    fprintf(stderr, "libgfx: SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  gfx_runtime.gl_context = SDL_GL_CreateContext(gfx_runtime.window);
  if (!gfx_runtime.gl_context) {
    fprintf(stderr, "libgfx: SDL_GL_CreateContext failed: %s\n",
            SDL_GetError());
    ylc_gfx_close();
    return -1;
  }

  if (!SDL_GL_MakeCurrent(gfx_runtime.window, gfx_runtime.gl_context)) {
    fprintf(stderr, "libgfx: SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
    ylc_gfx_close();
    return -1;
  }

  gfx_runtime.initialized = true;
  gfx_runtime.running = true;
  gfx_runtime.perf_freq = SDL_GetPerformanceFrequency();
  gfx_runtime.last_counter = SDL_GetPerformanceCounter();
  SDL_GetWindowSize(gfx_runtime.window, &gfx_resolution_value.width,
                    &gfx_resolution_value.height);
  gfx_screen_target.width = gfx_resolution_value.width;
  gfx_screen_target.height = gfx_resolution_value.height;
  fprintf(stderr, "libgfx: GL_VERSION=%s\n", glGetString(GL_VERSION));
  fprintf(stderr, "libgfx: GL_VENDOR=%s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "libgfx: GL_RENDERER=%s\n", glGetString(GL_RENDERER));
  glViewport(0, 0, gfx_resolution_value.width, gfx_resolution_value.height);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0);
  return 0;
}

void ylc_gfx_stop(void) { gfx_runtime.running = false; }

static void ylc_gfx_loop(YlcHostTickFn tick, void *ctx) {
  if (ylc_gfx_init() != 0) {
    return;
  }

  __clear_gui_loop_stop();
  while (gfx_runtime.running && !__gui_loop_should_stop()) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        gfx_runtime.running = false;
      } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        if (gfx_runtime.window &&
            SDL_GetWindowID(gfx_runtime.window) == event.window.windowID) {
          gfx_runtime.running = false;
        }
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        SDL_GetWindowSize(gfx_runtime.window, &gfx_resolution_value.width,
                          &gfx_resolution_value.height);
        gfx_screen_target.width = gfx_resolution_value.width;
        gfx_screen_target.height = gfx_resolution_value.height;
        glViewport(0, 0, gfx_resolution_value.width,
                   gfx_resolution_value.height);
      }

      for (EventHandler *h = gfx_runtime.handlers; h; h = h->next) {
        h->fn(&event);
      }
    }

    if (gfx_runtime.perf_freq != 0) {
      Uint64 now = SDL_GetPerformanceCounter();
      gfx_runtime.dt = (double)(now - gfx_runtime.last_counter) /
                       (double)gfx_runtime.perf_freq;
      gfx_runtime.time += gfx_runtime.dt;
      gfx_runtime.last_counter = now;
    }

    if (gfx_runtime.frame_fn) {
      gfx_runtime.frame_fn();
    } else {
      glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      SDL_GL_SwapWindow(gfx_runtime.window);
    }

    if (tick) {
      tick(ctx);
    }

    SDL_Delay(8);
  }

  ylc_gfx_close();
}

int ylc_gfx_run(void) {
  ylc_gfx_loop(NULL, NULL);
  return 0;
}

__attribute__((constructor)) static void ylc_gfx_lib_init(void) {

  if (ylc_gfx_init() == 0) {
    ylc_host_loop_register(ylc_gfx_loop);
  }
  fprintf(stderr, "libgfx: ready\n");
}

// exposed gfx primitives

void gfx_begin_frame(void) {
  if (!gfx_runtime.window) {
    return;
  }
  SDL_GL_MakeCurrent(gfx_runtime.window, gfx_runtime.gl_context);
  glViewport(0, 0, gfx_resolution_value.width, gfx_resolution_value.height);
}

void gfx_end_frame(void) {
  if (!gfx_runtime.window) {
    return;
  }
  SDL_GL_SwapWindow(gfx_runtime.window);
}

void gfx_clear(double r, double g, double b, double a) {
  glClearColor((float)r, (float)g, (float)b, (float)a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void *gfx_screen(void) { return &gfx_screen_target; }

void *gfx_target(int width, int height) {
  GfxTarget *target;
  GLenum status;

  if (width <= 0 || height <= 0) {
    return NULL;
  }

  target = calloc(1, sizeof(GfxTarget));
  if (!target) {
    return NULL;
  }

  target->width = width;
  target->height = height;

  glGenFramebuffers(1, &target->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);

  glGenTextures(1, &target->color_tex);
  glBindTexture(GL_TEXTURE_2D, target->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         target->color_tex, 0);

  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "libgfx: framebuffer incomplete: 0x%x\n", (unsigned)status);
    glDeleteTextures(1, &target->color_tex);
    glDeleteFramebuffers(1, &target->fbo);
    free(target);
    return NULL;
  }

  return target;
}

void gfx_bind_target(void *target_ptr) {
  GfxTarget *target = target_ptr;

  if (!target || target->is_screen) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gfx_resolution_value.width, gfx_resolution_value.height);
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
  glViewport(0, 0, target->width, target->height);
}

void *gfx_framebuffer(int width, int height) {
  GfxFramebuffer *fb;

  if (width <= 0 || height <= 0) {
    return NULL;
  }
  if (ylc_gfx_init() != 0) {
    return NULL;
  }

  fb = calloc(1, sizeof(GfxFramebuffer));
  if (!fb) {
    return NULL;
  }

  fb->width = width;
  fb->height = height;

  glGenTextures(1, &fb->texture);
  glBindTexture(GL_TEXTURE_2D, fb->texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  gfx_log_gl_error("gfx_framebuffer");

  return fb;
}

void gfx_framebuffer_write(void *fb_ptr, _String pixels) {
  GfxFramebuffer *fb = fb_ptr;
  size_t byte_count;

  if (!fb || !pixels.chars) {
    return;
  }

  // pixels holds raw RGBA bytes: 4 per texture texel
  byte_count = (size_t)fb->width * (size_t)fb->height * GFX_RGBA_BYTES;
  if ((size_t)pixels.size < byte_count) {
    return;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, fb->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb->width, fb->height, GL_RGBA,
                  GL_UNSIGNED_BYTE, pixels.chars);
  glBindTexture(GL_TEXTURE_2D, 0);
  gfx_log_gl_error("gfx_framebuffer_write");
}

void gfx_framebuffer_draw(void *fb_ptr, int scale) {
  GfxFramebuffer *fb = fb_ptr;
  GfxMesh *quad;
  GLuint program;
  GLint loc;
  int draw_width;
  int draw_height;
  int draw_x;
  int draw_y;

  if (!fb) {
    return;
  }
  if (scale <= 0) {
    scale = 1;
  }

  program = gfx_framebuffer_shader();
  if (!program) {
    return;
  }

  quad = gfx_fullscreen_quad();
  draw_width = fb->width * scale;
  draw_height = fb->height * scale;
  draw_x = (gfx_resolution_value.width - draw_width) / 2;
  draw_y = (gfx_resolution_value.height - draw_height) / 2;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(draw_x, draw_y, draw_width, draw_height);
  glDisable(GL_DEPTH_TEST);
  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, fb->texture);
  loc = glGetUniformLocation(program, "uTex");
  if (loc >= 0) {
    glUniform1i(loc, 0);
  }

  gfx_draw_mesh(quad);
  glBindTexture(GL_TEXTURE_2D, 0);
  glViewport(0, 0, gfx_resolution_value.width, gfx_resolution_value.height);
  glEnable(GL_DEPTH_TEST);
  gfx_log_gl_error("gfx_framebuffer_draw");
}

void *gfx_fullscreen_quad(void) {
  static const float verts[12] = {
      -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f,
  };

  if (!gfx_runtime.quad_ready) {
    glGenVertexArrays(1, &gfx_runtime.fullscreen_quad.vao);
    glGenBuffers(1, &gfx_runtime.fullscreen_quad.vbo);
    glBindVertexArray(gfx_runtime.fullscreen_quad.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gfx_runtime.fullscreen_quad.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    gfx_runtime.fullscreen_quad.vertex_count = 6;
    gfx_runtime.fullscreen_quad.mode = GL_TRIANGLES;
    gfx_runtime.quad_ready = true;
  }

  return &gfx_runtime.fullscreen_quad;
}

static void *gfx_make_mesh(_DoubleArray vertices, int stride, GLenum mode) {
  GfxMesh *mesh;
  float *data;
  size_t i;

  if (stride != 6) {
    fprintf(stderr, "libgfx: mesh currently requires stride 6\n");
    return NULL;
  }
  if (vertices.size == 0 || (vertices.size % stride) != 0) {
    fprintf(stderr, "libgfx: mesh got invalid vertex data\n");
    return NULL;
  }

  mesh = malloc(sizeof(GfxMesh));
  if (!mesh) {
    return NULL;
  }

  data = malloc((size_t)vertices.size * sizeof(float));
  if (!data) {
    free(mesh);
    return NULL;
  }

  for (i = 0; i < (size_t)vertices.size; i++) {
    data[i] = (float)vertices.data[i];
  }

  {
    size_t debug_vertices = (size_t)(vertices.size / stride);
    if (debug_vertices > 6) {
      debug_vertices = 6;
    }
    fprintf(stderr, "libgfx: mesh first vertices (mode=%u)\n", (unsigned)mode);
    for (i = 0; i < debug_vertices; i++) {
      size_t off = i * (size_t)stride;
      fprintf(stderr,
              "libgfx: v%zu pos=(%.6f, %.6f, %.6f) col=(%.6f, %.6f, %.6f)\n", i,
              data[off], data[off + 1], data[off + 2], data[off + 3],
              data[off + 4], data[off + 5]);
    }
  }

  glGenVertexArrays(1, &mesh->vao);
  glGenBuffers(1, &mesh->vbo);
  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER,
               (GLsizeiptr)((size_t)vertices.size * sizeof(float)), data,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  mesh->vertex_count = (int)(vertices.size / stride);
  mesh->mode = mode;
  gfx_log_gl_error("gfx_make_mesh");
  free(data);
  return mesh;
}

void *gfx_mesh(_DoubleArray vertices, int stride) {
  return gfx_make_mesh(vertices, stride, GL_TRIANGLES);
}

void *gfx_points(_DoubleArray vertices, int stride) {
  return gfx_make_mesh(vertices, stride, GL_POINTS);
}

void *gfx_lines(_DoubleArray vertices, int stride) {
  return gfx_make_mesh(vertices, stride, GL_LINES);
}

void *gfx_shader(_String vert, _String frag) {
  GLuint vs;
  GLuint fs;
  GLuint program;
  GLint ok = 0;
  char log[1024];
  GfxShader *shader;

  vs = gfx_compile_shader(vert.chars, GL_VERTEX_SHADER);
  if (!vs) {
    return NULL;
  }
  fs = gfx_compile_shader(frag.chars, GL_FRAGMENT_SHADER);
  if (!fs) {
    glDeleteShader(vs);
    return NULL;
  }

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (!ok) {
    glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
    fprintf(stderr, "libgfx: program link failed: %s\n", log);
    glDeleteProgram(program);
    return NULL;
  }

  shader = malloc(sizeof(GfxShader));
  if (!shader) {
    glDeleteProgram(program);
    return NULL;
  }
  shader->program = program;
  gfx_log_gl_error("gfx_shader");
  return shader;
}

void *gfx_compute_shader(_String src) {
  GLuint cs;
  GLuint program;
  GLint ok = 0;
  char log[1024];
  GfxShader *shader;

  cs = gfx_compile_shader(src.chars, GL_COMPUTE_SHADER);
  if (!cs) {
    return NULL;
  }

  program = glCreateProgram();
  glAttachShader(program, cs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  glDeleteShader(cs);
  if (!ok) {
    glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
    fprintf(stderr, "libgfx: compute program link failed: %s\n", log);
    glDeleteProgram(program);
    return NULL;
  }

  shader = malloc(sizeof(GfxShader));
  if (!shader) {
    glDeleteProgram(program);
    return NULL;
  }
  shader->program = program;
  gfx_log_gl_error("gfx_compute_shader");
  return shader;
}

void gfx_use_shader(void *shader_ptr) {
  GfxShader *shader = shader_ptr;
  if (!shader) {
    return;
  }
  glUseProgram(shader->program);
  gfx_log_gl_error("gfx_use_shader");
}

void gfx_use_compute_shader(void *shader_ptr) { gfx_use_shader(shader_ptr); }

void gfx_uniform1f(void *shader_ptr, _String name, double x) {
  GfxShader *shader = shader_ptr;
  GLint loc;
  if (!shader) {
    return;
  }
  loc = glGetUniformLocation(shader->program, name.chars);
  if (loc >= 0) {
    glUniform1f(loc, (float)x);
  }
}

void gfx_uniform2f(void *shader_ptr, _String name, double x, double y) {
  GfxShader *shader = shader_ptr;
  GLint loc;
  if (!shader) {
    return;
  }
  loc = glGetUniformLocation(shader->program, name.chars);
  if (loc >= 0) {
    glUniform2f(loc, (float)x, (float)y);
  }
}

void gfx_uniform_mat4(void *shader_ptr, _String name, _DoubleArray matrix) {
  GfxShader *shader = shader_ptr;
  GLint loc;
  float values[16];
  size_t i;

  if (!shader) {
    return;
  }

  loc = glGetUniformLocation(shader->program, name.chars);
  if (loc < 0) {
    return;
  }

  for (i = 0; i < 16; i++) {
    values[i] = (float)matrix.data[i];
  }

  glUniformMatrix4fv(loc, 1, GL_FALSE, values);
  gfx_log_gl_error("gfx_uniform_mat4");
}

void gfx_uniform_tex(void *shader_ptr, _String name, void *target_ptr,
                     int unit) {
  GfxShader *shader = shader_ptr;
  GLuint tex;
  GLint loc;

  if (!shader) {
    return;
  }

  tex = gfx_texture_id(target_ptr);
  if (!tex) {
    return;
  }

  loc = glGetUniformLocation(shader->program, name.chars);
  if (loc < 0) {
    return;
  }

  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(loc, unit);
  gfx_log_gl_error("gfx_uniform_tex");
}

void gfx_bind_image_texture(void *target_ptr, int unit, int access,
                            int format) {
  GLuint tex = gfx_texture_id(target_ptr);

  if (!tex) {
    return;
  }

  glBindImageTexture((GLuint)unit, tex, 0, GL_FALSE, 0, (GLenum)access,
                     (GLenum)format);
  gfx_log_gl_error("gfx_bind_image_texture");
}

void gfx_dispatch_compute(int x, int y, int z) {
  glDispatchCompute((GLuint)x, (GLuint)y, (GLuint)z);
  gfx_log_gl_error("gfx_dispatch_compute");
}

void gfx_memory_barrier(void) {
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                  GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
  gfx_log_gl_error("gfx_memory_barrier");
}

void gfx_draw_mesh(void *mesh_ptr) {
  GfxMesh *mesh = mesh_ptr;
  if (!mesh) {
    return;
  }
  if (mesh->mode == GL_POINTS) {
    glEnable(GL_PROGRAM_POINT_SIZE);
  }
  glBindVertexArray(mesh->vao);
  glDrawArrays(mesh->mode, 0, mesh->vertex_count);
  glBindVertexArray(0);
  gfx_log_gl_error("gfx_draw_mesh");
}

double gfx_time(void) { return gfx_runtime.time; }

double gfx_dt(void) { return gfx_runtime.dt; }

YLCIntIntTuple gfx_resolution(void) {
  return (YLCIntIntTuple){.x = gfx_resolution_value.width,
                          .y = gfx_resolution_value.height};
}

void gfx_run(void *cb) { gfx_runtime.frame_fn = (GfxFrameFn)cb; }

void gfx_register_sdl_event_handler(_String name, void (*cb)(SDL_Event *)) {
  EventHandler *handler;

  if (!cb) {
    return;
  }

  handler = calloc(1, sizeof(EventHandler));
  if (!handler) {
    return;
  }

  handler->name = name.chars;
  handler->fn = cb;
  handler->next = gfx_runtime.handlers;
  gfx_runtime.handlers = handler;
}

int gfx_event_type(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  return (int)event->type;
}

int gfx_event_key(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP) {
    return 0;
  }
  return (int)event->key.key;
}

int gfx_event_mouse_x(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  switch (event->type) {
  case SDL_EVENT_MOUSE_MOTION:
    return (int)event->motion.x;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
    return (int)event->button.x;
  default:
    return 0;
  }
}

int gfx_event_mouse_y(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  switch (event->type) {
  case SDL_EVENT_MOUSE_MOTION:
    return (int)event->motion.y;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
    return (int)event->button.y;
  default:
    return 0;
  }
}

int gfx_event_button(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event->type != SDL_EVENT_MOUSE_BUTTON_UP) {
    return 0;
  }
  return (int)event->button.button;
}

int gfx_event_wheel_y(SDL_Event *event) {
  if (!event) {
    return 0;
  }
  if (event->type != SDL_EVENT_MOUSE_WHEEL) {
    return 0;
  }
  return (int)event->wheel.y;
}
