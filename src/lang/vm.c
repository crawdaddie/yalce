#include "vm.h"
#include "../bindings.h"
#include "../graph/graph.h"
#include "common.h"
#include "compiler.h"
#include "dbg.h"
#include "obj_graph.h"
#include <math.h>
#include <time.h>

VM vm;

void define_native(const char *name, NativeFn function) {
  push(make_string_val((char *)name));
  push((Value){VAL_OBJ, {.object = (Object *)make_native(function)}});
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void make_bindings() {
  for (int i = 0; i < NUM_BINDINGS; i++) {
    Binding binding = bindings[i];
    define_native(binding.name, binding.function);
  }
}

static void reset_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
  vm.open_upvalues = NULL;
}

void init_vm() {
  reset_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);
  make_bindings();
}

void free_vm() {
  free_table(&vm.globals);
  free_table(&vm.strings);
  vm.open_upvalues = NULL;
};

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  CallFrame *frame = &vm.frames[vm.frame_count - 1];
  size_t instruction = frame->ip - frame->closure->function->chunk.code - 1;
  /* int line = frame->function->chunk.lines[instruction]; */
}

static bool is_falsy(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static Value peek(int distance) { return vm.stack_top[-1 - distance]; }

static bool call(ObjClosure *closure, int arg_count) {
  ObjFunction *function = closure->function;
  if (arg_count != function->arity) {
    runtime_error("Expected %d arguments but got %d", function->arity,
                  arg_count);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frame_count++];

  /* frame->function = function; */
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}
static bool call_value(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_FUNCTION:
      return call(AS_FUNCTION(callee), arg_count);

    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(arg_count, vm.stack_top - arg_count);
      vm.stack_top -= arg_count + 1;
      if (!IS_VOID(result)) {
        push(result);
      }
      return true;
    }
    case OBJ_CLOSURE: {
      return call(AS_CLOSURE(callee), arg_count);
    }
    default:
      break;
    }
  }
  runtime_error("Can only call functions");
  return false;
}
static ObjUpvalue *capture_upvalue(Value *local) {
  ObjUpvalue *prev_upvalue = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;
  while (upvalue != NULL && (Value *)upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = (ObjUpvalue *)upvalue->next;
  }
  if (upvalue != NULL && (Value *)upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue *created_upvalue = make_upvalue(local);
  created_upvalue->next = upvalue;
  if (prev_upvalue == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }
  return created_upvalue;
}
static void close_upvalues(Value *last) {
  while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
    ObjUpvalue *upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

Value nadd(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) + AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
}

Value nsub(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) - AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b));
}

Value nmul(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) * AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b));
}

Value ndiv(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) / AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b));
}

Value nmod(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) % AS_INTEGER(b));
  }
  return NUMBER_VAL(fmod(AS_NUMBER(a), AS_NUMBER(b)));
}

Value ncompare(Value a, Value b, int lt, int inclusive) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
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

Value pipe_values(Value a, Value b) {
  Graph *graph_a = ((ObjGraph *)a.as.object)->graph;
  Graph *graph_b = ((ObjGraph *)b.as.object)->graph;
  pipe_graph(graph_a, graph_b);
  return a;
}

Value nnegate(Value a) {
  if (IS_INTEGER(a)) {
    return INTEGER_VAL(-AS_INTEGER(a));
  }
  return NUMBER_VAL(-AS_NUMBER(a));
}
static void jump_ip(CallFrame *frame, int offset) { frame->ip += offset; }
static InterpretResult run() {
  CallFrame *frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

  for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
    disassemble_instruction(
        &frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
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
      Value result = pop();
      close_upvalues(frame->slots);
      vm.frame_count--;
      if (vm.frame_count == 0) {
        pop();
        return INTERPRET_OK;
      }
      vm.stack_top = frame->slots;
      push(result);
      frame = &vm.frames[vm.frame_count - 1];
      break;
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
        runtime_error("Operand must be a number.");
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

      if (!call_value(peek(arg_count), arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
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
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        /* table_delete(&vm.globals, name); */
        return INTERPRET_RUNTIME_ERROR;
      }
      pop();
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      pop();
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*(Value *)(frame->closure->upvalues[slot]->location));
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *(Value *)(frame->closure->upvalues[slot]->location) = peek(0);
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsy(peek(0)))
        jump_ip(frame, offset);
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      jump_ip(frame, offset);
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      jump_ip(frame, -offset);
      break;
    }

    case OP_PIPE: {
      Value b = pop();
      Value a = pop();
      push(pipe_values(a, b));
      break;
    }

    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = make_closure(function);
      push((Value){VAL_OBJ, {.object = (Object *)closure}});
      for (int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (is_local) {
          closure->upvalues[i] = capture_upvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE: {
      close_upvalues(vm.stack_top - 1);
      pop();
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
  ObjFunction *compiled = compile(source, &chunk);
  if (!compiled) {
    return INTERPRET_COMPILE_ERROR;
  }
  push(((Value){VAL_OBJ, {.object = (Object *)compiled}}));
  ObjClosure *closure = make_closure(compiled);
  pop();
  push((Value){VAL_OBJ, {.object = (Object *)closure}});
  call(closure, 0);

  return run();
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
