#include "../escape_analysis.h"
#include "../ht.h"
#include "../parse.h"
#include "../types/builtins.h"
#include "../types/inference.h"
#include "./codegen_wasm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct WasmContext {
  TypeEnv *env;
  ht bindings;         // Hash table: var name -> WasmBinding*
  int next_value_slot; // Next available memory slot for variables
};

WasmContext *create_wasm_context() {
  WasmContext *ctx = (WasmContext *)malloc(sizeof(WasmContext));
  ctx->env = NULL;
  ctx->next_value_slot = 0;
  ht_init(&ctx->bindings);
  initialize_builtin_types();
  return ctx;
}

void bind_variable(WasmContext *ctx, const char *name, int name_len,
                   Type *type) {
  WasmBinding *binding = (WasmBinding *)malloc(sizeof(WasmBinding));
  binding->slot = ctx->next_value_slot++;
  binding->type = type;

  uint64_t hash = hash_string(name, name_len);
  ht_set_hash(&ctx->bindings, name, hash, binding);
}

WasmBinding *lookup_variable(WasmContext *ctx, const char *name, int name_len) {
  uint64_t hash = hash_string(name, name_len);
  return (WasmBinding *)ht_get_hash(&ctx->bindings, name, hash);
}

// Generate base WASM module with shared table and memory
// This is loaded once and provides the runtime environment
uint8_t *generate_base_module(size_t *out_size) {
  CodeBuffer buffer;
  buffer_init(&buffer);

  // Magic number and version
  uint8_t header[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
  for (size_t i = 0; i < sizeof(header); i++) {
    buffer_push(&buffer, header[i]);
  }

  buffer_push(&buffer, 0x01); // Type section
  buffer_push(&buffer, 0x0b); // Section size (11 bytes)
  buffer_push(&buffer, 0x02); // 2 types

  // Type 0: (i32, i32) -> ()  for store_value
  buffer_push(&buffer, 0x60);
  buffer_push(&buffer, 0x02); // 2 params
  buffer_push(&buffer, 0x7f); // i32
  buffer_push(&buffer, 0x7f); // i32
  buffer_push(&buffer, 0x00); // 0 results

  // Type 1: (i32) -> i32  for load_value
  buffer_push(&buffer, 0x60);
  buffer_push(&buffer, 0x01); // 1 param
  buffer_push(&buffer, 0x7f); // i32
  buffer_push(&buffer, 0x01); // 1 result
  buffer_push(&buffer, 0x7f); // i32

  // Function section
  buffer_push(&buffer, 0x03); // Function section
  buffer_push(&buffer, 0x03); // Section size
  buffer_push(&buffer, 0x02); // 2 functions
  buffer_push(&buffer, 0x00); // function 0: type 0 (store_value)
  buffer_push(&buffer, 0x01); // function 1: type 1 (load_value)

  // Table section - shared function table
  buffer_push(&buffer, 0x04); // Table section
  buffer_push(
      &buffer,
      0x04); // Section size (4 bytes: count=1, funcref, no-max, initial=100)
  buffer_push(&buffer, 0x01);      // 1 table
  buffer_push(&buffer, 0x70);      // funcref
  buffer_push(&buffer, 0x00);      // no maximum
  encode_u32_leb128(&buffer, 100); // initial size: 100 slots (0x64 = 1 byte)

  // Memory section - shared linear memory
  buffer_push(&buffer, 0x05); // Memory section
  buffer_push(&buffer, 0x03); // Section size
  buffer_push(&buffer, 0x01); // 1 memory
  buffer_push(&buffer, 0x00); // no maximum
  buffer_push(&buffer, 0x01); // initial: 1 page (64KB)

  buffer_push(&buffer, 0x07); // Export section
  buffer_push(&buffer, 0x2d); // Section size (45 bytes)

  buffer_push(&buffer, 0x04); // 4 exports

  buffer_push(&buffer, 0x05); // name length
  buffer_push(&buffer, 't');
  buffer_push(&buffer, 'a');
  buffer_push(&buffer, 'b');
  buffer_push(&buffer, 'l');
  buffer_push(&buffer, 'e');
  buffer_push(&buffer, 0x01); // table
  buffer_push(&buffer, 0x00); // index 0

  buffer_push(&buffer, 0x06); // name length
  buffer_push(&buffer, 'm');
  buffer_push(&buffer, 'e');
  buffer_push(&buffer, 'm');
  buffer_push(&buffer, 'o');
  buffer_push(&buffer, 'r');
  buffer_push(&buffer, 'y');
  buffer_push(&buffer, 0x02); // memory
  buffer_push(&buffer, 0x00); // index 0

  buffer_push(&buffer, 0x0b); // name length
  buffer_push(&buffer, 's');
  buffer_push(&buffer, 't');
  buffer_push(&buffer, 'o');
  buffer_push(&buffer, 'r');
  buffer_push(&buffer, 'e');
  buffer_push(&buffer, '_');
  buffer_push(&buffer, 'v');
  buffer_push(&buffer, 'a');
  buffer_push(&buffer, 'l');
  buffer_push(&buffer, 'u');
  buffer_push(&buffer, 'e');
  buffer_push(&buffer, 0x00); // function
  buffer_push(&buffer, 0x00); // index 0

  buffer_push(&buffer, 0x0a); // name length
  buffer_push(&buffer, 'l');
  buffer_push(&buffer, 'o');
  buffer_push(&buffer, 'a');
  buffer_push(&buffer, 'd');
  buffer_push(&buffer, '_');
  buffer_push(&buffer, 'v');
  buffer_push(&buffer, 'a');
  buffer_push(&buffer, 'l');
  buffer_push(&buffer, 'u');
  buffer_push(&buffer, 'e');
  buffer_push(&buffer, 0x00); // function
  buffer_push(&buffer, 0x01); // index 1

  CodeBuffer code_section;
  buffer_init(&code_section);

  buffer_push(&code_section, 0x0a); // Code section ID

  CodeBuffer section_content;
  buffer_init(&section_content);
  buffer_push(&section_content, 0x02); // 2 functions

  // Function 0: store_value(slot, value)
  // Stores value at memory[slot * 4]
  CodeBuffer func0;
  buffer_init(&func0);
  buffer_push(&func0, 0x00); // 0 locals

  // (i32.store (i32.mul (local.get 0) (i32.const 4)) (local.get 1))
  buffer_push(&func0, 0x20); // local.get
  buffer_push(&func0, 0x00); // param 0 (slot)
  buffer_push(&func0, 0x41); // i32.const
  buffer_push(&func0, 0x04); // 4
  buffer_push(&func0, 0x6c); // i32.mul
  buffer_push(&func0, 0x20); // local.get
  buffer_push(&func0, 0x01); // param 1 (value)
  buffer_push(&func0, 0x36); // i32.store
  buffer_push(&func0, 0x02); // alignment
  buffer_push(&func0, 0x00); // offset
  buffer_push(&func0, 0x0b); // end

  encode_u32_leb128(&section_content, func0.size);
  for (size_t i = 0; i < func0.size; i++) {
    buffer_push(&section_content, func0.data[i]);
  }

  // Function 1: load_value(slot) -> i32
  // Loads value from memory[slot * 4]
  CodeBuffer func1;
  buffer_init(&func1);
  buffer_push(&func1, 0x00); // 0 locals

  // (i32.load (i32.mul (local.get 0) (i32.const 4)))
  buffer_push(&func1, 0x20); // local.get
  buffer_push(&func1, 0x00); // param 0 (slot)
  buffer_push(&func1, 0x41); // i32.const
  buffer_push(&func1, 0x04); // 4
  buffer_push(&func1, 0x6c); // i32.mul
  buffer_push(&func1, 0x28); // i32.load
  buffer_push(&func1, 0x02); // alignment
  buffer_push(&func1, 0x00); // offset
  buffer_push(&func1, 0x0b); // end

  encode_u32_leb128(&section_content, func1.size);
  for (size_t i = 0; i < func1.size; i++) {
    buffer_push(&section_content, func1.data[i]);
  }

  // Write code section size
  encode_u32_leb128(&code_section, section_content.size);
  for (size_t i = 0; i < section_content.size; i++) {
    buffer_push(&code_section, section_content.data[i]);
  }

  // Append code section to buffer
  for (size_t i = 0; i < code_section.size; i++) {
    buffer_push(&buffer, code_section.data[i]);
  }

  buffer_free(&func0);
  buffer_free(&func1);
  buffer_free(&section_content);
  buffer_free(&code_section);

  uint8_t *result = (uint8_t *)malloc(buffer.size);
  memcpy(result, buffer.data, buffer.size);
  *out_size = buffer.size;

  buffer_free(&buffer);
  return result;
}

// Generate incremental WASM module from AST
// This module imports table, memory, and helper functions from base module
// Returns pointer to WASM binary, size written to out_size
static uint8_t *generate_executable_module_from_ast(Ast *ast, WasmContext *ctx,
                                                    size_t *out_size) {
  CodeBuffer buffer;
  buffer_init(&buffer);

  uint8_t header[] = {
      0x00, 0x61, 0x73, 0x6d, // Magic number (\0asm)
      0x01, 0x00, 0x00, 0x00  // Version 1
  };
  for (size_t i = 0; i < sizeof(header); i++) {
    buffer_push(&buffer, header[i]);
  }

  buffer_push(&buffer, 0x01); // Type section ID
  CodeBuffer type_section;
  buffer_init(&type_section);

  buffer_push(&type_section, 0x03); // 3 types

  // Type 0: (i32, i32) -> () for store_value
  buffer_push(&type_section, 0x60);
  buffer_push(&type_section, 0x02); // 2 params
  buffer_push(&type_section, 0x7f); // i32
  buffer_push(&type_section, 0x7f); // i32
  buffer_push(&type_section, 0x00); // 0 results

  // Type 1: (i32) -> i32 for load_value
  buffer_push(&type_section, 0x60);
  buffer_push(&type_section, 0x01); // 1 param
  buffer_push(&type_section, 0x7f); // i32
  buffer_push(&type_section, 0x01); // 1 result
  buffer_push(&type_section, 0x7f); // i32

  // Type 2: () -> i32 for eval
  buffer_push(&type_section, 0x60);
  buffer_push(&type_section, 0x00); // 0 params
  buffer_push(&type_section, 0x01); // 1 result
  buffer_push(&type_section, 0x7f); // i32

  encode_u32_leb128(&buffer, type_section.size);
  for (size_t i = 0; i < type_section.size; i++) {
    buffer_push(&buffer, type_section.data[i]);
  }
  buffer_free(&type_section);

  // Import section
  buffer_push(&buffer, 0x02); // Import section ID
  CodeBuffer import_section;
  buffer_init(&import_section);

  buffer_push(&import_section, 0x04); // 4 imports

  // Import table: "env"."table"
  buffer_push(&import_section, 0x03); // module name length
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 'n');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 0x05); // field name length
  buffer_push(&import_section, 't');
  buffer_push(&import_section, 'a');
  buffer_push(&import_section, 'b');
  buffer_push(&import_section, 'l');
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 0x01);      // import kind: table
  buffer_push(&import_section, 0x70);      // funcref
  buffer_push(&import_section, 0x00);      // limits: no max
  encode_u32_leb128(&import_section, 100); // initial: 100

  // Import memory: "env"."memory"
  buffer_push(&import_section, 0x03); // module name length
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 'n');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 0x06); // field name length
  buffer_push(&import_section, 'm');
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 'm');
  buffer_push(&import_section, 'o');
  buffer_push(&import_section, 'r');
  buffer_push(&import_section, 'y');
  buffer_push(&import_section, 0x02);    // import kind: memory
  buffer_push(&import_section, 0x00);    // limits: no max
  encode_u32_leb128(&import_section, 1); // initial: 1 page

  // Import function: "env"."store_value"
  buffer_push(&import_section, 0x03); // module name length
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 'n');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 0x0b); // field name length
  buffer_push(&import_section, 's');
  buffer_push(&import_section, 't');
  buffer_push(&import_section, 'o');
  buffer_push(&import_section, 'r');
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, '_');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 'a');
  buffer_push(&import_section, 'l');
  buffer_push(&import_section, 'u');
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 0x00);    // import kind: func
  encode_u32_leb128(&import_section, 0); // type index 0

  // Import function: "env"."load_value"
  buffer_push(&import_section, 0x03); // module name length
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 'n');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 0x0a); // field name length
  buffer_push(&import_section, 'l');
  buffer_push(&import_section, 'o');
  buffer_push(&import_section, 'a');
  buffer_push(&import_section, 'd');
  buffer_push(&import_section, '_');
  buffer_push(&import_section, 'v');
  buffer_push(&import_section, 'a');
  buffer_push(&import_section, 'l');
  buffer_push(&import_section, 'u');
  buffer_push(&import_section, 'e');
  buffer_push(&import_section, 0x00);    // import kind: func
  encode_u32_leb128(&import_section, 1); // type index 1

  encode_u32_leb128(&buffer, import_section.size);
  for (size_t i = 0; i < import_section.size; i++) {
    buffer_push(&buffer, import_section.data[i]);
  }
  buffer_free(&import_section);

  // Function section - 1 function (eval)
  buffer_push(&buffer, 0x03); // Function section ID
  CodeBuffer func_section;
  buffer_init(&func_section);

  buffer_push(&func_section, 0x01);    // 1 function
  encode_u32_leb128(&func_section, 2); // type index 2

  encode_u32_leb128(&buffer, func_section.size);
  for (size_t i = 0; i < func_section.size; i++) {
    buffer_push(&buffer, func_section.data[i]);
  }
  buffer_free(&func_section);

  buffer_push(&buffer, 0x07); // Export section ID
  CodeBuffer export_section;
  buffer_init(&export_section);

  buffer_push(&export_section, 0x01); // 1 export
  buffer_push(&export_section, 0x04); // name length
  buffer_push(&export_section, 'e');
  buffer_push(&export_section, 'v');
  buffer_push(&export_section, 'a');
  buffer_push(&export_section, 'l');
  buffer_push(&export_section, 0x00);    // export kind: function
  encode_u32_leb128(&export_section, 2); // function index 2 (after 2 imports)

  encode_u32_leb128(&buffer, export_section.size);
  for (size_t i = 0; i < export_section.size; i++) {
    buffer_push(&buffer, export_section.data[i]);
  }
  buffer_free(&export_section);

  // Code section
  buffer_push(&buffer, 0x0a); // Code section ID
  CodeBuffer code_section;
  buffer_init(&code_section);

  buffer_push(&code_section, 0x01); // 1 function

  // Generate function body for eval
  CodeBuffer func_body;
  buffer_init(&func_body);
  buffer_push(&func_body, 0x00); // 0 local declarations

  codegen(ast, ctx, &func_body);

  buffer_push(&func_body, 0x0b);

  encode_u32_leb128(&code_section, func_body.size);
  for (size_t i = 0; i < func_body.size; i++) {
    buffer_push(&code_section, func_body.data[i]);
  }
  buffer_free(&func_body);

  encode_u32_leb128(&buffer, code_section.size);
  for (size_t i = 0; i < code_section.size; i++) {
    buffer_push(&buffer, code_section.data[i]);
  }
  buffer_free(&code_section);

  uint8_t *result = (uint8_t *)malloc(buffer.size);
  if (!result) {
    buffer_free(&buffer);
    *out_size = 0;
    return NULL;
  }

  memcpy(result, buffer.data, buffer.size);
  *out_size = buffer.size;
  buffer_free(&buffer);

  return result;
}

uint8_t *generate_executable_module(WasmContext *ctx, char *input,
                                    size_t *out_size) {
  Ast *prog = parse_input(input, NULL);

  if (!prog) {
    fprintf(stderr, "Parse error\n");
    *out_size = 0;
    return NULL;
  }

  TICtx ti_ctx = {.env = ctx->env, .scope = 0};
  Type *typecheck_result = infer(prog, &ti_ctx);
  if (!typecheck_result) {
    fprintf(stderr, "Type error\n");
    *out_size = 0;
    return NULL;
  }

  escape_analysis(prog);

  return generate_executable_module_from_ast(prog, ctx, out_size);
}
