#include "input.h"
// clang-format off
// -- need to make sure stdio is included BEFORE readline
#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
// clang-format on
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

void init_readline() {
  rl_attempted_completion_function = custom_completion;
  rl_completion_entry_function = completion_generator;
  rl_read_init_file(NULL); // read .initrc
}

void repl_input_(char *input, int bufsize, const char *prompt) {
  char *line = readline(prompt);
  if (line == NULL) {
    // Handle EOF
    input[0] = '\0';
    return;
  }

  // Add input to history if non-empty
  if (*line) {
    add_history(line);
  }

  // Copy line to input buffer, ensuring we don't exceed bufsize
  strncpy(input, line, bufsize - 1);
  input[bufsize - 1] = '\0'; // Ensure null-termination

  // Handle line continuation
  while (strlen(input) > 0 && input[strlen(input) - 1] == '\\') {
    char *continuation = readline("  ");
    if (continuation == NULL) {
      // Handle EOF in continuation
      break;
    }

    // Replace '\' with '\n'
    input[strlen(input) - 1] = '\n';

    // Append continuation, ensuring we don't exceed bufsize
    int remaining = bufsize - strlen(input) - 1;
    if (remaining > 0) {
      strncat(input, continuation, remaining);
    }

    if (*continuation) {
      add_history(continuation);
    }

    free(continuation);

    // If we've filled the buffer, stop reading more
    if (strlen(input) >= bufsize - 1) {
      break;
    }
  }

  free(line);

  // Ensure the input ends with a newline and null terminator
  int len = strlen(input);
  if (len < bufsize - 1 && (len == 0 || input[len - 1] != '\n')) {
    input[len] = '\n';
    input[len + 1] = '\0';
  }
  // printf("%s", COLOR_RESET);
}

char *repl_input(const char *prompt) {
  char *line = readline(prompt);
  if (line == NULL) {
    // Handle EOF
    return NULL;
  }

  // Add input to history if non-empty
  if (*line) {
    add_history(line);
  }

  // Handle line continuation
  while (strlen(line) > 0 && line[strlen(line) - 1] == '\\') {
    char *continuation = readline("  ");
    if (continuation == NULL) {
      // Handle EOF in continuation
      break;
    }

    // Replace '\' with '\n'
    line[strlen(line) - 1] = '\n';

    // Reallocate line buffer to accommodate continuation
    size_t new_len = strlen(line) + strlen(continuation) + 1;
    char *new_line = realloc(line, new_len);
    if (new_line == NULL) {
      // Handle memory allocation failure
      free(line);
      free(continuation);
      return NULL;
    }
    line = new_line;

    // Append continuation
    strcat(line, continuation);

    if (*continuation) {
      add_history(continuation);
    }

    free(continuation);
  }

  // Ensure the input ends with a newline
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

char *read_script(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return NULL;
  }

  // Determine the size of the file
  fseek(fp, 0, SEEK_END); // Move the file pointer to the end of the file
  long fsize = ftell(fp); // Get the position, which is the file size
  rewind(fp);

  char *fcontent = (char *)malloc(fsize + 1);

  size_t bytes_read = fread(fcontent, 1, fsize, fp);

  if (bytes_read != fsize) {
    fprintf(stderr, "Error reading file: %s\n", filename);
    fclose(fp);
    free(fcontent); // Don't forget to free the allocated memory
    return NULL;
  }

  // Null-terminate the string
  fcontent[fsize] = '\0';
  fclose(fp);
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
