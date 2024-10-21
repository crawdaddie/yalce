#include "ctx.h"
#ifdef GUI_MODE
#include "../gui/gui.h"

#endif
#include "backend_llvm/jit.h"
#include "format_utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef GUI_MODE
int create_scope() {
  Ctx *audio_ctx = get_audio_ctx();
  double *output = audio_ctx->output_buf;
  _create_scope(output);
  return 1;
}

int plot_sig(SignalRef sig) {
  _create_plot_array_window(sig->size, sig->buf);
  return 1;
}
#endif

// Global variables for thread synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool jit_completed = false;

struct thread_args {
  int argc;
  char **argv;
} thread_args;

void *run_jit(void *arg) {
  struct thread_args *args = (struct thread_args *)arg;
  jit(args->argc, args->argv);
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t jit_thread;
  int jit_result;
  bool run_gui = false;

#ifdef GUI_MODE
  // Check for --gui argument
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--gui") == 0) {
      run_gui = true;
      break;
    }
  }
#endif

  if (run_gui) {
#ifdef GUI_MODE
    // printf("run gui\n");
    if (init_gui()) {
      return 1;
    }

    // Start JIT thread
    struct thread_args thread_args = {argc, argv};
    if (pthread_create(&jit_thread, NULL, run_jit, &thread_args) != 0) {
      perror("Failed to create JIT thread");
      return 1;
    }

    printf(COLOR_MAGENTA "------------------\n"
                         "GUI MODE ENABLED  \n" STYLE_RESET_ALL);

    gui_loop();

#endif
  } else {
    return jit(argc, argv);
  }
}
