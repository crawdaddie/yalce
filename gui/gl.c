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
  glClear(GL_COLOR_BUFFER_BIT);
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

// ===== CAMERA COMPONENT =====
typedef struct {
  // Camera position and orientation
  float position[3];
  float target[3];
  float up[3];

  // Projection parameters
  float fov;        // Field of view in degrees
  float aspect;     // Aspect ratio (width/height)
  float near_plane; // Near clipping plane
  float far_plane;  // Far clipping plane

  // Computed matrices (4x4, column-major for OpenGL)
  float view_matrix[16];
  float projection_matrix[16];
  float mvp_matrix[16];

  // Uniform names
  char *view_uniform_name;
  char *projection_uniform_name;
  char *mvp_uniform_name;
} CameraData;

// Matrix math utilities
void mat4_identity(float *matrix) {
  for (int i = 0; i < 16; i++) {
    matrix[i] = 0.0f;
  }
  matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void mat4_multiply(float *result, const float *a, const float *b) {
  float temp[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp[i * 4 + j] = 0.0f;
      for (int k = 0; k < 4; k++) {
        temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
      }
    }
  }
  memcpy(result, temp, sizeof(temp));
}

void vec3_normalize(float *vec) {
  float length = sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
  if (length > 0.0f) {
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
  }
}

void vec3_cross(float *result, const float *a, const float *b) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}

void vec3_subtract(float *result, const float *a, const float *b) {
  result[0] = a[0] - b[0];
  result[1] = a[1] - b[1];
  result[2] = a[2] - b[2];
}

// Create look-at view matrix
void camera_look_at(float *view_matrix, const float *eye, const float *center,
                    const float *up) {
  float f[3], s[3], u[3];

  // Forward vector (from eye to center)
  vec3_subtract(f, center, eye);
  vec3_normalize(f);

  // Right vector
  vec3_cross(s, f, up);
  vec3_normalize(s);

  // Up vector
  vec3_cross(u, s, f);

  mat4_identity(view_matrix);

  view_matrix[0] = s[0];
  view_matrix[4] = s[1];
  view_matrix[8] = s[2];
  view_matrix[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);

  view_matrix[1] = u[0];
  view_matrix[5] = u[1];
  view_matrix[9] = u[2];
  view_matrix[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);

  view_matrix[2] = -f[0];
  view_matrix[6] = -f[1];
  view_matrix[10] = -f[2];
  view_matrix[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];

  view_matrix[3] = 0.0f;
  view_matrix[7] = 0.0f;
  view_matrix[11] = 0.0f;
  view_matrix[15] = 1.0f;
}

// Create perspective projection matrix
void camera_perspective(float *proj_matrix, float fov_degrees, float aspect,
                        float near_plane, float far_plane) {
  float fov_rad = fov_degrees * M_PI / 180.0f;
  float f = 1.0f / tanf(fov_rad / 2.0f);

  mat4_identity(proj_matrix);

  proj_matrix[0] = f / aspect;
  proj_matrix[5] = f;
  proj_matrix[10] = (far_plane + near_plane) / (near_plane - far_plane);
  proj_matrix[11] = -1.0f;
  proj_matrix[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
  proj_matrix[15] = 0.0f;
}

// Update camera matrices
void camera_update_matrices(CameraData *camera) {
  // Update view matrix
  camera_look_at(camera->view_matrix, camera->position, camera->target,
                 camera->up);

  // Update projection matrix
  camera_perspective(camera->projection_matrix, camera->fov, camera->aspect,
                     camera->near_plane, camera->far_plane);

  // Combine into MVP matrix (MVP = Projection * View * Model)
  // For now, Model matrix is identity
  mat4_multiply(camera->mvp_matrix, camera->projection_matrix,
                camera->view_matrix);
}

// Camera initialization (no GL resources needed)
bool init_camera_data(CustomOpenGLState *state, GLObj *obj) {

  CameraData *camera = (CameraData *)obj->data;

  // Update matrices on initialization
  camera_update_matrices(camera);

  printf("Camera initialized: pos(%.2f,%.2f,%.2f) target(%.2f,%.2f,%.2f) "
         "fov=%.1f '%s'\n",
         camera->position[0], camera->position[1], camera->position[2],
         camera->target[0], camera->target[1], camera->target[2], camera->fov,
         camera->mvp_uniform_name);

  return true;
}

// Camera render function - uploads matrices as uniforms
bool render_camera_data(CustomOpenGLState *state, GLObj *obj) {
  CameraData *camera = (CameraData *)obj->data;

  // printf("Setting camera uniforms\n");

  // Upload view matrix
  if (camera->view_uniform_name) {
    GLint view_location =
        glGetUniformLocation(state->shader_program, camera->view_uniform_name);
    if (view_location != -1) {
      glUniformMatrix4fv(view_location, 1, GL_FALSE, camera->view_matrix);
      printf("Set view matrix uniform '%s'\n", camera->view_uniform_name);
    } else {
      printf("Warning: view uniform '%s' not found\n",
             camera->view_uniform_name);
    }
  }

  // Upload projection matrix
  if (camera->projection_uniform_name) {
    GLint proj_location = glGetUniformLocation(state->shader_program,
                                               camera->projection_uniform_name);
    if (proj_location != -1) {
      glUniformMatrix4fv(proj_location, 1, GL_FALSE, camera->projection_matrix);
      printf("Set projection matrix uniform '%s'\n",
             camera->projection_uniform_name);
    } else {
      printf("Warning: projection uniform '%s' not found\n",
             camera->projection_uniform_name);
    }
  }

  // Upload combined MVP matrix
  if (camera->mvp_uniform_name) {
    GLint mvp_location =
        glGetUniformLocation(state->shader_program, camera->mvp_uniform_name);
    if (mvp_location != -1) {
      glUniformMatrix4fv(mvp_location, 1, GL_FALSE, camera->mvp_matrix);
      // printf("Set MVP matrix uniform '%s'\n", camera->mvp_uniform_name);
    } else {
      // printf("Warning: MVP uniform '%s' not found\n",
      // camera->mvp_uniform_name);
    }
  }

  return true;
}

// Factory function for camera
void *Camera(double pos_x, double pos_y, double pos_z, double target_x,
             double target_y, double target_z, double fov, double aspect,
             double near_plane, double far_plane, _String mvp_uniform_name) {

  CameraData *data =
      malloc(sizeof(CameraData) + ((mvp_uniform_name.size + 1) * sizeof(char)));

  // Set position
  data->position[0] = (float)pos_x;
  data->position[1] = (float)pos_y;
  data->position[2] = (float)pos_z;

  // Set target
  data->target[0] = (float)target_x;
  data->target[1] = (float)target_y;
  data->target[2] = (float)target_z;

  // Set up vector (Y-up)
  data->up[0] = 0.0f;
  data->up[1] = 1.0f;
  data->up[2] = 0.0f;

  // Set projection parameters
  data->fov = (float)fov;
  data->aspect = (float)aspect;
  data->near_plane = (float)near_plane;
  data->far_plane = (float)far_plane;

  // Set uniform names
  data->view_uniform_name = NULL;
  data->projection_uniform_name = NULL;
  data->mvp_uniform_name = (char *)(data + 1);
  memcpy(data->mvp_uniform_name, mvp_uniform_name.chars, mvp_uniform_name.size);
  data->mvp_uniform_name[mvp_uniform_name.size] = '\0'; // ADD THIS LINE!

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_camera_data,
                      .render_gl = (GLObjRenderFn)render_camera_data,
                      .next = NULL};
  return append_obj(obj);
}

// Alternative factory with separate view/projection uniforms
void *CameraSeparate(double pos_x, double pos_y, double pos_z, double target_x,
                     double target_y, double target_z, double fov,
                     double aspect, double near_plane, double far_plane,
                     _String view_uniform_name,
                     _String projection_uniform_name) {

  CameraData *data = malloc(sizeof(CameraData));

  data->position[0] = (float)pos_x;
  data->position[1] = (float)pos_y;
  data->position[2] = (float)pos_z;

  data->target[0] = (float)target_x;
  data->target[1] = (float)target_y;
  data->target[2] = (float)target_z;

  data->up[0] = 0.0f;
  data->up[1] = 1.0f;
  data->up[2] = 0.0f;

  data->fov = (float)fov;
  data->aspect = (float)aspect;
  data->near_plane = (float)near_plane;
  data->far_plane = (float)far_plane;

  data->view_uniform_name = view_uniform_name.chars;
  data->projection_uniform_name = projection_uniform_name.chars;
  data->mvp_uniform_name = NULL;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_camera_data,
                      .render_gl = (GLObjRenderFn)render_camera_data,
                      .next = NULL};
  return append_obj(obj);
}

// Simple orbital camera factory
void *OrbitCamera(double radius, double angle_x, double angle_y,
                  double target_x, double target_y, double target_z, double fov,
                  double aspect, _String mvp_uniform_name) {

  // Calculate position from spherical coordinates
  double pos_x = target_x + radius * cos(angle_y) * sin(angle_x);
  double pos_y = target_y + radius * sin(angle_y);
  double pos_z = target_z + radius * cos(angle_y) * cos(angle_x);

  return Camera(pos_x, pos_y, pos_z, target_x, target_y, target_z, fov, aspect,
                0.1, 100.0, mvp_uniform_name);
}
// Add matrix uniform support to your existing uniform system
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
    // printf("Set matrix uniform '%s'\n", uniform->name);
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

// NEW: Matrix uniform factory
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
