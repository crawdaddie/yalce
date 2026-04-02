#ifndef _LANG_ESCAPE_ANALYSIS_H
#define _LANG_ESCAPE_ANALYSIS_H
#include "parse.h"

// Simple allocation tracking
typedef struct Allocation {
  uint32_t id;
  const char *varname; // Variable that holds this allocation
  Ast *alloc_site;     // Where it was allocated
  bool escapes;        // Does it escape the function?
  bool is_returned;    // Is it returned from function?
  bool is_captured;    // Is it captured by closure?
  bool is_mutable;     // Is it mutated (e.g. via array_set)?
  int scope;
  struct Allocation *next;
} Allocation;

// Escape Analysis Context - tracks state during analysis
typedef struct {
  Allocation *allocations; // Linked list of allocations in current scope
  int scope;               // Current nesting level (for closures)
  bool in_function;        // Are we inside a function definition?
  bool is_return_stmt;
} EACtx;

typedef enum { EA_STACK_ALLOC, EA_HEAP_ALLOC } EscapeStatus;

typedef uint32_t EscapeAttributes;

typedef enum {
  EA_ATTR_MUTABLE = 0x1u,
} EscapeAttribute;

typedef struct EscapeMeta {
  EscapeStatus status;
  uint32_t id;
  EscapeAttributes attributes;
} EscapeMeta;

void escape_analysis(Ast *prog);

#endif
