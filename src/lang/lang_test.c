#include "chunk.h"
#include "dbg.h"
#include "parse.tab.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

void parse_file(char const *const filename) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(filename, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }

  if (buffer) {
    parse_line(buffer, 0);
  }
}
static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\"", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\"", path);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

void run_file(const char *path) {
  char *source = read_file(path);
  InterpretResult result = interpret(source);
  free(source);
  if (result == INTERPRET_COMPILE_ERROR)
    exit(65);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(70);
}

void parse_lines(char const *const filename) {
  FILE *file = fopen(filename, "r");
  char *line = NULL;
  size_t line_len = 0;
  int i = 0;
  int read;

  while ((read = getline(&line, &line_len, file)) != -1) {
    parse_line(line, i);
    i++;
  }

  fclose(file);
}

int main(int argc, char **argv) {
  int repl = 0;
  repl = argc == 1;

  init_vm();
  Chunk chunk;
  init_chunk(&chunk);
  /* int constant = add_constant(&chunk, NUMBER_VAL(1.2)); */
  /* write_chunk(&chunk, OP_CONSTANT); */
  /* write_chunk(&chunk, constant); */
  /*  */
  /* constant = add_constant(&chunk, NUMBER_VAL(3.4)); */
  /* write_chunk(&chunk, OP_CONSTANT); */
  /* write_chunk(&chunk, constant); */
  /*  */
  /* write_chunk(&chunk, OP_ADD); */
  /*  */
  /* constant = add_constant(&chunk, NUMBER_VAL(5.6)); */
  /* write_chunk(&chunk, OP_CONSTANT); */
  /* write_chunk(&chunk, constant); */
  /*  */
  /* write_chunk(&chunk, OP_DIVIDE); */
  /* write_chunk(&chunk, OP_RETURN); */
  /*  */
  /* interpret(&chunk); */
  /* free_chunk(&chunk); */

  init_table();
  if (argc == 2) {
    char *filename = argv[1];
    run_file(filename);
  }

  if (repl) {
    char input[2048];
    for (;;) {
      fgets(input, 2048, stdin);
      parse_line(input, 0);
    }
  }
  free_vm();
}
