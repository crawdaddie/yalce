#include "dbg.h"
#include "util.h"
#define COLOR 0
void printf_color(char *fmt, int ansi, ...) {
  va_list args;
  va_start(args, ansi);
  if (COLOR) printf("\033[%dm", ansi);
  vprintf(fmt, args);
  if (COLOR) printf("\033[%dm", 37);
  va_end(args);
}
void disassemble_chunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassemble_instruction(chunk, offset);
  }
}
static int simple_instruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}
static int constant_instruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int disassemble_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
