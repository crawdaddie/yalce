#include "dbg.h"
void printf_color(char *fmt, int ansi, ...) {
  va_list args;
  va_start(args, ansi);
  printf("\033[%dm", ansi);
  vprintf(fmt, args);
  printf("\033[%dm", 37);
  printf("");
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
int disassemble_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
