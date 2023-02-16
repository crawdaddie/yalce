#include "chunk.h"
#include "dbg.h"
#include "parse.tab.h"
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
  int last_index = 1;
  for (int i = 0; i < argc; i++) {
    char *arg = argv[i];
    if (strcmp(arg, "--repl") == 0) {
      repl = 1;
      continue;
    }
    if (i == argc - 1) {
      last_index = i;
      continue;
    }
  }
  char *filename = argv[last_index];

  Chunk chunk;
  init_chunk(&chunk);
  int constant = add_constant(&chunk, NUMBER_VAL(1.2));

  write_chunk(&chunk, OP_CONSTANT);
  write_chunk(&chunk, constant);
  write_chunk(&chunk, OP_RETURN);

  /* disassemble_chunk(&chunk, "test chunk"); */
  free_chunk(&chunk);

  init_table();
  if (argc > 1) {
    parse_file(filename);
  }

  if (repl) {
    char input[2048];
    for (;;) {
      fgets(input, 2048, stdin);
      parse_line(input, 0);
    }
  }
}
