#include "dbg.h"
#include "chunk.h"
#include "list.h"
#include "obj.h"
#include "obj_function.h"
#include "util.h"

#define COLOR 0
void printf_color(char *fmt, int ansi, ...) {
  va_list args;
  va_start(args, ansi);
  if (COLOR)
    printf("\033[%dm", ansi);
  vprintf(fmt, args);
  if (COLOR)
    printf("\033[%dm", 37);
  va_end(args);
}

static void print_function(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}
void print_value(Value val);
void print_object(Object *object) {
  switch (object->type) {
  case OBJ_STRING: {
    ObjString *str = (ObjString *)object;
    printf_color("%s", 96, str->chars);
    break;
  }

  case OBJ_FUNCTION: {
    print_function((ObjFunction *)object);
    break;
  }
  case OBJ_LIST: {
    ObjList *l = (ObjList *)object;
    for (int i = 0; i < l->length; i++) {
      print_value(l->values[i]);
      printf(", ");
    }
    break;
  }
  default:
    break;
  }
}
void print_value(Value val) {
  switch (val.type) {

  case VAL_BOOL:
    printf(AS_BOOL(val) ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf_color("%lf", 96, val.as.number);
    break;
  case VAL_INTEGER:
    printf_color("%d", 96, val.as.integer);
    break;
  case VAL_OBJ:
    print_object(AS_OBJ(val));
    break;
  default:
    break;
  }
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
static int byte_instruction(const char *name, Chunk *chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}
static int jump_instruction(const char *name, int sign, Chunk *chunk,
                            int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
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
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simple_instruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simple_instruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simple_instruction("OP_DIVIDE", offset);
  case OP_LT:
    return simple_instruction("OP_LT", offset);
  case OP_GT:
    return simple_instruction("OP_GT", offset);

  case OP_LTE:
    return simple_instruction("OP_LTE", offset);
  case OP_GTE:
    return simple_instruction("OP_GTE", offset);
  case OP_NIL:
    return simple_instruction("OP_NIL", offset);
  case OP_TRUE:
    return simple_instruction("OP_TRUE", offset);
  case OP_FALSE:
    return simple_instruction("OP_FALSE", offset);
  case OP_NEGATE:
    return simple_instruction("OP_NEGATE", offset);
  case OP_NOT:
    return simple_instruction("OP_NOT", offset);
  case OP_MODULO:
    return simple_instruction("OP_MODULO", offset);
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_POP:
    return simple_instruction("OP_POP", offset);
  case OP_DEFINE_GLOBAL:
    return simple_instruction("OP_DEFINE_GLOBAL", offset);
  case OP_GET_GLOBAL:
    return constant_instruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL:
    return constant_instruction("OP_SET_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return byte_instruction("OP_GET_LOCAL", chunk, offset);
  case OP_SET_LOCAL:
    return byte_instruction("OP_SET_LOCAL", chunk, offset);
  case OP_JUMP:
    return jump_instruction("OP_JUMP", 1, chunk, offset);
  case OP_JUMP_IF_FALSE:
    return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
  case OP_LOOP:
    return jump_instruction("OP_LOOP", -1, chunk, offset);

  case OP_CALL:
    return byte_instruction("OP_CALL", chunk, offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
