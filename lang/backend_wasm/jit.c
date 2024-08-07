#include "parse.h"
#include "serde.h"
#include "types/inference.h"
#include "types/util.h"
#include <stdlib.h>
#include <string.h>

typedef enum Op {
  Unreachable = 0x00,
  Nop = 0x01,
  Block = 0x02,
  Loop = 0x03,
  If = 0x04,
  Else = 0x05,
  End = 0x0b,
  Br = 0x0c,
  BrIf = 0x0d,
  Return = 0x0f,

  // Call operators
  Call = 0x10,
  CallIndirect = 0x11,

  // Parametric operators
  Drop = 0x1a,

  // Variable access
  LocalGet = 0x20,
  LocalSet = 0x21,
  LocalTee = 0x22,

  // Memory-related operators
  I32Load = 0x28,
  I32Store = 0x36,

  // Constants
  I32Const = 0x41,
  F64Const = 0x44,

  // Types
  I32Type = 0x7F, // i32 type
  F64Type = 0x7C, // f64 type

  // Comparison operators
  I32Eqz = 0x45,
  I32Eq = 0x46,
  I32Ne = 0x47,
  I32LtS = 0x48,
  I32LtU = 0x49,

  // Numeric operators
  I32Add = 0x6a,
  I32Sub = 0x6b,
  I32Mul = 0x6c,
  I32And = 0x71,
  I32Or = 0x72,
  I32Xor = 0x73,
  I32Shl = 0x74,
  I32ShrS = 0x75,
  I32ShrU = 0x76,

  RefNull = 0xd0,

  MiscPrefix = 0xfc,

} Op;

typedef struct {
  uint8_t *buffer;
  size_t size;
  size_t capacity;
} WasmModule;

WasmModule *create_wasm_module() {
  WasmModule *module = (WasmModule *)malloc(sizeof(WasmModule));
  module->buffer = (uint8_t *)malloc(1024); // Initial capacity
  module->size = 0;
  module->capacity = 1024;
  return module;
}

void append_byte(WasmModule *module, uint8_t byte) {
  if (module->size >= module->capacity) {
    module->capacity *= 2;
    module->buffer = (uint8_t *)realloc(module->buffer, module->capacity);
  }
  module->buffer[module->size++] = byte;
}

void append_bytes(WasmModule *module, uint8_t *bytes, size_t num) {
  if (module->size >= module->capacity) {
    module->capacity *= 2;
    module->buffer = (uint8_t *)realloc(module->buffer, module->capacity);
  }

  while (num--) {
    module->buffer[module->size++] = *(bytes++);
  }
}

void append_uint32(WasmModule *module, uint32_t value) {
  append_byte(module, value & 0xFF);
  append_byte(module, (value >> 8) & 0xFF);
  append_byte(module, (value >> 16) & 0xFF);
  append_byte(module, (value >> 24) & 0xFF);
}

void generate_wasm_header(WasmModule *module) {
  // Magic number
  append_uint32(module, 0x6D736100);
  // Version
  append_uint32(module, 1);
}

WasmModule *codegen(Ast *ast, WasmModule *module);

typedef WasmModule *(*WasmNumTypeclassMethod)(Type *lt, Type *rt,
                                              WasmModule *module);

WasmModule *codegen_binop(Ast *ast, WasmModule *module) {
  Ast *l = ast->data.AST_BINOP.left;
  Type *lt = l->md;
  WasmModule *resl = codegen(l, module);

  Ast *r = ast->data.AST_BINOP.right;
  Type *rt = r->md;
  WasmModule *resr = codegen(r, module);
  if (!(resl && resr)) {
    return NULL;
  }

  module = resr;

  token_type op = ast->data.AST_BINOP.op;

  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    TypeClass *tc_impl = typeclass_impl(ast->md, &TCNum);

    if ((tc_impl != NULL)) {

      int op_index = op - TOKEN_PLUS;
      WasmNumTypeclassMethod *method_ptr =
          get_typeclass_method(tc_impl, op_index);

      if (method_ptr == NULL) {
        fprintf(stderr, "Invalid operation index for typeclass %s\n",
                tc_impl->name);
        return NULL;
      }

      WasmNumTypeclassMethod method = *method_ptr;

      if (method == NULL) {
        fprintf(stderr, "typeclass %s method not implemented for op %d\n",
                tc_impl->name, op);
        return NULL;
      }

      return method(lt, rt, module);
    }
    return module;
  }

  default:
    return NULL;
  }
}

WasmModule *codegen(Ast *ast, WasmModule *module) {

  // Actual code generation based on AST
  switch (ast->tag) {
  case AST_INT:
    append_byte(module, I32Const);
    append_byte(module, ast->data.AST_INT.value & 0x7F);
    return module;

  case AST_DOUBLE: {
    append_byte(module, F64Const);
    double value = ast->data.AST_DOUBLE.value;
    // Convert the double to a uint64_t
    uint64_t bits;
    memcpy(&bits, &value, sizeof(double));

    // Append the 8 bytes of the double in little-endian order
    for (int i = 0; i < 8; i++) {
      append_byte(module, (bits >> (i * 8)) & 0xFF);
    }
    return module;
  }

  case AST_BINOP:
    // return codegen_binop(ast,  module);
    codegen(ast->data.AST_BINOP.left, module);
    codegen(ast->data.AST_BINOP.right, module);

    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS: {
      if (((Type *)ast->md)->kind == T_INT) {
        append_byte(module, I32Add);
      }
      return module;
    }

    case TOKEN_MINUS: {
      if (((Type *)ast->md)->kind == T_INT) {
        append_byte(module, I32Sub);
      }
      return module;
    }

    case TOKEN_STAR: {
      if (((Type *)ast->md)->kind == T_INT) {
        append_byte(module, I32Mul);
      }
      return module;
    }

    case TOKEN_LT: {
      if (((Type *)ast->md)->kind == T_INT) {
        append_byte(module, I32LtS);
      }
      return module;
    }
    }
    break;

  case AST_BODY: {
    if (ast->data.AST_BODY.len == 1) {
      return codegen(ast->data.AST_BODY.head[0], module);
    }
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.head[i];
      codegen(stmt, module);
    }
    return module;
  }

  case AST_STRING: {

    return module;
  }

  case AST_CHAR: {
    const char ch = ast->data.AST_CHAR.value;

    return module;
  }

  case AST_FMT_STRING: {
    return module;
  }

  case AST_BOOL: {
    return module;
  }

  case AST_TUPLE: {
    return module;
  }

  case AST_LIST: {
    return module;
  }

  case AST_LET: {
    return module;
  }
  case AST_IDENTIFIER: {
    return module;
  }

  case AST_LAMBDA: {
    return module;
  }

  case AST_APPLICATION: {
    return module;
  }

  case AST_EXTERN_FN: {
    return module;
  }
  case AST_MATCH: {
    return module;
  }
  default:
    return NULL;
  }
  return module;
}

// shared type env
static TypeEnv *env = NULL;

size_t module_size(WasmModule *module) { return module->size; }
uint8_t *module_data(WasmModule *module) { return module->buffer; }

void generate_type_section(Type *result_type, WasmModule *module) {
  // Type section
  size_t type_section_start = module->size;
  append_byte(module, 1);    // Section ID
  append_byte(module, 0);    // Placeholder for section size
  append_byte(module, 1);    // Number of types
  append_byte(module, 0x60); // Function type
  append_byte(module, 0);    // 0 parameters
  append_byte(module, 1);    // 1 result
  //
  if (result_type->kind == T_INT) {
    append_byte(module, I32Type); // i32 type
  } else if (result_type->kind == T_NUM) {
    append_byte(module, F64Type); // f64 type
  }
  module->buffer[type_section_start + 1] =
      module->size - type_section_start - 2;
}
void generate_function_section(WasmModule *module) {
  // Function section
  size_t function_section_start = module->size;
  append_byte(module, 3); // Section ID
  append_byte(module, 0); // Placeholder for section size
  append_byte(module, 1); // Number of functions
  append_byte(module, 0); // Type index
  module->buffer[function_section_start + 1] =
      module->size - function_section_start - 2;
}

void generate_export_section(WasmModule *module) {
  // Export section
  size_t export_section_start = module->size;
  append_byte(module, 7); // Section ID for Export
  append_byte(module, 0); // Placeholder for section size
  append_byte(module, 1); // Number of exports

  // Export name "main"
  append_byte(module, 4); // Name length
  append_bytes(module, (uint8_t *)"main", 4);

  append_byte(module, 0); // Export kind: Function
  append_byte(module, 0); // Function index

  // Update export section size
  module->buffer[export_section_start + 1] =
      module->size - export_section_start - 2;
}

void generate_code_section(Ast *prog, WasmModule *module) {
  // Code section
  size_t code_section_start = module->size;
  append_byte(module, 10); // Section ID
  append_byte(module, 0);  // Placeholder for section size
  append_byte(module, 1);  // Number of functions

  size_t function_body_start = module->size;
  append_byte(module, 0); // Placeholder for function body size
  append_byte(module, 0); // Number of locals

  module = codegen(prog, module);
  // Add return instruction
  append_byte(module, Return); // return opcode
  append_byte(module, End);

  // Update function body size
  module->buffer[function_body_start] = module->size - function_body_start - 1;

  // Update code section size
  module->buffer[code_section_start + 1] =
      module->size - code_section_start - 2;
}

void _initialize() { env = initialize_type_env(env); }
void *jit(char *input) {

  if (strcmp("%dump_type_env", input) == 0) {
    print_type_env(env);
    return NULL;
  }

  Ast *prog = parse_input(input);

  Type *typecheck_result = infer_ast(&env, prog);
  if (typecheck_result) {
    printf("`");
    print_type(typecheck_result);
    printf("\n");
  } else {
    fprintf(stderr, "Error: typecheck failed\n");
    return NULL;
  }

  WasmModule *module = create_wasm_module();
  generate_wasm_header(module);
  generate_type_section(typecheck_result, module);
  generate_function_section(module);
  generate_export_section(module);
  generate_code_section(prog, module);
  return module;
}
