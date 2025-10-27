#ifndef _LANG_WASM_CODEGEN_H
#define _LANG_WASM_CODEGEN_H
#include "../ht.h"
#include "../parse.h"
#include "../types/type.h"
#include <stddef.h>
#include <stdint.h>

// WASM variable binding - maps identifier to memory slot
typedef struct {
  int slot;   // Memory slot index
  Type *type; // Type of the variable
} WasmBinding;

// Forward declaration
typedef struct WasmContext WasmContext;

// Dynamic buffer for building WASM code
typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} CodeBuffer;

void buffer_init(CodeBuffer *buf);

void buffer_push(CodeBuffer *buf, uint8_t byte);
void buffer_free(CodeBuffer *buf);

// Encode signed integer as LEB128
void encode_i32_leb128(CodeBuffer *buf, int32_t value);

// Encode unsigned integer as LEB128
void encode_u32_leb128(CodeBuffer *buf, uint32_t value);

// Generate WASM code for an AST expression
void codegen(Ast *ast, WasmContext *ctx, CodeBuffer *code);

// Variable binding functions
void bind_variable(WasmContext *ctx, const char *name, int name_len, Type *type);
WasmBinding *lookup_variable(WasmContext *ctx, const char *name, int name_len);
#endif
