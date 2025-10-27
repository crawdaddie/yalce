// Example usage of the simple WASM-compatible parser/analyzer
// This can be compiled to WASM without LLVM dependencies

#include "wasm_simple.h"
#include <stdio.h>

// Example: Parse and analyze a simple expression
void example_simple() {
  SimpleContext *ctx = create_simple_context();

  char *input = "let x = 1 + 2";
  Ast *result = parse_and_analyze(ctx, input, NULL);

  if (result) {
    printf("Successfully parsed and analyzed!\n");
    // The AST is now available in 'result'
    // - result has been type-checked
    // - result has escape analysis metadata attached
  } else {
    printf("Failed to parse or analyze\n");
  }

  free_simple_context(ctx);
}

// Example: Parse multiple inputs with the same context
void example_multiple() {
  SimpleContext *ctx = create_simple_context();

  char *inputs[] = {
    "let square = fn x -> x * x",
    "let result = square 5",
    NULL
  };

  for (int i = 0; inputs[i] != NULL; i++) {
    Ast *ast = parse_and_analyze(ctx, inputs[i], NULL);
    if (ast) {
      printf("Input %d: OK\n", i);
    } else {
      printf("Input %d: FAILED\n", i);
    }
  }

  free_simple_context(ctx);
}

// WASM export: Create a context (call this first)
__attribute__((export_name("wasm_create_context")))
SimpleContext *wasm_create_context() {
  return create_simple_context();
}

// WASM export: Parse and analyze input
__attribute__((export_name("wasm_parse_and_analyze")))
Ast *wasm_parse_and_analyze(SimpleContext *ctx, char *input) {
  return parse_and_analyze(ctx, input, NULL);
}

// WASM export: Free context when done
__attribute__((export_name("wasm_free_context")))
void wasm_free_context(SimpleContext *ctx) {
  free_simple_context(ctx);
}
