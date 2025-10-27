#include "escape_analysis.h"
#include "parse.h"
#include "serde.h"
#include "types/builtins.h"
#include "types/inference.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple WASM-compatible frontend: parse, typecheck, escape analysis only
// No LLVM dependencies - just returns the analyzed AST
typedef struct {
  TypeEnv *env;
} SimpleContext;

// Print AST directly to stdout without buffering (WASM-friendly)
void print_ast_wasm(Ast *ast) {

  if (!ast) {
    printf("null");
    return;
  }

  switch (ast->tag) {
  case AST_BODY:
    for (AstList *current = ast->data.AST_BODY.stmts; current != NULL;
         current = current->next) {
      print_ast_wasm(current->ast);
      if (current->next != NULL) {
        printf("\n");
      }
    }
    break;

  case AST_LET:
    printf("(let ");
    print_ast_wasm(ast->data.AST_LET.binding);
    printf(" ");
    print_ast_wasm(ast->data.AST_LET.expr);
    printf(")");
    if (ast->data.AST_LET.in_expr) {
      printf(" : ");
      print_ast_wasm(ast->data.AST_LET.in_expr);
    }
    break;

  case AST_INT:
    printf("%d", ast->data.AST_INT.value);
    break;

  case AST_FLOAT:
    printf("%f", ast->data.AST_FLOAT.value);
    break;

  case AST_DOUBLE:
    printf("%f", ast->data.AST_DOUBLE.value);
    break;

  case AST_STRING:
    printf("\"");
    printf("%.*s", (int)ast->data.AST_STRING.length,
           ast->data.AST_STRING.value);
    printf("\"");
    break;

  case AST_CHAR:
    printf("'%c'", ast->data.AST_CHAR.value);
    break;

  case AST_BOOL:
    printf("%s", ast->data.AST_BOOL.value ? "true" : "false");
    break;

  case AST_VOID:
    printf("()");
    break;

  case AST_IDENTIFIER:
    printf("%s", ast->data.AST_IDENTIFIER.value);
    break;

  case AST_PLACEHOLDER_ID:
    printf("_");
    break;

  case AST_APPLICATION:
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      printf("(");
    }
    print_ast_wasm(ast->data.AST_APPLICATION.function);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      printf(" ");
      print_ast_wasm(ast->data.AST_APPLICATION.args + i);
      printf(")");
    }
    break;

  case AST_BINOP:
    printf("(");
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS:
      printf("+");
      break;
    case TOKEN_MINUS:
      printf("-");
      break;
    case TOKEN_STAR:
      printf("*");
      break;
    case TOKEN_SLASH:
      printf("/");
      break;
    case TOKEN_LT:
      printf("<");
      break;
    case TOKEN_GT:
      printf(">");
      break;
    case TOKEN_LTE:
      printf("<=");
      break;
    case TOKEN_GTE:
      printf(">=");
      break;
    case TOKEN_EQUALITY:
      printf("==");
      break;
    case TOKEN_NOT_EQUAL:
      printf("!=");
      break;
    default:
      printf("?");
      break;
    }
    printf(" ");
    print_ast_wasm(ast->data.AST_BINOP.left);
    printf(" ");
    print_ast_wasm(ast->data.AST_BINOP.right);
    printf(")");
    break;

  case AST_LAMBDA:
    printf("(fn ");
    print_ast_wasm(ast->data.AST_LAMBDA.params);
    printf(" -> ");
    print_ast_wasm(ast->data.AST_LAMBDA.body);
    printf(")");
    break;

  case AST_MATCH:
    printf("(match ");
    print_ast_wasm(ast->data.AST_MATCH.expr);
    printf(" { ... })");
    break;

  default:
    printf("(? tag=%d)", ast->tag);
    break;
  }
}

SimpleContext *create_simple_context() {
  SimpleContext *ctx = (SimpleContext *)malloc(sizeof(SimpleContext));
  ctx->env = NULL;
  initialize_builtin_types();
  return ctx;
}

Ast *parse_and_analyze(SimpleContext *ctx, char *input, const char *dirname) {
  // Parse the input
  //
  Ast *prog = parse_input(input, dirname);
  if (!prog) {
    fprintf(stderr, "Parse error\n");
    return NULL;
  }

  // Type inference
  TICtx ti_ctx = {.env = ctx->env, .scope = 0};
  Type *typecheck_result = infer(prog, &ti_ctx);
  if (!typecheck_result) {
    fprintf(stderr, "Type error\n");
    return NULL;
  }

  // Escape analysis
  escape_analysis(prog);

  // Print AST directly to stdout
  // printf("AST: ");
  // print_ast_wasm(prog);
  // printf("\n");

  return prog;
}

void free_simple_context(SimpleContext *ctx) {
  // Note: TypeEnv cleanup would go here if needed
  free(ctx);
}

// Dynamic buffer for building WASM code
typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} CodeBuffer;

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
void codegen_expr(Ast *ast, CodeBuffer *code) {
  if (!ast)
    return;

  switch (ast->tag) {
  case AST_INT: {
    // i32.const <value>
    buffer_push(code, 0x41); // i32.const opcode
    encode_i32_leb128(code, ast->data.AST_INT.value);
    break;
  }

  case AST_BINOP: {
    // Generate left operand
    codegen_expr(ast->data.AST_BINOP.left, code);

    // Generate right operand
    codegen_expr(ast->data.AST_BINOP.right, code);

    // Generate operation
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS:
      buffer_push(code, 0x6a); // i32.add
      break;
    case TOKEN_MINUS:
      buffer_push(code, 0x6b); // i32.sub
      break;
    case TOKEN_STAR:
      buffer_push(code, 0x6c); // i32.mul
      break;
    case TOKEN_SLASH:
      buffer_push(code, 0x6d); // i32.div_s
      break;
    default:
      fprintf(stderr, "Unsupported binary operation: %d\n",
              ast->data.AST_BINOP.op);
      break;
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
        codegen_expr(current->ast, code);

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
        codegen_expr(&ast->data.AST_APPLICATION.args[0], code);
        codegen_expr(&ast->data.AST_APPLICATION.args[1], code);

        // Generate operation
        if (strcmp(name, "+") == 0) {
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

// Generate executable WASM module from AST
// Returns pointer to WASM binary, size written to out_size
uint8_t *generate_executable_module(Ast *ast, size_t *out_size) {
  // Generate function body code
  CodeBuffer function_code;
  buffer_init(&function_code);

  // Generate code for the expression
  codegen_expr(ast, &function_code);

  // Add 'end' instruction
  buffer_push(&function_code, 0x0b);

  // Build code section
  CodeBuffer code_section;
  buffer_init(&code_section);

  buffer_push(&code_section, 0x0a); // Code section ID

  // Calculate section size
  CodeBuffer section_content;
  buffer_init(&section_content);

  buffer_push(&section_content, 0x01); // 1 function

  // Function body size
  CodeBuffer func_body;
  buffer_init(&func_body);
  buffer_push(&func_body, 0x00); // 0 local declarations

  // Copy function code
  for (size_t i = 0; i < function_code.size; i++) {
    buffer_push(&func_body, function_code.data[i]);
  }

  // Write function body size
  encode_u32_leb128(&section_content, func_body.size);

  // Copy function body
  for (size_t i = 0; i < func_body.size; i++) {
    buffer_push(&section_content, func_body.data[i]);
  }

  // Write code section size
  encode_u32_leb128(&code_section, section_content.size);

  // Copy section content
  for (size_t i = 0; i < section_content.size; i++) {
    buffer_push(&code_section, section_content.data[i]);
  }

  // Fixed sections
  uint8_t header[] = {
      0x00, 0x61, 0x73, 0x6d, // Magic number (\0asm)
      0x01, 0x00, 0x00, 0x00  // Version 1
  };

  uint8_t type_section[] = {
      0x01,      // Type section ID
      0x05,      // Section size
      0x01,      // 1 type
      0x60,      // func type
      0x00,      // 0 parameters
      0x01, 0x7f // 1 result: i32
  };

  uint8_t func_section[] = {
      0x03, // Function section ID
      0x02, // Section size
      0x01, // 1 function
      0x00  // function 0, type 0
  };

  uint8_t export_section[] = {
      0x07,                                         // Export section ID
      0x0d,                                         // Section size
      0x01,                                         // 1 export
      0x09,                                         // name length
      'r',  'e', 'p', 'l', '_', 'e', 'v', 'a', 'l', // "repl_eval"
      0x00,                                         // export kind: function
      0x00                                          // function index 0
  };

  // Calculate total size
  size_t total_size = sizeof(header) + sizeof(type_section) +
                      sizeof(func_section) + sizeof(export_section) +
                      code_section.size;

  // Allocate final buffer
  uint8_t *buffer = (uint8_t *)malloc(total_size);
  if (!buffer) {
    buffer_free(&function_code);
    buffer_free(&code_section);
    buffer_free(&section_content);
    buffer_free(&func_body);
    *out_size = 0;
    return NULL;
  }

  // Copy all sections
  size_t offset = 0;
  memcpy(buffer + offset, header, sizeof(header));
  offset += sizeof(header);

  memcpy(buffer + offset, type_section, sizeof(type_section));
  offset += sizeof(type_section);

  memcpy(buffer + offset, func_section, sizeof(func_section));
  offset += sizeof(func_section);

  memcpy(buffer + offset, export_section, sizeof(export_section));
  offset += sizeof(export_section);

  memcpy(buffer + offset, code_section.data, code_section.size);
  offset += code_section.size;

  // Cleanup
  buffer_free(&function_code);
  buffer_free(&code_section);
  buffer_free(&section_content);
  buffer_free(&func_body);

  *out_size = total_size;
  return buffer;
}
