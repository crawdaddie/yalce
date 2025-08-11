#ifndef _LANG_ESCAPE_ANALYSIS_H
#define _LANG_ESCAPE_ANALYSIS_H
#include "parse.h"

typedef struct EscapesEnv {
  const char *varname;
  Ast *expr;
  uint32_t id;
  struct EscapesEnv *next;
} EscapesEnv;

typedef struct Allocation {
  uint32_t id;
  const char *name; // Variable that holds this allocation
  Ast *alloc_site;  // Where it was allocated
  bool escapes;     // Does it escape the function?
  bool is_returned; // Is it returned from function?
  bool is_captured; // Is it captured by closure?
  Ast *ast;
  struct Allocation *next;
} Allocation;

// Escape Analysis Context - tracks state during analysis
typedef struct EACtx {
  Allocation *allocations;      // Linked list of allocations in current scope
  int scope_depth;              // Current nesting level (for closures)
  bool in_function;             // Are we inside a function definition?
  const char *current_function; // Name of current function (for debugging)
} EACtx;

typedef enum { EA_STACK_ALLOC, EA_HEAP_ALLOC } EscapeStatus;
typedef struct EscapeMeta {
  EscapeStatus status;
} EscapeMeta;

void escape_analysis(Ast *prog, EACtx *ctx);

#endif
