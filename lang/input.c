#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void repl_input(char *input, int bufsize, const char *prompt) {
  char prev;
  char c;
  int position = 0;
  if (prompt == NULL) {
    prompt = "\033[1;31mλ \033[1;0m"
             "\033[1;36m";
  }

  printf("%s", prompt);
  while (1) {
    prev = c;
    c = getchar();

    if (c == 'n' && prev == '\\') {
      input[position - 1] = '\n';
      continue;
    }

    if (c == EOF || c == '\n') {
      if (prev == '\\') {
        return repl_input(input + position, bufsize, "  ");
      }
      input[position] = '\n';
      input[++position] = '\0';
      return;
    }
    if (position == bufsize) {
      printf("input exceeds bufsize\n");
      // TODO: increase size of input buffer
    }

    input[position] = c;
    position++;
  }
  printf("\033[1;0m");
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

char *get_dirname(const char *path) {
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
