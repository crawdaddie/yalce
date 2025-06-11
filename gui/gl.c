#include "./common.h"
#include <GL/glew.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  const char *vertex_shader;
  const char *fragment_shader;
  GLuint shader_program;
  void *objs;
} CustomOpenGLState;

GLuint compile_shader(const char *source, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info[512];
    glGetShaderInfoLog(shader, 512, NULL, info);
    fprintf(stderr, "Shader compilation failed: %s %s\n", info, source);
    return 0;
  }
  return shader;
}

void gl_clear(double r, double g, double b, double alpha) {
  glClearColor(r, g, b, alpha);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gl_use_program(CustomOpenGLState *state) {
  glUseProgram(state->shader_program);
}

typedef bool (*GLObjInitFn)(CustomOpenGLState *state, void *obj);
typedef bool (*GLObjRenderFn)(CustomOpenGLState *state, void *obj);

typedef struct GLObj {
  void *data;
  GLObjInitFn init_gl;
  GLObjRenderFn render_gl;
  struct GLObj *next;
} GLObj;

static bool init_opengl_decl_win(void *_state) {

  CustomOpenGLState *state = _state;
  state->shader_program = glCreateProgram();

  GLObj *head = state->objs;
  while (head) {

    head->init_gl(state, head);
    head = head->next;
  }

  glLinkProgram(state->shader_program);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);

  GLint success;
  glGetProgramiv(state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    return false;
  }
  return true;
}

void open_gl_decl_renderer(void *_state) {

  CustomOpenGLState *state = _state;
  gl_clear(0.2, 0.3, 0.3, 1.0);

  gl_use_program(state);
  GLObj *head = state->objs;
  while (head) {
    if (head->render_gl) {
      head->render_gl(state, head);
    }
    head = head->next;
  }
}

typedef void (*DeclGlFn)();
typedef struct {
  int size;
  const char *chars;
} _String;

GLObj *_dcl_ctx_head = NULL;
GLObj *_dcl_ctx_tail = NULL;

GLObj *append_obj(GLObj obj) {
  if (_dcl_ctx_head == NULL) {
    GLObj *head = malloc(sizeof(GLObj));
    *head = obj;
    _dcl_ctx_head = head;
    _dcl_ctx_tail = head;
    return head;
  }

  GLObj *tail = malloc(sizeof(GLObj));
  *tail = obj;
  _dcl_ctx_tail->next = tail;
  _dcl_ctx_tail = tail;
  return tail;
}

bool init_vshader(CustomOpenGLState *state, GLObj *obj) {
  GLuint vs = compile_shader(obj->data, GL_VERTEX_SHADER);
  if (!vs) {
    glDeleteShader(vs);
    return false;
  }
  glAttachShader(state->shader_program, vs);
  return true;
}

void *VShader(_String str) {
  GLObj obj = (GLObj){.data = str.chars, .init_gl = (void *)init_vshader};
  return append_obj(obj);
}

bool init_fshader(CustomOpenGLState *state, GLObj *obj) {

  GLuint fs = compile_shader(obj->data, GL_FRAGMENT_SHADER);
  if (!fs) {
    glDeleteShader(fs);
    return false;
  }
  glAttachShader(state->shader_program, fs);
  return true;
}

void *FShader(_String str) {
  GLObj obj = (GLObj){.data = str.chars, .init_gl = (void *)init_fshader};
  return append_obj(obj);
}

int create_decl_window(void *_decl_cb) {
  DeclGlFn init = _decl_cb;

  _dcl_ctx_head = NULL;
  _dcl_ctx_tail = NULL;
  init();

  CustomOpenGLState *state = calloc(1, sizeof(CustomOpenGLState));
  state->objs = _dcl_ctx_head;

  window_creation_data *data = malloc(sizeof(window_creation_data));
  data->init_gl = init_opengl_decl_win;
  data->render_fn = open_gl_decl_renderer;

  data->data = state;

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_OPENGL_WINDOW_EVENT;
  event.user.data1 = data;

  return SDL_PushEvent(&event);
}

typedef struct {
  int num_vertices;

  // Each triangle gets its own GL resources
  GLuint vao;
  GLuint vbo;
  float *gl_vertices; // Converted float data
} TriData;

void render_tri_data(CustomOpenGLState *state, GLObj *obj) {

  TriData *d = obj->data;
  glBindVertexArray(d->vao);
  glDrawArrays(GL_TRIANGLES, 0, d->num_vertices);
}
bool init_tri_data(CustomOpenGLState *state, GLObj *obj) {

  TriData *d = obj->data;
  int num_vertices = d->num_vertices;
  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);

  glBindVertexArray(d->vao);

  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 6,
               d->gl_vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(num_vertices * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  return true;
}

void *TriangleData(_DoubleArray _d) {
  TriData *d = malloc(sizeof(TriData) + (sizeof(float) * _d.size));
  d->gl_vertices = (float *)(d + 1);

  for (int i = 0; i < _d.size; i++) {
    d->gl_vertices[i] = _d.data[i];
  }
  d->num_vertices = _d.size / 6;

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_tri_data,
                      .render_gl = (GLObjInitFn)render_tri_data};
  return append_obj(obj);
}
// ===== POINTS COMPONENT =====
typedef struct {
  int num_points;
  float point_size;
  GLuint vao;
  GLuint vbo;
  float *gl_vertices; // 6 components per point (3 pos + 3 color)
} PointData;

void render_point_data(CustomOpenGLState *state, GLObj *obj) {
  PointData *point_data = (PointData *)obj->data;

  glPointSize(point_data->point_size);

  glBindVertexArray(point_data->vao);
  glDrawArrays(GL_POINTS, 0, point_data->num_points);
  glBindVertexArray(0);
}

bool init_point_data(CustomOpenGLState *state, GLObj *obj) {
  PointData *d = (PointData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);

  glBindVertexArray(d->vao);
  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);

  int total_floats = d->num_points * 6;
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * total_floats, d->gl_vertices,
               GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

void *Points(_DoubleArray _d, double point_size) {
  int num_points = _d.size / 6;

  PointData *d = malloc(sizeof(PointData) + (sizeof(float) * _d.size));
  d->gl_vertices = (float *)(d + 1);
  d->num_points = num_points;
  d->point_size = (float)point_size;
  d->vao = 0;
  d->vbo = 0;

  for (int i = 0; i < _d.size; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_point_data,
                      .render_gl = (GLObjRenderFn)render_point_data,
                      .next = NULL};
  return append_obj(obj);
}

// ===== POLYGON COMPONENT (TRIANGLE_FAN) =====
typedef struct {
  int num_vertices;
  GLuint vao;
  GLuint vbo;
  float *gl_vertices;
} PolygonData;

void render_polygon_data(CustomOpenGLState *state, GLObj *obj) {
  PolygonData *poly_data = (PolygonData *)obj->data;

  glBindVertexArray(poly_data->vao);
  glDrawArrays(GL_TRIANGLE_FAN, 0, poly_data->num_vertices);
  glBindVertexArray(0);
}

bool init_polygon_data(CustomOpenGLState *state, GLObj *obj) {
  PolygonData *d = (PolygonData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);

  glBindVertexArray(d->vao);
  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);

  int total_floats = d->num_vertices * 6;
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * total_floats, d->gl_vertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

void *Polygon(_DoubleArray _d) {
  int num_vertices = _d.size / 6;

  PolygonData *d = malloc(sizeof(PolygonData) + (sizeof(float) * _d.size));
  d->gl_vertices = (float *)(d + 1);
  d->num_vertices = num_vertices;
  d->vao = 0;
  d->vbo = 0;

  for (int i = 0; i < _d.size; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_polygon_data,
                      .render_gl = (GLObjRenderFn)render_polygon_data,
                      .next = NULL};
  return append_obj(obj);
}

// ===== LINE STRIP COMPONENT =====
typedef struct {
  int num_vertices;
  float line_width;
  GLuint vao;
  GLuint vbo;
  float *gl_vertices;
} LineStripData;

void render_line_strip_data(CustomOpenGLState *state, GLObj *obj) {
  LineStripData *line_data = (LineStripData *)obj->data;

  glLineWidth(line_data->line_width);
  glBindVertexArray(line_data->vao);
  glDrawArrays(GL_LINE_STRIP, 0, line_data->num_vertices);
  glBindVertexArray(0);
}

bool init_line_strip_data(CustomOpenGLState *state, GLObj *obj) {
  LineStripData *d = (LineStripData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);

  glBindVertexArray(d->vao);
  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);

  int total_floats = d->num_vertices * 6;
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * total_floats, d->gl_vertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

void *Line(_DoubleArray _d, double line_width) {
  int num_vertices = _d.size / 6;

  LineStripData *d = malloc(sizeof(LineStripData) + (sizeof(float) * _d.size));
  d->gl_vertices = (float *)(d + 1);
  d->num_vertices = num_vertices;
  d->line_width = (float)line_width;
  d->vao = 0;
  d->vbo = 0;

  for (int i = 0; i < _d.size; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_line_strip_data,
                      .render_gl = (GLObjRenderFn)render_line_strip_data,
                      .next = NULL};
  return append_obj(obj);
}

// ===== INDIVIDUAL LINES COMPONENT =====
typedef struct {
  int num_lines;
  float line_width;
  GLuint vao;
  GLuint vbo;
  float *gl_vertices;
} LinesData;

void render_lines_data(CustomOpenGLState *state, GLObj *obj) {
  LinesData *line_data = (LinesData *)obj->data;

  glLineWidth(line_data->line_width);
  glBindVertexArray(line_data->vao);
  glDrawArrays(GL_LINES, 0, line_data->num_lines * 2);
  glBindVertexArray(0);
}

bool init_lines_data(CustomOpenGLState *state, GLObj *obj) {
  LinesData *d = (LinesData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);

  glBindVertexArray(d->vao);
  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);

  int total_floats =
      d->num_lines * 2 * 6; // 2 vertices per line, 6 components per vertex
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * total_floats, d->gl_vertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

void *Lattice(_DoubleArray _d, double line_width) {
  int num_lines =
      _d.size / 12; // 12 components per line (2 vertices * 6 components)

  LinesData *d = malloc(sizeof(LinesData) + (sizeof(float) * _d.size));
  d->gl_vertices = (float *)(d + 1);
  d->num_lines = num_lines;
  d->line_width = (float)line_width;
  d->vao = 0;
  d->vbo = 0;

  for (int i = 0; i < _d.size; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_lines_data,
                      .render_gl = (GLObjRenderFn)render_lines_data,
                      .next = NULL};
  return append_obj(obj);
}

// ===== QUAD COMPONENT (2 triangles) =====
typedef struct {
  GLuint vao;
  GLuint vbo;
  GLuint ebo; // Element buffer for indices
  float *gl_vertices;
  unsigned int *indices;
} QuadData;

void render_quad_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *quad_data = (QuadData *)obj->data;

  glBindVertexArray(quad_data->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

bool init_quad_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *d = (QuadData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);
  glGenBuffers(1, &d->ebo);

  glBindVertexArray(d->vao);

  // Upload vertex data
  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, d->gl_vertices,
               GL_STATIC_DRAW);

  // Upload index data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, d->indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

void *Quad(_DoubleArray _d) {
  // Quad needs 4 vertices (24 components) + 6 indices
  QuadData *d = malloc(sizeof(QuadData) + (sizeof(float) * 24) +
                       (sizeof(unsigned int) * 6));
  d->gl_vertices = (float *)(d + 1);
  d->indices = (unsigned int *)(d->gl_vertices + 24);
  d->vao = 0;
  d->vbo = 0;
  d->ebo = 0;

  for (int i = 0; i < 24; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  d->indices[0] = 0;
  d->indices[1] = 1;
  d->indices[2] = 2;
  d->indices[3] = 2;
  d->indices[4] = 3;
  d->indices[5] = 0;

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_quad_data,
                      .render_gl = (GLObjRenderFn)render_quad_data,
                      .next = NULL};
  return append_obj(obj);
}
