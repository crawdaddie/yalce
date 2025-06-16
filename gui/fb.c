#include "./common.h"
#include "./gl.h"
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

// ===== FRAMEBUFFER COMPONENT =====
typedef struct {
  const char *texture_uniform_name;
  void *scene_callback; // YL function that creates the scene
  GLuint framebuffer;
  GLuint color_texture;
  GLuint depth_renderbuffer;
  int width, height;

  // Internal scene state
  CustomOpenGLState *internal_state;
  bool scene_compiled;
  GLObj *saved_global_head; // Save global state during compilation
  GLObj *saved_global_tail;
} FrameBufferData;

bool init_framebuffer(CustomOpenGLState *state, GLObj *obj) {
  FrameBufferData *fb = (FrameBufferData *)obj->data;

  // Set default size if not specified
  fb->width = 512;
  fb->height = 512;

  // Create framebuffer
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

  // Create depth renderbuffer
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

  // Create internal OpenGL state for the framebuffer scene
  fb->internal_state = calloc(1, sizeof(CustomOpenGLState));
  fb->internal_state->shader_program = glCreateProgram();

  // Compile the internal scene
  // Save current global state
  fb->saved_global_head = _dcl_ctx_head;
  fb->saved_global_tail = _dcl_ctx_tail;

  // Reset global state for internal scene compilation
  _dcl_ctx_head = NULL;
  _dcl_ctx_tail = NULL;

  // Call the scene callback to build internal scene
  fb->internal_state->init_cb = (DeclGlFn)fb->scene_callback;
  fb->internal_state->init_cb();

  // Save the internal scene objects
  fb->internal_state->objs = _dcl_ctx_head;

  // Initialize internal scene objects
  GLObj *head = fb->internal_state->objs;
  while (head) {
    if (head->init_gl) {
      head->init_gl(fb->internal_state, head);
    }
    head = head->next;
  }

  // Link internal shader program
  glLinkProgram(fb->internal_state->shader_program);

  GLint success;
  glGetProgramiv(fb->internal_state->shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    printf("Internal framebuffer shader program failed to link!\n");
  }

  // Restore global state
  _dcl_ctx_head = fb->saved_global_head;
  _dcl_ctx_tail = fb->saved_global_tail;

  fb->scene_compiled = true;

  // Restore default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

void render_framebuffer(CustomOpenGLState *state, GLObj *obj) {
  FrameBufferData *fb = (FrameBufferData *)obj->data;

  if (!fb->scene_compiled) {
    return;
  }

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
  glUseProgram(fb->internal_state->shader_program);

  // Render internal scene
  GLObj *head = fb->internal_state->objs;
  while (head) {
    if (head->render_gl) {
      head->render_gl(fb->internal_state, head);
    }
    head = head->next;
  }

  // Restore main render state
  glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer);
  glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2],
             prev_viewport[3]);
  glUseProgram(prev_program);

  // Bind the framebuffer texture to the specified texture unit
  // Find an available texture unit (for simplicity, use GL_TEXTURE1)
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, fb->color_texture);

  // Set the texture uniform in the main shader
  GLint texture_loc =
      glGetUniformLocation(state->shader_program, fb->texture_uniform_name);
  if (texture_loc != -1) {
    glUniform1i(texture_loc, 1); // Use texture unit 1
  }

  // Reset to texture unit 0
  glActiveTexture(GL_TEXTURE0);
}

GLObj *FrameBuffer(_String utexture_name, void *cb) {
  FrameBufferData *data = malloc(sizeof(FrameBufferData));
  data->texture_uniform_name = utexture_name.chars;
  data->scene_callback = cb;
  data->scene_compiled = false;
  data->internal_state = NULL;
  data->framebuffer = 0;
  data->color_texture = 0;
  data->depth_renderbuffer = 0;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_framebuffer,
                      .render_gl = (GLObjRenderFn)render_framebuffer,
                      .next = NULL};
  return append_obj(obj);
}

// ===== TEXTURED QUAD COMPONENT =====
typedef struct {
  GLuint vao;
  GLuint vbo;
  GLuint ebo;
  float *gl_vertices;
  unsigned int *indices;
  GLObj *base_quad; // The original quad this texture is applied to
} TexturedQuadData;

void render_textured_quad(CustomOpenGLState *state, GLObj *obj) {
  TexturedQuadData *tq = (TexturedQuadData *)obj->data;

  // Render the base quad (this will use whatever texture is currently bound)
  if (tq->base_quad && tq->base_quad->render_gl) {
    tq->base_quad->render_gl(state, tq->base_quad);
  }
}

bool init_textured_quad(CustomOpenGLState *state, GLObj *obj) {
  TexturedQuadData *tq = (TexturedQuadData *)obj->data;

  // Initialize the base quad
  if (tq->base_quad && tq->base_quad->init_gl) {
    return tq->base_quad->init_gl(state, tq->base_quad);
  }
  return true;
}

GLObj *apply_texture(GLObj *framebuffer_obj, GLObj *quad_obj) {
  // The framebuffer will automatically bind its texture when rendered
  // The quad just needs to use the texture that's bound
  // For simplicity, we'll just return the quad since the framebuffer
  // handles texture binding in its render function

  TexturedQuadData *data = malloc(sizeof(TexturedQuadData));
  data->base_quad = quad_obj;

  GLObj obj = (GLObj){.data = data,
                      .init_gl = (GLObjInitFn)init_textured_quad,
                      .render_gl = (GLObjRenderFn)render_textured_quad,
                      .next = NULL};
  return append_obj(obj);
}
