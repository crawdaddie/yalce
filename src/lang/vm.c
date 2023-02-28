#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "dbg.h"

VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }

void init_vm() {
  reset_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);
}

void free_vm() {
  free_table(&vm.globals);
  free_table(&vm.strings);
};

static bool is_falsy(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static Value peek(int distance) { return vm.stack_top[-1 - distance]; }

Value nadd(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for +"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) + AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
}

Value nsub(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for -"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) - AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b));
}

Value nmul(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for *"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) * AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b));
}

Value ndiv(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for /"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) / AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b));
}

Value nmod(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for %"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) % AS_INTEGER(b));
  }
  return NUMBER_VAL(fmod(AS_NUMBER(a), AS_NUMBER(b)));
}

Value ncompare(Value a, Value b, int lt, int inclusive) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for %"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    if (lt && inclusive) {
      return BOOL_VAL(AS_INTEGER(a) <= AS_INTEGER(b));
    }

    if (lt && !inclusive) {
      return BOOL_VAL(AS_INTEGER(a) < AS_INTEGER(b));
    }

    if (!lt && inclusive) {
      return BOOL_VAL(AS_INTEGER(a) >= AS_INTEGER(b));
    }

    if (!lt && !inclusive) {
      return BOOL_VAL(AS_INTEGER(a) > AS_INTEGER(b));
    }
  }

  if (lt && inclusive) {
    return BOOL_VAL(AS_NUMBER(a) <= AS_NUMBER(b));
  }

  if (lt && !inclusive) {
    return BOOL_VAL(AS_NUMBER(a) < AS_NUMBER(b));
  }

  if (!lt && !inclusive) {
    return BOOL_VAL(AS_NUMBER(a) > AS_NUMBER(b));
  }

  if (!lt && inclusive) {
    return BOOL_VAL(AS_NUMBER(a) >= AS_NUMBER(b));
  }
}
Value nnegate(Value a) {
  if (IS_INTEGER(a)) {
    return INTEGER_VAL(-AS_INTEGER(a));
  }
  return NUMBER_VAL(-AS_NUMBER(a));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
    // print out stack
    printf("stack: \n");
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");

#ifdef DEBUG_VM_CONSTANTS
    printf("constants: \n");
    printf("          ");
    for (Value *const_val = vm.chunk->constants.values;
         const_val < vm.chunk->constants.values + vm.chunk->constants.count;
         const_val++) {
      printf("[ ");
      print_value(*const_val);
      printf(" ]");
    }
    printf("\n");

#endif
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_RETURN: {
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
        /* runtime_error("Operand must be a number."); */
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

    case OP_LT: {
      Value b = pop();
      Value a = pop();
      push(ncompare(a, b, 1, 0));
      break;
    }

    case OP_GT: {
      Value b = pop();
      Value a = pop();
      push(ncompare(a, b, 0, 0));
      break;
    }

    case OP_LTE: {
      Value b = pop();
      Value a = pop();
      push(ncompare(a, b, 1, 1));
      break;
    }

    case OP_GTE: {
      Value b = pop();
      Value a = pop();
      push(ncompare(a, b, 0, 1));
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

    case OP_PRINT: {
      print_value(pop());
      printf("\n");
      break;
    }

    case OP_POP: {
      pop();
      break;
    }

    case OP_DEFINE_GLOBAL: {

      ObjString *name = READ_STRING();
      Value val = *(vm.stack_top - 1);
      table_set(&vm.globals, name, val);
      pop();
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!table_get(&vm.globals, name, &value)) {
        /* runtime_error("Undefined variable '%s'.", name->chars); */
        printf("Undefined variable '%s'\n", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        /* table_delete(&vm.globals, name); */
        /* return INTERPRET_RUNTIME_ERROR; */
      }
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(vm.stack[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm.stack[slot] = peek(0);
      break;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsy(peek(0)))
        vm.ip += offset;
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm.ip += offset;
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      vm.ip -= offset;
      break;
    }
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
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
  Value v = *vm.stack_top;
  return v;
}
