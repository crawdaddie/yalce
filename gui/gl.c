#include "./gl.h"
#include "./common.h"
#include <GL/glew.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

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
static bool init_opengl_decl_win(void *_state) {

  CustomOpenGLState *state = _state;
  state->shader_program = glCreateProgram();

  _dcl_ctx_head = NULL;
  _dcl_ctx_tail = NULL;
  state->init_cb();

  state->objs = _dcl_ctx_head;

  GLObj *head = state->objs;
  while (head) {
    if (head->init_gl) {
      head->init_gl(state, head);
    }
    head = head->next;
  }

  glLinkProgram(state->shader_program);

  glEnable(GL_DEPTH_TEST);
  int width, height;
  SDL_Window *window = SDL_GL_GetCurrentWindow();
  if (window) {
    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
  }

  GLint success;
  glGetProgramiv(state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    return false;
  }
  return true;
}

void open_gl_decl_renderer(void *_state) {

  CustomOpenGLState *state = _state;

  glUseProgram(state->shader_program);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GLObj *head = state->objs;
  while (head) {
    if (head->render_gl) {
      head->render_gl(state, head);
    }
    head = head->next;
  }
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

void opengl_win_event_handler(CustomOpenGLState *state, SDL_Event *event) {
  if (event->type == SDL_WINDOWEVENT &&
      event->window.event == SDL_WINDOWEVENT_RESIZED) {
    int width = event->window.data1;
    int height = event->window.data2;
    glViewport(0, 0, width, height);
  }
  GLObj *head = state->objs;
  while (head) {
    if (head->handle_events) {
      head->handle_events(state, head, event);
    }
    head = head->next;
  }
}

int create_decl_window(void *_decl_cb) {
  CustomOpenGLState *state = calloc(1, sizeof(CustomOpenGLState));

  window_creation_data *data = malloc(sizeof(window_creation_data));
  data->init_gl = init_opengl_decl_win;
  data->render_fn = open_gl_decl_renderer;
  data->handle_event = opengl_win_event_handler;
  state->init_cb = _decl_cb;

  data->data = state;

  SDL_Event event;
  SDL_zero(event);
  event.type = CREATE_OPENGL_WINDOW_EVENT;
  event.user.data1 = data;

  return SDL_PushEvent(&event);
}

// ===== Uniform Components =====
typedef struct {
  const char *name;
  float value[16]; // Support 4x4 matrix
  int components;  // 1=float, 2=vec2, 3=vec3, 4=vec4, 16=mat4
  GLint loc;
} UniformData;

bool init_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  UniformData *data = obj->data;
  data->loc = glGetUniformLocation(state->shader_program, data->name);
  return true;
}

bool render_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  UniformData *uniform = (UniformData *)obj->data;

  GLint location = uniform->loc;
  if (location == -1) {
    uniform->loc = glGetUniformLocation(state->shader_program, uniform->name);
    location = uniform->loc;
  }

  switch (uniform->components) {
  case 1:
    glUniform1f(location, uniform->value[0]);
    break;
  case 2:
    glUniform2f(location, uniform->value[0], uniform->value[1]);
    break;
  case 3:
    glUniform3f(location, uniform->value[0], uniform->value[1],
                uniform->value[2]);
    break;
  case 4:
    glUniform4f(location, uniform->value[0], uniform->value[1],
                uniform->value[2], uniform->value[3]);
    break;
  case 16:
    glUniformMatrix4fv(location, 1, GL_FALSE, uniform->value);
    break;
  }

  return true;
}

// Uniform Factory functions
void *Uniform1f(_String name, double value) {
  UniformData *data = malloc(sizeof(UniformData));
  data->name = name.chars;
  data->value[0] = (float)value;
  data->components = 1;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_uniform_data,
                      .render_gl = (GLObjRenderFn)render_uniform_data,
                      .next = NULL};
  return append_obj(obj);
}

void *Uniform3f(_String name, double x, double y, double z) {
  UniformData *data = malloc(sizeof(UniformData));
  data->name = name.chars;
  data->value[0] = (float)x;
  data->value[1] = (float)y;
  data->value[2] = (float)z;
  data->components = 3;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_uniform_data,
                      .render_gl = (GLObjRenderFn)render_uniform_data,
                      .next = NULL};

  return append_obj(obj);
}

void *UniformMat4(_String name, _DoubleArray matrix_values) {
  UniformData *data = malloc(sizeof(UniformData));
  data->name = name.chars;
  data->components = 16;

  for (int i = 0; i < 16; i++) {
    data->value[i] = (float)matrix_values.data[i];
  }

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_uniform_data,
                      .render_gl = (GLObjRenderFn)render_uniform_data,
                      .next = NULL};
  return append_obj(obj);
}

void *UniformTime(_String name) {}

void set_uniform(GLObj *obj, double *data) {
  if (obj != NULL) {
    UniformData *udata = obj->data;

    for (int i = 0; i < udata->components; i++) {
      udata->value[i] = data[i];
    }
  }
}

typedef struct {
  const char *name;
  float value[16]; // Support 4x4 matrix
  int components;  // 1=float, 2=vec2, 3=vec3, 4=vec4, 16=mat4
  GLint loc;
  GLObj *source_uniform;   // The uniform we're following
  float lerp_factor;       // How fast we catch up (0.0 = never, 1.0 = instant)
  Uint32 last_update_time; // For frame-rate independent smoothing
} LaggedUniformData;

bool init_lagged_uniform3f(CustomOpenGLState *state, GLObj *obj) {
  LaggedUniformData *data = (LaggedUniformData *)obj->data;

  data->loc = glGetUniformLocation(state->shader_program, data->name);

  UniformData *source_data = (UniformData *)data->source_uniform->data;
  data->value[0] = source_data->value[0];
  data->value[1] = source_data->value[1];
  data->value[2] = source_data->value[2];

  data->last_update_time = SDL_GetTicks();

  return true;
}

bool render_lagged_uniform3f(CustomOpenGLState *state, GLObj *obj) {
  LaggedUniformData *data = (LaggedUniformData *)obj->data;

  Uint32 current_time = SDL_GetTicks();
  float delta_time = (current_time - data->last_update_time) / 1000.0f;
  data->last_update_time = current_time;

  UniformData *source_data = (UniformData *)data->source_uniform->data;

  float frame_lerp_factor =
      1.0f - powf(1.0f - data->lerp_factor,
                  delta_time * 60.0f); // Assuming 60fps baseline

  data->value[0] +=
      (source_data->value[0] - data->value[0]) * frame_lerp_factor;
  data->value[1] +=
      (source_data->value[1] - data->value[1]) * frame_lerp_factor;
  data->value[2] +=
      (source_data->value[2] - data->value[2]) * frame_lerp_factor;

  GLint location = data->loc;
  if (location == -1) {
    data->loc = glGetUniformLocation(state->shader_program, data->name);
    location = data->loc;
  }

  glUniform3f(location, data->value[0], data->value[1], data->value[2]);

  return true;
}

void *LagUniform(_String output_name, double lag_factor,
                 GLObj *source_uniform) {
  LaggedUniformData *data = malloc(sizeof(LaggedUniformData));

  data->name = output_name.chars;
  data->components = 3;
  data->loc = -1;

  data->source_uniform = (GLObj *)source_uniform;
  data->lerp_factor = (float)lag_factor; // 0.1 = slow lag, 0.9 = fast catch-up
  data->last_update_time = SDL_GetTicks();

  UniformData *source_data = (UniformData *)data->source_uniform->data;
  data->value[0] = source_data->value[0];
  data->value[1] = source_data->value[1];
  data->value[2] = source_data->value[2];

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_lagged_uniform3f,
                      .render_gl = (GLObjRenderFn)render_lagged_uniform3f,
                      .next = NULL};
  return append_obj(obj);
}

//=================================================

typedef struct {
  int num_vertices;
  GLuint vao;
  GLuint vbo;
  float *gl_vertices;
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

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

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
  unsigned int num_quads;
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

  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, d->gl_vertices,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, d->indices,
               GL_STATIC_DRAW);

  // Only position attribute (location 0) - 3 components per vertex
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  return true;
}

void *Quad(_DoubleArray _d) {
  // Quad needs 4 vertices (12 components: 4 vertices * 3 floats each) + 6
  // indices
  QuadData *d = malloc(sizeof(QuadData) + (sizeof(float) * 12) +
                       (sizeof(unsigned int) * 6));
  d->gl_vertices = (float *)(d + 1);
  d->indices = (unsigned int *)(d->gl_vertices + 12);
  d->vao = 0;
  d->vbo = 0;
  d->ebo = 0;

  for (int i = 0; i < 12; i++) {
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

void render_quads_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *quads_data = (QuadData *)obj->data;

  glBindVertexArray(quads_data->vao);
  glDrawElements(GL_TRIANGLES, quads_data->num_quads * 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

bool init_quads_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *d = (QuadData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);
  glGenBuffers(1, &d->ebo);

  glBindVertexArray(d->vao);

  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * d->num_quads * 4 * 3,
               d->gl_vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * d->num_quads * 6,
               d->indices, GL_STATIC_DRAW);

  // Only position attribute (location 0) - 3 components per vertex
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  return true;
}

void *Quads(_DoubleArray _d) {
  // Calculate number of quads: each quad needs 4 vertices * 3 components = 12
  // floats
  int num_quads = _d.size / 12;

  if (_d.size % 12 != 0) {
    fprintf(stderr,
            "Warning: Quads data size (%d) is not divisible by 12. Some "
            "vertices may be ignored.\n",
            _d.size);
  }

  // Allocate space for QuadsData + vertices + indices
  int vertex_floats =
      num_quads * 4 * 3;           // 4 vertices per quad, 3 floats per vertex
  int index_count = num_quads * 6; // 6 indices per quad (2 triangles)

  QuadData *d = malloc(sizeof(QuadData) + (sizeof(float) * vertex_floats) +
                       (sizeof(unsigned int) * index_count));

  d->num_quads = num_quads;
  d->gl_vertices = (float *)(d + 1);
  d->indices = (unsigned int *)(d->gl_vertices + vertex_floats);
  d->vao = 0;
  d->vbo = 0;
  d->ebo = 0;

  // Copy vertex data
  for (int i = 0; i < vertex_floats && i < _d.size; i++) {
    d->gl_vertices[i] = (float)_d.data[i];
  }

  // Generate indices for all quads
  for (int quad = 0; quad < num_quads; quad++) {
    int base_vertex = quad * 4; // Each quad has 4 vertices
    int base_index = quad * 6;  // Each quad has 6 indices

    // First triangle: vertices 0, 1, 2
    d->indices[base_index + 0] = base_vertex + 0;
    d->indices[base_index + 1] = base_vertex + 1;
    d->indices[base_index + 2] = base_vertex + 2;

    // Second triangle: vertices 2, 3, 0
    d->indices[base_index + 3] = base_vertex + 2;
    d->indices[base_index + 4] = base_vertex + 3;
    d->indices[base_index + 5] = base_vertex + 0;
  }

  GLObj obj = (GLObj){.data = d,
                      .init_gl = (GLObjInitFn)init_quads_data,
                      .render_gl = (GLObjRenderFn)render_quads_data,
                      .next = NULL};
  return append_obj(obj);
}

void render_quad_color_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *quad_data = (QuadData *)obj->data;

  glBindVertexArray(quad_data->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

bool init_quad_color_data(CustomOpenGLState *state, GLObj *obj) {
  QuadData *d = (QuadData *)obj->data;

  glGenVertexArrays(1, &d->vao);
  glGenBuffers(1, &d->vbo);
  glGenBuffers(1, &d->ebo);

  glBindVertexArray(d->vao);

  glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, d->gl_vertices,
               GL_STATIC_DRAW);

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

void *QuadColor(_DoubleArray _d) {
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

// Matrix math utilities
typedef struct {
  float m[16]; // Column-major order
} Mat4;

typedef struct {
  float x, y, z;
} Vec3;

void mat4_identity(Mat4 *m) {
  memset(m->m, 0, sizeof(float) * 16);
  m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

void mat4_perspective(Mat4 *m, float fov, float aspect, float near, float far) {
  mat4_identity(m);
  float f = 1.0f / tanf(fov * 0.5f);
  m->m[0] = f / aspect;
  m->m[5] = f;
  m->m[10] = (far + near) / (near - far);
  m->m[11] = -1.0f;
  m->m[14] = (2.0f * far * near) / (near - far);
  m->m[15] = 0.0f;
}

void mat4_lookat(Mat4 *m, Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 f = {center.x - eye.x, center.y - eye.y, center.z - eye.z};
  float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
  f.x /= len;
  f.y /= len;
  f.z /= len;

  Vec3 s = {f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z,
            f.x * up.y - f.y * up.x};
  len = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
  s.x /= len;
  s.y /= len;
  s.z /= len;

  Vec3 u = {s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z,
            s.x * f.y - s.y * f.x};

  mat4_identity(m);
  m->m[0] = s.x;
  m->m[4] = s.y;
  m->m[8] = s.z;
  m->m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
  m->m[1] = u.x;
  m->m[5] = u.y;
  m->m[9] = u.z;
  m->m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
  m->m[2] = -f.x;
  m->m[6] = -f.y;
  m->m[10] = -f.z;
  m->m[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
}

void mat4_multiply(Mat4 *result, const Mat4 *a, const Mat4 *b) {
  Mat4 temp;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp.m[i * 4 + j] = 0.0f;
      for (int k = 0; k < 4; k++) {
        temp.m[i * 4 + j] += a->m[i * 4 + k] * b->m[k * 4 + j];
      }
    }
  }
  *result = temp;
}

// ===== MVP View COMPONENT =====
typedef struct {
  GLint model_loc;
  GLint view_loc;
  GLint projection_loc;
  GLint camera_pos_loc;
  Vec3 pos;
  Vec3 target;
} MVPData;

void render_mvp_view(CustomOpenGLState *state, GLObj *obj) {

  MVPData *data = (MVPData *)obj->data;

  Mat4 model, view, projection;

  mat4_identity(&model);

  Vec3 camera_pos = data->pos;
  Vec3 target = data->target;
  Vec3 up = {0.0f, 1.0f, 0.0f};
  mat4_lookat(&view, camera_pos, target, up);

  int width, height;
  SDL_Window *window = SDL_GL_GetCurrentWindow();
  if (window) {
    SDL_GetWindowSize(window, &width, &height);
  } else {
    // Fallback to default if we can't get the window
    width = 640;
    height = 480;
  }

  float aspect = (float)width / (float)height;
  mat4_perspective(&projection, 45.0f * M_PI / 180.0f, aspect, 0.01f,
                   100000000.0f);

  if (data->model_loc == -1) {
    data->model_loc = glGetUniformLocation(state->shader_program, "uModel");
  }

  if (data->view_loc == -1) {
    data->view_loc = glGetUniformLocation(state->shader_program, "uView");
  }

  if (data->projection_loc == -1) {
    data->projection_loc =
        glGetUniformLocation(state->shader_program, "uProjection");
  }
  if (data->camera_pos_loc == -1) {
    data->camera_pos_loc =
        glGetUniformLocation(state->shader_program, "uCameraPos");
  }

  glUniformMatrix4fv(data->model_loc, 1, GL_FALSE, model.m);
  glUniformMatrix4fv(data->view_loc, 1, GL_FALSE, view.m);
  glUniformMatrix4fv(data->projection_loc, 1, GL_FALSE, projection.m);
  glUniform3f(data->camera_pos_loc, camera_pos.x, camera_pos.y, camera_pos.z);
}

bool init_mvp_view(CustomOpenGLState *state, GLObj *obj) {
  MVPData *data = (MVPData *)obj->data;

  data->model_loc = glGetUniformLocation(state->shader_program, "uModel");
  data->view_loc = glGetUniformLocation(state->shader_program, "uView");
  data->projection_loc =
      glGetUniformLocation(state->shader_program, "uProjection");
  data->camera_pos_loc =
      glGetUniformLocation(state->shader_program, "uCameraPos");

  return true;
}

void mvp_view_event_handler(CustomOpenGLState *state, GLObj *obj,
                            SDL_Event *event) {

  MVPData *data = obj->data;

  if (event->type == SDL_KEYDOWN) {
    float rotate_speed = 0.2f; // Adjust rotation speed
    float move_speed = 100.f;  // Adjust movement speed

    switch (event->key.keysym.sym) {
    case SDLK_UP: {
      float dx = data->target.x - data->pos.x;
      float dy = data->target.y - data->pos.y;
      float dz = data->target.z - data->pos.z;

      // Normalize the direction vector
      float len = sqrtf(dx * dx + dy * dy + dz * dz);
      if (len > move_speed) { // Don't go past the target
        dx /= len;
        dy /= len;
        dz /= len;

        // Move along the vector toward target
        data->pos.x += dx * move_speed;
        data->pos.y += dy * move_speed;
        data->pos.z += dz * move_speed;
      }

      break;
    }

    case SDLK_DOWN: {
      float dx = data->target.x - data->pos.x;
      float dy = data->target.y - data->pos.y;
      float dz = data->target.z - data->pos.z;

      float len = sqrtf(dx * dx + dy * dy + dz * dz);
      if (len > 0.001f) { // Avoid division by zero
        dx /= len;
        dy /= len;
        dz /= len;

        data->pos.x -= dx * move_speed;
        data->pos.y -= dy * move_speed;
        data->pos.z -= dz * move_speed;
      }
      break;
    }

    case SDLK_LEFT: {
      float dx = data->pos.x - data->target.x;
      float dz = data->pos.z - data->target.z;

      float dist = sqrtf(dx * dx + dz * dz);
      float angle = atan2f(dz, dx);

      angle += rotate_speed;

      data->pos.x = data->target.x + dist * cosf(angle);
      data->pos.z = data->target.z + dist * sinf(angle);
      break;
    }

    case SDLK_RIGHT: {
      float dx = data->pos.x - data->target.x;
      float dz = data->pos.z - data->target.z;

      float dist = sqrtf(dx * dx + dz * dz);
      float angle = atan2f(dz, dx);

      angle -= rotate_speed;

      data->pos.x = data->target.x + dist * cosf(angle);
      data->pos.z = data->target.z + dist * sinf(angle);
    } break;
    }
  }
}

void *MVPView(double px, double py, double pz, double tx, double ty,
              double tz) {
  MVPData *data = malloc(sizeof(MVPData));
  data->pos = (Vec3){px, py, pz};
  data->target = (Vec3){tx, ty, tz};

  GLObj obj =
      (GLObj){.data = data,
              .init_gl = (GLObjInitFn)init_mvp_view,
              .render_gl = (GLObjRenderFn)render_mvp_view,
              .handle_events = (GLObjEventHandlerFn)mvp_view_event_handler,
              .next = NULL};
  return append_obj(obj);
}

typedef struct {
  GLint model_loc;
  GLint view_loc;
  GLint projection_loc;
  GLObj *pos_uniform;    // Points to Uniform3f object for position
  GLObj *target_uniform; // Points to Uniform3f object for target
} CamData;

void render_cam_view(CustomOpenGLState *state, GLObj *obj) {
  CamData *data = (CamData *)obj->data;

  UniformData *pos_data = (UniformData *)data->pos_uniform->data;
  Vec3 camera_pos = {pos_data->value[0], pos_data->value[1],
                     pos_data->value[2]};

  UniformData *target_data = (UniformData *)data->target_uniform->data;
  Vec3 target = {target_data->value[0], target_data->value[1],
                 target_data->value[2]};

  Mat4 model, view, projection;

  mat4_identity(&model);

  Vec3 up = {0.0f, 1.0f, 0.0f};
  mat4_lookat(&view, camera_pos, target, up);

  int width, height;
  SDL_Window *window = SDL_GL_GetCurrentWindow();
  if (window) {
    SDL_GetWindowSize(window, &width, &height);
  } else {
    width = 640;
    height = 480;
  }

  float aspect = (float)width / (float)height;
  mat4_perspective(&projection, 45.0f * M_PI / 180.0f, aspect, 0.01f,
                   100000000.0f);

  if (data->model_loc == -1) {
    data->model_loc = glGetUniformLocation(state->shader_program, "uModel");
  }
  if (data->view_loc == -1) {
    data->view_loc = glGetUniformLocation(state->shader_program, "uView");
  }
  if (data->projection_loc == -1) {
    data->projection_loc =
        glGetUniformLocation(state->shader_program, "uProjection");
  }

  // Send matrices to shader
  glUniformMatrix4fv(data->model_loc, 1, GL_FALSE, model.m);
  glUniformMatrix4fv(data->view_loc, 1, GL_FALSE, view.m);
  glUniformMatrix4fv(data->projection_loc, 1, GL_FALSE, projection.m);
}

bool init_cam_view(CustomOpenGLState *state, GLObj *obj) {
  CamData *data = (CamData *)obj->data;

  data->model_loc = -1;
  data->view_loc = -1;
  data->projection_loc = -1;

  return true;
}

void *CamView(void *pos_uniform, void *target_uniform) {
  CamData *data = malloc(sizeof(CamData));
  data->pos_uniform = (GLObj *)pos_uniform;
  data->target_uniform = (GLObj *)target_uniform;

  data->model_loc = -1;
  data->view_loc = -1;
  data->projection_loc = -1;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_cam_view,
                      .render_gl = (GLObjRenderFn)render_cam_view,
                      .next = NULL};
  return append_obj(obj);
}

GLObj *gl_obj_bind_handler(GLObjEventHandlerFn handler, GLObj *obj) {
  obj->handle_events = handler;
  return obj;
}

typedef struct {
  double r, g, b;
} BackgroundData;

void render_bg(CustomOpenGLState *state, GLObj *obj) {
  BackgroundData *data = (BackgroundData *)obj->data;
  glClearColor(data->r, data->g, data->b, 1.0f);
}

GLObj *Clear(double r, double g, double b) {

  BackgroundData *data = malloc(sizeof(BackgroundData));
  data->r = r;
  data->g = g;
  data->b = b;

  GLObj obj = (GLObj){
      .data = data, .render_gl = (GLObjRenderFn)render_bg, .next = NULL};
  return append_obj(obj);
}
// ===== FRAMEBUFFER COMPONENT =====
typedef struct {
  const char *tex_name;
  DeclGlFn init_fb; // Function pointer type should match
  CustomOpenGLState *state;

  GLuint framebuffer;
  GLuint color_texture;
  GLuint depth_renderbuffer;
  int width, height;
} FrameBufferData;

void render_fb(CustomOpenGLState *state, GLObj *obj) {
  FrameBufferData *fb = (FrameBufferData *)obj->data;

  // Save current render state
  GLint prev_framebuffer;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_framebuffer);
  GLint prev_viewport[4];
  glGetIntegerv(GL_VIEWPORT, prev_viewport);
  GLint prev_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);

  // Switch to framebuffer rendering
  glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer);
  glViewport(0, 0, fb->width, fb->height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use internal shader program
  glUseProgram(fb->state->shader_program);

  // Render internal scene
  GLObj *head = fb->state->objs;
  while (head) {
    if (head->render_gl) {
      head->render_gl(fb->state, head);
    }
    head = head->next;
  }

  // Restore main render state
  glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer);
  glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2],
             prev_viewport[3]);
  glUseProgram(prev_program);

  // Bind the framebuffer texture to the specified texture unit
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, fb->color_texture);

  // Set the texture uniform in the main shader
  GLint texture_loc = glGetUniformLocation(state->shader_program, fb->tex_name);
  if (texture_loc != -1) {
    glUniform1i(texture_loc, 1); // Use texture unit 1
  }

  // Reset to texture unit 0
  glActiveTexture(GL_TEXTURE0);
}

bool init_fb(CustomOpenGLState *outer_state, GLObj *obj) {
  // Save global context
  GLObj *tmp_dcl_ctx_head = _dcl_ctx_head;
  GLObj *tmp_dcl_ctx_tail = _dcl_ctx_tail;
  _dcl_ctx_head = NULL;
  _dcl_ctx_tail = NULL;

  FrameBufferData *fb = (FrameBufferData *)obj->data;

  fb->width = 512;
  fb->height = 512;

  glGenFramebuffers(1, &fb->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer);

  // Create color texture
  glGenTextures(1, &fb->color_texture);
  glBindTexture(GL_TEXTURE_2D, fb->color_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb->width, fb->height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         fb->color_texture, 0);

  glGenRenderbuffers(1, &fb->depth_renderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, fb->depth_renderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fb->width,
                        fb->height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, fb->depth_renderbuffer);

  // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Framebuffer not complete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return false;
  }

  // Initialize internal state
  CustomOpenGLState *inner_state = fb->state;
  inner_state->shader_program = glCreateProgram();

  // Call the framebuffer's declarative function to build scene
  inner_state->init_cb();
  inner_state->objs = _dcl_ctx_head;

  // Initialize all objects in the framebuffer scene
  GLObj *head = inner_state->objs;
  while (head) {
    if (head->init_gl) {
      head->init_gl(inner_state, head);
    }
    head = head->next;
  }

  // Link the framebuffer shader program
  glLinkProgram(inner_state->shader_program);

  GLint success;
  glGetProgramiv(inner_state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    char info[512];
    glGetProgramInfoLog(inner_state->shader_program, 512, NULL, info);
    printf("Framebuffer shader program linking failed: %s\n", info);
    return false;
  }

  // Restore default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Restore global context
  _dcl_ctx_head = tmp_dcl_ctx_head;
  _dcl_ctx_tail = tmp_dcl_ctx_tail;

  return true;
}

GLObj *FrameBuffer(_String texture_name, void *cb) {
  // Allocate space for FrameBufferData + CustomOpenGLState
  FrameBufferData *data =
      malloc(sizeof(FrameBufferData) + sizeof(CustomOpenGLState));
  data->tex_name = texture_name.chars;
  data->state = (CustomOpenGLState *)(data + 1);
  data->init_fb = (DeclGlFn)cb;
  data->state->init_cb = (DeclGlFn)cb;

  // Initialize OpenGL objects to 0
  data->framebuffer = 0;
  data->color_texture = 0;
  data->depth_renderbuffer = 0;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_fb,
                      .render_gl = (GLObjRenderFn)render_fb,
                      .next = NULL};
  return append_obj(obj);
}

// apply_texture can remain simple since framebuffer handles binding
GLObj *apply_texture(GLObj *tex, GLObj *shape) { return shape; }
// Add these includes at the top of your file

// ===== TEXTURE COMPONENT =====
typedef struct {
  const char *filename;
  const char *uniform_name;
  GLuint texture_id;
  GLint uniform_location;
  int texture_unit; // Which texture unit to bind to (0, 1, 2, etc.)
} TextureData;

bool init_texture(CustomOpenGLState *state, GLObj *obj) {
  TextureData *tex = (TextureData *)obj->data;

  // Load image using SDL_image
  SDL_Surface *surface = IMG_Load(tex->filename);
  if (!surface) {
    fprintf(stderr, "Failed to load image %s: %s\n", tex->filename,
            IMG_GetError());
    return false;
  }

  // Convert to RGBA format if needed
  SDL_Surface *rgba_surface = NULL;
  if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
    rgba_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    surface = rgba_surface;
  }

  if (!surface) {
    fprintf(stderr, "Failed to convert image to RGBA format\n");
    return false;
  }

  // Generate OpenGL texture
  glGenTextures(1, &tex->texture_id);
  glActiveTexture(GL_TEXTURE0 + tex->texture_unit);
  glBindTexture(GL_TEXTURE_2D, tex->texture_id);

  // Upload image data to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, surface->pixels);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Generate mipmaps for better quality at distance
  glGenerateMipmap(GL_TEXTURE_2D);

  // Get uniform location
  tex->uniform_location =
      glGetUniformLocation(state->shader_program, tex->uniform_name);

  // Clean up
  SDL_FreeSurface(surface);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);

  printf("Loaded texture %s (ID: %d) to unit %d\n", tex->filename,
         tex->texture_id, tex->texture_unit);
  return true;
}

void render_texture(CustomOpenGLState *state, GLObj *obj) {
  TextureData *tex = (TextureData *)obj->data;

  // Bind texture to its assigned unit
  glActiveTexture(GL_TEXTURE0 + tex->texture_unit);
  glBindTexture(GL_TEXTURE_2D, tex->texture_id);

  // Set the uniform to point to this texture unit
  if (tex->uniform_location != -1) {
    glUniform1i(tex->uniform_location, tex->texture_unit);
  } else {
    // Try to get the location again (in case shader wasn't ready during init)
    tex->uniform_location =
        glGetUniformLocation(state->shader_program, tex->uniform_name);
    if (tex->uniform_location != -1) {
      glUniform1i(tex->uniform_location, tex->texture_unit);
    }
  }
}

// Factory function to create a texture
void *ImgTexture(_String filename, _String uniform_name, int texture_unit) {
  TextureData *data = malloc(sizeof(TextureData));
  data->filename = malloc(filename.size);
  memcpy(data->filename, filename.chars, filename.size);
  data->uniform_name = uniform_name.chars;

  data->uniform_name = malloc(uniform_name.size);

  memcpy(data->uniform_name, uniform_name.chars, filename.size);
  data->texture_unit = texture_unit;

  data->texture_id = 0;
  data->uniform_location = -1;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_texture,
                      .render_gl = (GLObjRenderFn)render_texture,
                      .next = NULL};
  return append_obj(obj);
}
