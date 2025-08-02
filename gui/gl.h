#ifndef _LANG_GUI_GL_H
#define _LANG_GUI_GL_H
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
typedef void (*DeclGlFn)();
typedef struct {
  const char *vertex_shader;
  const char *fragment_shader;
  GLuint shader_program;
  DeclGlFn init_cb;
  void *objs;
} CustomOpenGLState;

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

#endif
