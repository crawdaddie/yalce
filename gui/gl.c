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

#include <GLFW/glfw3.h>

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

typedef bool (*GLObjInitFn)(CustomOpenGLState *state, void *obj);
typedef bool (*GLObjRenderFn)(CustomOpenGLState *state, void *obj);

typedef bool (*GLObjEventHandlerFn)(CustomOpenGLState *state, void *obj,
                                    SDL_Event *event);
typedef struct GLObj {
  void *data;
  GLObjInitFn init_gl;
  GLObjRenderFn render_gl;
  GLObjEventHandlerFn handle_events;
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

  glEnable(GL_DEPTH_TEST);
  // Set initial viewport
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

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark gray instead of black
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(state->shader_program);
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

void opengl_win_event_handler(CustomOpenGLState *state, SDL_Event *event) {
  // Handle window resize
  if (event->type == SDL_WINDOWEVENT &&
      event->window.event == SDL_WINDOWEVENT_RESIZED) {
    int width = event->window.data1;
    int height = event->window.data2;
    glViewport(0, 0, width, height);
    printf("Window resized to %dx%d, viewport updated\n", width, height);
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
  DeclGlFn init = _decl_cb;

  _dcl_ctx_head = NULL;
  _dcl_ctx_tail = NULL;
  init();

  CustomOpenGLState *state = calloc(1, sizeof(CustomOpenGLState));
  state->objs = _dcl_ctx_head;

  window_creation_data *data = malloc(sizeof(window_creation_data));
  data->init_gl = init_opengl_decl_win;
  data->render_fn = open_gl_decl_renderer;
  data->handle_event = opengl_win_event_handler;

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

  // Enable point sprites (makes gl_PointCoord available in fragment shader)
  // glEnable(GL_PROGRAM_POINT_SIZE); // Allow shader to control point size

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
  mat4_perspective(&projection, 45.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f);

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

  // // Send matrices to shader
  glUniformMatrix4fv(data->model_loc, 1, GL_FALSE, model.m);
  glUniformMatrix4fv(data->view_loc, 1, GL_FALSE, view.m);
  glUniformMatrix4fv(data->projection_loc, 1, GL_FALSE, projection.m);
}

bool init_mvp_view(CustomOpenGLState *state, GLObj *obj) {
  MVPData *data = (MVPData *)obj->data;

  data->model_loc = glGetUniformLocation(state->shader_program, "uModel");
  data->view_loc = glGetUniformLocation(state->shader_program, "uView");
  data->projection_loc =
      glGetUniformLocation(state->shader_program, "uProjection");

  return true;
}

void mvp_view_event_handler(CustomOpenGLState *state, GLObj *obj,
                            SDL_Event *event) {

  MVPData *data = obj->data;

  if (event->type == SDL_KEYDOWN) {
    float rotate_speed = 0.2f; // Adjust rotation speed
    float move_speed = 0.5f;   // Adjust movement speed

    switch (event->key.keysym.sym) {
    case SDLK_UP:
      // Move camera toward target (closer)
      {
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
      }
      break;

    case SDLK_DOWN:
      // Move camera away from target (farther)
      {
        float dx = data->target.x - data->pos.x;
        float dy = data->target.y - data->pos.y;
        float dz = data->target.z - data->pos.z;

        // Normalize the direction vector
        float len = sqrtf(dx * dx + dy * dy + dz * dz);
        if (len > 0.001f) { // Avoid division by zero
          dx /= len;
          dy /= len;
          dz /= len;

          // Move along the vector away from target
          data->pos.x -= dx * move_speed;
          data->pos.y -= dy * move_speed;
          data->pos.z -= dz * move_speed;
        }
      }
      break;

    case SDLK_LEFT:
      // Rotate camera left around target (counter-clockwise in XZ plane)
      {
        float dx = data->pos.x - data->target.x;
        float dz = data->pos.z - data->target.z;

        float dist = sqrtf(dx * dx + dz * dz);
        float angle = atan2f(dz, dx);

        angle += rotate_speed;

        data->pos.x = data->target.x + dist * cosf(angle);
        data->pos.z = data->target.z + dist * sinf(angle);
      }
      break;

    case SDLK_RIGHT:
      // Rotate camera right around target (clockwise in XZ plane)
      {
        float dx = data->pos.x - data->target.x;
        float dz = data->pos.z - data->target.z;

        float dist = sqrtf(dx * dx + dz * dz);
        float angle = atan2f(dz, dx);

        angle -= rotate_speed;

        data->pos.x = data->target.x + dist * cosf(angle);
        data->pos.z = data->target.z + dist * sinf(angle);
      }
      break;
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

// ===== Uniform Components =====
typedef struct {
  const char *name;
  float value[16]; // Support 4x4 matrix
  int components;  // 1=float, 2=vec2, 3=vec3, 4=vec4, 16=mat4
} UniformData;

bool init_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  // Uniforms don't need GL initialization
  return true;
}

bool render_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  UniformData *uniform = (UniformData *)obj->data;

  GLint location = glGetUniformLocation(state->shader_program, uniform->name);
  if (location == -1) {
    printf("Warning: uniform '%s' not found\n", uniform->name);
    return true;
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
  case 16: // Matrix 4x4
    glUniformMatrix4fv(location, 1, GL_FALSE, uniform->value);
    printf("Set matrix uniform '%s'\n", uniform->name);
    break;
  }

  return true;
}

// Factory functions
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

  // Convert double array to float matrix
  for (int i = 0; i < 16; i++) {
    data->value[i] = (float)matrix_values.data[i];
  }

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_uniform_data,
                      .render_gl = (GLObjRenderFn)render_uniform_data,
                      .next = NULL};
  return append_obj(obj);
}
