#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "util.h"
VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }
void init_vm() { reset_stack(); }
void free_vm(){};

static bool is_falsy(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static Value peek(int distance) { return vm.stack_top[-1 - distance]; }
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_RETURN: {
      print_value(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    case OP_ADD: {
      Value b = pop();
      Value a = pop();
      push(nadd(a, b));
      break;
    }

    case OP_SUBTRACT: {
      Value b = pop();
      Value a = pop();
      push(nsub(a, b));
      break;
    }

    case OP_MULTIPLY: {
      Value b = pop();
      Value a = pop();
      push(nmul(a, b));
      break;
    }

    case OP_DIVIDE: {
      Value b = pop();
      Value a = pop();
      push(ndiv(a, b));
      break;
    }
    case OP_NOT:
      push(BOOL_VAL(is_falsy(pop())));
      break;

    case OP_NEGATE:
      if (!IS_NUMERIC(peek(0))) {
        /* runtimeError("Operand must be a number."); */
        return INTERPRET_RUNTIME_ERROR;
      }
      push(nnegate(pop()));
      break;

    case OP_MODULO: {
      Value b = pop();
      Value a = pop();
      push(nmod(a, b));
      break;
    }

    case OP_NIL: {
      push(NIL_VAL);
      break;
    }

    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }

    case OP_FALSE: {
      push(BOOL_VAL(false));
      break;
    }

    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_CALL: {
      int arg_count = READ_BYTE();
      printf("arg count %d\n", arg_count);
      break;
    }
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  init_chunk(&chunk);
  if (!compile(source, &chunk)) {
    return INTERPRET_COMPILE_ERROR;
  }
  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
  InterpretResult result = run();
  free_chunk(&chunk);
  return result;
}

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}
