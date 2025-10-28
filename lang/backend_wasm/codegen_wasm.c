#include "./codegen_wasm.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>

void buffer_init(CodeBuffer *buf) {
  buf->capacity = 256;
  buf->size = 0;
  buf->data = (uint8_t *)malloc(buf->capacity);
}

void buffer_push(CodeBuffer *buf, uint8_t byte) {
  if (buf->size >= buf->capacity) {
    buf->capacity *= 2;
    buf->data = (uint8_t *)realloc(buf->data, buf->capacity);
  }
  buf->data[buf->size++] = byte;
}

void buffer_free(CodeBuffer *buf) { free(buf->data); }

// Encode signed integer as LEB128
void encode_i32_leb128(CodeBuffer *buf, int32_t value) {
  int more = 1;
  while (more) {
    uint8_t byte = value & 0x7f;
    value >>= 7;
    // Sign extend
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    buffer_push(buf, byte);
  }
}

// Encode unsigned integer as LEB128
void encode_u32_leb128(CodeBuffer *buf, uint32_t value) {
  do {
    uint8_t byte = value & 0x7f;
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    buffer_push(buf, byte);
  } while (value != 0);
}

// Generate WASM code for an AST expression
void codegen(Ast *ast, WasmContext *ctx, CodeBuffer *code) {
  if (!ast)
    return;

  switch (ast->tag) {
  case AST_INT: {
    // i32.const <value>
    buffer_push(code, 0x41); // i32.const opcode
    encode_i32_leb128(code, ast->data.AST_INT.value);
    break;
  }

  case AST_IDENTIFIER: {
    const char *name = ast->data.AST_IDENTIFIER.value;
    int name_len = ast->data.AST_IDENTIFIER.length;
    WasmBinding *binding = lookup_variable(ctx, name, name_len);

    if (binding) {
      // Generate: call load_value (passing slot as i32.const)
      buffer_push(code, 0x41); // i32.const
      encode_i32_leb128(code, binding->slot);
      buffer_push(code, 0x10);    // call
      encode_u32_leb128(code, 1); // function index 1 (load_value)
    } else {
      fprintf(stderr, "Undefined variable: %.*s\n", name_len, name);
      // Fallback: push 0
      buffer_push(code, 0x41); // i32.const
      buffer_push(code, 0x00); // 0
    }
    break;
  }

  case AST_LET: {
    // let binding = expr
    Ast *binding = ast->data.AST_LET.binding;
    Ast *expr = ast->data.AST_LET.expr;
    Ast *in_expr = ast->data.AST_LET.in_expr;

    if (binding->tag == AST_IDENTIFIER) {
      const char *name = binding->data.AST_IDENTIFIER.value;
      int name_len = binding->data.AST_IDENTIFIER.length;

      // Allocate slot for this variable
      bind_variable(ctx, name, name_len, ast->type);
      WasmBinding *var_binding = lookup_variable(ctx, name, name_len);

      // Store to memory: call store_value(slot, value)
      // store_value signature: (i32 slot, i32 value) -> ()
      // Push arguments in order: slot, then value

      // Push slot first
      buffer_push(code, 0x41); // i32.const (slot)
      encode_i32_leb128(code, var_binding->slot);

      codegen(expr, ctx, code);

      // Stack is now: [slot, value]
      buffer_push(code, 0x10);    // call
      encode_u32_leb128(code, 0); // function index 0 (store_value)
    }

    // If there's an in_expr, generate code for it
    if (in_expr) {
      codegen(in_expr, ctx, code);
    } else {
      // Return unit/void value (0 for now)
      buffer_push(code, 0x41); // i32.const
      buffer_push(code, 0x00); // 0
    }
    break;
  }

  case AST_BODY: {
    // Execute ALL statements in body
    // Last one is the return value
    if (ast->data.AST_BODY.stmts) {
      AstList *current = ast->data.AST_BODY.stmts;
      AstList *last = NULL;

      // Find last statement
      AstList *temp = current;
      while (temp) {
        last = temp;
        temp = temp->next;
      }

      while (current) {
        codegen(current->ast, ctx, code);

        // Drop result if not the last statement
        if (current != last) {
          buffer_push(code, 0x1a); // drop instruction
        }

        current = current->next;
      }
    }
    break;
  }

  case AST_APPLICATION: {
    // Function application: (f arg1 arg2 ...)
    // For now, handle built-in operators
    Ast *func = ast->data.AST_APPLICATION.function;

    if (func->tag == AST_IDENTIFIER) {
      const char *name = func->data.AST_IDENTIFIER.value;
      int arg_count = ast->data.AST_APPLICATION.len;

      // Binary operators
      if (arg_count == 2) {
        // Generate both arguments
        codegen(&ast->data.AST_APPLICATION.args[0], ctx, code);
        codegen(&ast->data.AST_APPLICATION.args[1], ctx, code);

        // Generate operation
        if (CHARS_EQ(name, "+")) {
          buffer_push(code, 0x6a); // i32.add
        } else if (strcmp(name, "-") == 0) {
          buffer_push(code, 0x6b); // i32.sub
        } else if (strcmp(name, "*") == 0) {
          buffer_push(code, 0x6c); // i32.mul
        } else if (strcmp(name, "/") == 0) {
          buffer_push(code, 0x6d); // i32.div_s
        } else {
          fprintf(stderr, "Unsupported operator: %s\n", name);
          // Fallback: drop both args, push 0
          buffer_push(code, 0x1a); // drop arg2
          buffer_push(code, 0x1a); // drop arg1
          buffer_push(code, 0x41); // i32.const
          buffer_push(code, 0x00); // 0
        }
      } else {
        fprintf(stderr, "Unsupported arity for operator %s: %d\n", name,
                arg_count);
        buffer_push(code, 0x41); // i32.const
        buffer_push(code, 0x00); // 0
      }
    } else {
      fprintf(stderr, "Unsupported function type in application\n");
      buffer_push(code, 0x41); // i32.const
      buffer_push(code, 0x00); // 0
    }
    break;
  }

  default:
    fprintf(stderr, "Unsupported AST node type: %d\n", ast->tag);
    // Generate i32.const 0 as fallback
    buffer_push(code, 0x41);
    buffer_push(code, 0x00);
    break;
  }
}
