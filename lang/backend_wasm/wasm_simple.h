#ifndef _LANG_BACKEND_WASM_SIMPLE_H
#define _LANG_BACKEND_WASM_SIMPLE_H

#include "../parse.h"
#include "../types/inference.h"

// Simple context for WASM-compatible parsing and analysis
// No LLVM dependencies
typedef struct {
  TypeEnv *env;
} SimpleContext;

// Create a new simple context with initialized type environment
SimpleContext *create_simple_context();

// Parse input, run type inference, and escape analysis
// Returns the analyzed AST or NULL on error
// - ctx: Simple context with type environment
// - input: Source code string to parse
// - dirname: Directory name for resolving imports (can be NULL)
Ast *parse_and_analyze(SimpleContext *ctx, char *input, const char *dirname);

// Free the simple context
void free_simple_context(SimpleContext *ctx);

#endif
