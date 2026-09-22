#include "input.h"
// clang-format off
// -- need to make sure stdio is included BEFORE readline
#include <signal.h>
#include <stdio.h>
#include <poll.h>
#include <readline/history.h>
#include <readline/readline.h>
// clang-format on
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMPLETIONS 100
const char *completions_array[MAX_COMPLETIONS] = {
    "fn",        "let",   "in",    "and",  "()",     "extern",
    "true",      "false", "match", "with", "import", "%dump_type_env",
    "%dump_ast",
};

static int completion_count = 13;

void add_completion_item(const char *item, int count) {
  completions_array[11 + count] = item;
  completion_count++;
}

char *completion_generator(const char *text, int state) {
  static int list_index;
  static int len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while (list_index < completion_count) {
    const char *name = completions_array[list_index];
    list_index++;

    if (strncmp(name, text, len) == 0) {
      return strdup(name);
    }
  }

  return NULL;
}

char **custom_completion(const char *text, int start, int end) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, completion_generator);
}

#define HISTORY_FILE ".repl_history"
#define MAX_HISTORY_LEN 1000
void load_history() {
  if (read_history(HISTORY_FILE) != 0) {
    printf("No existing history file found, starting fresh.\n");
  }
  stifle_history(MAX_HISTORY_LEN);
}

void save_history() {
  if (write_history(HISTORY_FILE) != 0) {
    fprintf(stderr, "Warning: Could not save history to %s\n", HISTORY_FILE);
  }
}
void sigint_handler(int sig) {
  save_history();
  exit(0);
}
void init_readline() {
  rl_attempted_completion_function = custom_completion;
  rl_completion_entry_function = completion_generator;
  rl_read_init_file(NULL); // read .initrc

  load_history();

  signal(SIGINT, sigint_handler);
}
char *repl_input(const char *prompt) {
  char *line = readline(prompt);
  if (line == NULL) {
    return NULL;
  }

  if (*line) {
    add_history(line);
  }

  while (strlen(line) > 0 && line[strlen(line) - 1] == '\\') {
    char *continuation = readline("  ");
    if (continuation == NULL) {
      // Handle EOF in continuation
      break;
    }

    line[strlen(line) - 1] = '\n';

    size_t new_len = strlen(line) + strlen(continuation) + 1;
    char *new_line = realloc(line, new_len);
    if (new_line == NULL) {
      free(line);
      free(continuation);
      return NULL;
    }
    line = new_line;

    strcat(line, continuation);

    if (*continuation) {
      add_history(continuation);
    }

    free(continuation);
  }

  size_t len = strlen(line);
  if (len == 0 || line[len - 1] != '\n') {
    char *new_line = realloc(line, len + 2);
    if (new_line == NULL) {
      // Handle memory allocation failure
      free(line);
      return NULL;
    }
    line = new_line;
    line[len] = '\n';
    line[len + 1] = '\0';
  }

  return line;
}

static char *nb_accum = NULL;
static size_t nb_accum_len = 0;
static YlcReplLineCb nb_cb = NULL;
static void *nb_userdata = NULL;
static bool nb_active = false;

static void repl_nb_reset(void) {
  free(nb_accum);
  nb_accum = NULL;
  nb_accum_len = 0;
}

static void repl_nb_submit(char *line) {
  size_t len = strlen(line);

  if (len > 0 && line[len - 1] == '\\') {
    size_t new_len = nb_accum_len + len;
    char *tmp = realloc(nb_accum, new_len + 1);
    if (!tmp) {
      free(line);
      return;
    }

    nb_accum = tmp;
    memcpy(nb_accum + nb_accum_len, line, len - 1);
    nb_accum[nb_accum_len + len - 1] = '\n';
    nb_accum[nb_accum_len + len] = '\0';
    nb_accum_len = new_len;
    free(line);
    return;
  }

  size_t final_len = nb_accum_len + len + 2;
  char *input = malloc(final_len);
  if (!input) {
    free(line);
    return;
  }

  if (nb_accum_len > 0) {
    memcpy(input, nb_accum, nb_accum_len);
  }
  memcpy(input + nb_accum_len, line, len);
  input[nb_accum_len + len] = '\n';
  input[nb_accum_len + len + 1] = '\0';
  free(line);
  repl_nb_reset();

  if (nb_cb) {
    nb_cb(input, nb_userdata);
  }
  free(input);
}

static void repl_nb_line_ready(char *line) {
  if (!line) {
    repl_nb_reset();
    rl_callback_handler_remove();
    nb_active = false;
    return;
  }

  if (*line) {
    add_history(line);
  }
  repl_nb_submit(line);
}

void ylc_repl_begin(const char *prompt, YlcReplLineCb cb, void *userdata) {
  nb_cb = cb;
  nb_userdata = userdata;
  nb_active = true;
  rl_callback_handler_install(prompt, repl_nb_line_ready);
}

void ylc_repl_poll_stdin(void) {
  if (!nb_active) {
    return;
  }

  struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};
  if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
    rl_callback_read_char();
  }
}

void ylc_repl_feed_text(const char *text) {
  if (!nb_active || !text) {
    return;
  }

  rl_insert_text(text);
  rl_redisplay();
}

static void repl_call_named(const char *name, int key) {
  rl_command_func_t *fn = rl_named_function(name);
  if (fn) {
    fn(1, key);
    rl_redisplay();
  }
}

void ylc_repl_feed_key(int key) {
  if (!nb_active) {
    return;
  }

  if (key == YLC_REPL_KEY_BACKSPACE) {
    if (rl_point > 0) {
      rl_delete_text(rl_point - 1, rl_point);
      rl_point--;
      rl_redisplay();
    }
    return;
  }

  if (key == YLC_REPL_KEY_ENTER) {
    char *line = strdup(rl_line_buffer ? rl_line_buffer : "");
    rl_replace_line("", 0);
    rl_point = 0;
    rl_redisplay();
    repl_nb_line_ready(line);
    return;
  }

  if (key == YLC_REPL_KEY_LEFT) {
    repl_call_named("backward-char", key);
  } else if (key == YLC_REPL_KEY_RIGHT) {
    repl_call_named("forward-char", key);
  } else if (key == YLC_REPL_KEY_UP) {
    repl_call_named("previous-history", key);
  } else if (key == YLC_REPL_KEY_DOWN) {
    repl_call_named("next-history", key);
  }
}

char *read_script(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return NULL;
  }

  fseek(fp, 0, SEEK_END); // Move the file pointer to the end of the file
  long fsize = ftell(fp); // Get the position, which is the file size
  rewind(fp);

  char *fcontent = (char *)malloc(fsize + 1);

  size_t bytes_read = fread(fcontent, 1, fsize, fp);
  fclose(fp);

  if (bytes_read != fsize) {
    fprintf(stderr, "Error reading file: %s\n", filename);
    fclose(fp);
    free(fcontent);
    return NULL;
  }

  fcontent[fsize] = '\0';

  return fcontent;
}

char *_get_dirname(const char *path) {
  // Make a copy of the path to avoid modifying the original
  char *path_copy = strdup(path);
  if (path_copy == NULL) {
    return NULL; // Memory allocation failed
  }

  // Find the last occurrence of '/' or '\'
  char *last_slash = strrchr(path_copy, '/');
  char *last_backslash = strrchr(path_copy, '\\');
  char *last_separator =
      (last_slash > last_backslash) ? last_slash : last_backslash;

  if (last_separator == NULL) {
    // No directory separator found, return "." for current directory
    free(path_copy);
    return strdup(".");
  }

  // Null-terminate the string at the last separator
  *last_separator = '\0';

  // If the path is now empty (e.g., "/file.txt"), return "/"
  if (path_copy[0] == '\0' && (path[0] == '/' || path[0] == '\\')) {
    free(path_copy);
    return NULL;
  }

  // Return the modified path
  return path_copy;
}
char *get_dirname(const char *path) {
  char *last_slash = strrchr(path, '/');
  if (last_slash == NULL) {
    return strdup(".");
  }
  if (last_slash == path) {
    return strdup("/");
  }
  size_t len = last_slash - path;
  char *dirname = malloc(len + 1);
  if (dirname == NULL) {
    return NULL;
  }
  strncpy(dirname, path, len);
  dirname[len] = '\0';
  return dirname;
}
const char *get_mod_name_from_path_identifier(const char *str) {
  if (!str)
    return NULL;

  const char *last_slash = strrchr(str, '/');
  if (last_slash) {
    return last_slash + 1;
  } else {
    return str;
  }
}

char *resolve_relative_path(const char *base_path, const char *relative_path) {
  char *result = malloc(strlen(base_path) + strlen(relative_path) + 2);
  if (result == NULL) {
    return NULL;
  }
  strcpy(result, base_path);
  strcat(result, "/");
  strcat(result, relative_path);
  return result;
}

char *normalize_path(const char *path) {
  char *normalized = strdup(path);
  if (normalized == NULL) {
    return NULL;
  }

  char *src = normalized;
  char *dst = normalized;
  int depth = 0;

  while (*src) {
    if (src[0] == '/') {
      *dst++ = *src++;
      while (*src == '/')
        src++;
    } else if (src[0] == '.' && (src[1] == '/' || src[1] == '\0')) {
      src += 1;
      if (*src)
        src++;
    } else if (src[0] == '.' && src[1] == '.' &&
               (src[2] == '/' || src[2] == '\0')) {
      if (depth > 0) {
        depth--;
        if (dst > normalized + 1) {
          dst--;
          while (dst > normalized && *(dst - 1) != '/')
            dst--;
        }
      } else {
        *dst++ = '.';
        *dst++ = '.';
        if (src[2] == '/')
          *dst++ = '/';
      }
      src += 2;
      if (*src)
        src++;
    } else {
      depth++;
      while (*src && *src != '/')
        *dst++ = *src++;
    }
  }

  if (dst == normalized) {
    *dst++ = '.';
  }
  *dst = '\0';

  return normalized;
}
