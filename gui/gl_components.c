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
// Updated GLObj structure with render function
typedef bool (*GLObjInitFn)(void *state, void *obj);
typedef void (*GLObjRenderFn)(void *state, void *obj);

typedef struct GLObj {
  void *data;
  GLObjInitFn init_gl;
  GLObjRenderFn render_gl; // New: render function
  struct GLObj *next;
} GLObj;

typedef struct {
  const char *vertex_shader;
  const char *fragment_shader;

  float *vertices;
  int num_vertices;
  // OpenGL resources
  GLuint vao;
  GLuint vbo;
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

void gl_clear(double r, double g, double b, double alpha) {
  glClearColor(r, g, b, alpha);
  glClear(GL_COLOR_BUFFER_BIT);
}

void gl_bind_triangles(CustomOpenGLState *state) {
  glBindVertexArray(state->vao);
  glDrawArrays(GL_TRIANGLES, 0, state->num_vertices);
}
void gl_use_program(CustomOpenGLState *state) {
  glUseProgram(state->shader_program);
}

// Updated renderer to call each component's render method
void open_gl_decl_renderer(void *_state) {
  CustomOpenGLState *state = _state;

  // Clear buffers once at the start
  gl_clear(0.2, 0.3, 0.3, 1.0);

  // Use the shader program once
  gl_use_program(state);

  // Call render method for each component that has one
  GLObj *head = state->objs;
  while (head) {
    if (head->render_gl) {
      head->render_gl(state, head);
    }
    head = head->next;
  }
}

// Shader components don't need to render anything
void render_vshader(CustomOpenGLState *state, GLObj *obj) {
  // Shaders don't render, they just provide the program
  // This could be used for shader-specific uniforms later
}

void render_fshader(CustomOpenGLState *state, GLObj *obj) {
  // Fragment shaders don't render either
}

// Triangle component renders itself
void render_tri_data(CustomOpenGLState *state, GLObj *obj) {
  printf("Rendering triangle component\n");

  // Bind this triangle's VAO and draw
  glBindVertexArray(state->vao);
  glDrawArrays(GL_TRIANGLES, 0, state->num_vertices);

  // Could add component-specific uniforms here
  // For example: set material properties, transform matrices, etc.
}

// Example: Add a new component type - Points
typedef struct {
  int num_points;
  double *points;
  float point_size;
} PointData;

bool init_point_data(CustomOpenGLState *state, GLObj *obj) {
  PointData *point_data = (PointData *)obj->data;

  // Create separate VAO/VBO for points
  GLuint point_vao, point_vbo;
  glGenVertexArrays(1, &point_vao);
  glGenBuffers(1, &point_vbo);

  glBindVertexArray(point_vao);
  glBindBuffer(GL_ARRAY_BUFFER, point_vbo);

  // Convert double points to float
  float *float_points = malloc(sizeof(float) * point_data->num_points * 6);
  for (int i = 0; i < point_data->num_points * 6; i++) {
    float_points[i] = (float)point_data->points[i];
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * point_data->num_points * 6,
               float_points, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  // Store GL resources in the component data for later use
  // (You'd need to extend PointData struct to hold these)

  free(float_points);
  printf("Initialized %d points\n", point_data->num_points);
  return true;
}

void render_point_data(CustomOpenGLState *state, GLObj *obj) {
  PointData *point_data = (PointData *)obj->data;

  printf("Rendering %d points\n", point_data->num_points);

  // Set point size
  glPointSize(point_data->point_size);

  // Bind points VAO and draw
  // (You'd need to store the VAO in PointData during init)
  // glBindVertexArray(point_data->vao);
  // glDrawArrays(GL_POINTS, 0, point_data->num_points);
}

// Factory function for points
void *PointData(_DoubleArray points, double point_size, int num_points) {
  PointData *data = malloc(sizeof(PointData));
  data->points = points.data;
  data->point_size = (float)point_size;
  data->num_points = num_points;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = init_point_data,
                      .render_gl = render_point_data,
                      .next = NULL};
  return append_obj(obj);
}

// Example: Uniform component
typedef struct {
  const char *name;
  float value[4]; // Support up to vec4
  int components; // 1=float, 2=vec2, 3=vec3, 4=vec4
} UniformData;

bool init_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  // Uniforms don't need GL initialization
  // The location lookup happens during render
  return true;
}

void render_uniform_data(CustomOpenGLState *state, GLObj *obj) {
  UniformData *uniform = (UniformData *)obj->data;

  GLint location = glGetUniformLocation(state->shader_program, uniform->name);
  if (location == -1) {
    printf("Warning: uniform '%s' not found\n", uniform->name);
    return;
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
  }

  printf("Set uniform '%s' with %d components\n", uniform->name,
         uniform->components);
}

void *Uniform1f(const char *name, double value) {
  UniformData *data = malloc(sizeof(UniformData));
  data->name = name;
  data->value[0] = (float)value;
  data->components = 1;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = init_uniform_data,
                      .render_gl = render_uniform_data,
                      .next = NULL};
  return append_obj(obj);
}

void *Uniform3f(const char *name, double x, double y, double z) {
  UniformData *data = malloc(sizeof(UniformData));
  data->name = name;
  data->value[0] = (float)x;
  data->value[1] = (float)y;
  data->value[2] = (float)z;
  data->components = 3;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = init_uniform_data,
                      .render_gl = render_uniform_data,
                      .next = NULL};
  return append_obj(obj);
}
