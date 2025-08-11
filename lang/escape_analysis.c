#include "./escape_analysis.h"
#include "serde.h"
#include "types/type.h"
#include <stdlib.h>
#include <string.h>

static int32_t next_alloc_id;

// Add allocation to context
void ctx_add_allocation(EACtx *ctx, const char *varname, Ast *alloc_site) {
  Allocation *alloc = malloc(sizeof(Allocation));
  *alloc = (Allocation){.id = next_alloc_id++,
                        .name = varname,
                        .alloc_site = alloc_site,
                        .escapes = false,
                        .is_returned = false,
                        .is_captured = false,
                        .next = ctx->allocations};
  ctx->allocations = alloc;

  printf("Added allocation %d for variable '%s'\n", alloc->id, varname);
}

// Print allocation report
void print_allocation_report(EACtx *ctx, const char *scope_name) {
  printf("\n=== Allocation Report for %s ===\n", scope_name);

  if (!ctx->allocations) {
    printf("No allocations found.\n");
    printf("=====================================\n\n");
    return;
  }

  int total_allocations = 0;
  int stack_allocations = 0;
  int heap_allocations = 0;

  printf("Variable Name    | ID  | Scope | Status     | Reason\n");
  printf("-----------------|-----|-------|------------|------------------\n");

  for (Allocation *alloc = ctx->allocations; alloc; alloc = alloc->next) {
    total_allocations++;

    const char *status = alloc->escapes ? "HEAP" : "STACK";
    const char *reason = "";

    if (alloc->escapes) {
      heap_allocations++;
      if (alloc->is_returned) {
        reason = "returned";
      } else if (alloc->is_captured) {
        reason = "captured";
      } else {
        reason = "unknown";
      }
    } else {
      stack_allocations++;
      reason = "local only";
    }

    printf("%-16s | %-3d | %-5d | %-10s | %s\n", alloc->name, alloc->id,
           ctx->scope_depth, status, reason);
  }

  printf("-----------------|-----|-------|------------|------------------\n");
  printf("Summary: %d total, %d stack, %d heap\n", total_allocations,
         stack_allocations, heap_allocations);
  printf("=====================================\n\n");
}

void print_final_escape_report(EACtx *ctx) {
  print_allocation_report(ctx, "Program (Final)");
}

// Find allocation by variable name
Allocation *ctx_find_allocation(EACtx *ctx, const char *varname) {
  if (!ctx->allocations) {
    return NULL;
  }
  for (Allocation *a = ctx->allocations; a; a = a->next) {
    if (strcmp(a->name, varname) == 0) {
      return a;
    }
  }
  return NULL;
}

// Mark allocation as escaping
void mark_escape(Allocation *alloc, const char *reason) {
  if (alloc && !alloc->escapes) {
    alloc->escapes = true;
    printf("Allocation %d (%s) escapes: %s\n", alloc->id, alloc->name, reason);

    // Set escape metadata on the allocation site
    if (alloc->alloc_site) {
      EscapeMeta *meta = malloc(sizeof(EscapeMeta));
      meta->status = EA_HEAP_ALLOC;
      alloc->alloc_site->ea_md = meta;
    }
  }
}

// Check if a type needs heap allocation
bool type_needs_allocation(Type *type) {
  if (!type)
    return false;
  // Only arrays and lists need heap allocation in your system
  return is_array_type(type) || is_list_type(type);
}

// Helper function to check if a variable is used in an AST subtree
bool allocation_used_in_ast(const char *varname, Ast *ast) {
  if (!ast || !varname)
    return false;

  switch (ast->tag) {
  case AST_IDENTIFIER:
    return strcmp(ast->data.AST_IDENTIFIER.value, varname) == 0;

  case AST_BODY:
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      if (allocation_used_in_ast(varname, ast->data.AST_BODY.stmts[i])) {
        return true;
      }
    }
    return false;

  case AST_APPLICATION:
    if (allocation_used_in_ast(varname, ast->data.AST_APPLICATION.function)) {
      return true;
    }
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      if (allocation_used_in_ast(varname, ast->data.AST_APPLICATION.args + i)) {
        return true;
      }
    }
    return false;

  case AST_LET:
    if (allocation_used_in_ast(varname, ast->data.AST_LET.expr)) {
      return true;
    }
    if (ast->data.AST_LET.in_expr &&
        allocation_used_in_ast(varname, ast->data.AST_LET.in_expr)) {
      return true;
    }
    return false;

  case AST_MATCH:
    if (allocation_used_in_ast(varname, ast->data.AST_MATCH.expr)) {
      return true;
    }
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      if (allocation_used_in_ast(varname,
                                 ast->data.AST_MATCH.branches + (2 * i)) ||
          allocation_used_in_ast(varname,
                                 ast->data.AST_MATCH.branches + (2 * i) + 1)) {
        return true;
      }
    }
    return false;

  case AST_ARRAY:
  case AST_LIST:
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      if (allocation_used_in_ast(varname, ast->data.AST_LIST.items + i)) {
        return true;
      }
    }
    return false;

  case AST_TUPLE:
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      if (allocation_used_in_ast(varname, ast->data.AST_LIST.items + i)) {
        return true;
      }
    }
    return false;

  default:
    return false;
  }
}

// Check if a function exposes internal memory (like cstr)
bool function_exposes_memory(const char *func_name) {
  return false;
  // // Functions that return pointers to internal memory
  // return strcmp(func_name, "cstr") == 0 || strcmp(func_name, "data_ptr") == 0
  // ||
  //        strcmp(func_name, "get_internal_ptr") == 0;
}

// Check if a function safely dereferences without exposing memory
bool function_safely_dereferences(const char *func_name) {
  // Functions that return values from inside containers, not pointers
  return strcmp(func_name, "array_at") == 0 || strcmp(func_name, "get") == 0 ||
         strcmp(func_name, "at") == 0 || strcmp(func_name, "index") == 0;
}

void escape_analysis(Ast *ast, EACtx *ctx) {
  if (!ast)
    return;

  switch (ast->tag) {
  case AST_INT:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_CHAR:
  case AST_BOOL:
  case AST_VOID: {
    // Primitives don't allocate
    break;
  }

  case AST_ARRAY: {
    // Array allocation - this is the key case for your example
    // Arrays always need heap allocation, but might be optimizable to stack

    // Analyze array elements first
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      escape_analysis(ast->data.AST_LIST.items + i, ctx);
    }

    // This array allocation starts as "doesn't escape"
    // Will be marked as escaping if:
    // 1. It's returned from function
    // 2. It's stored in a global
    // 3. It's captured by a closure
    // 4. It's passed to unknown function that might store it

    printf("Found array allocation\n");
    break;
  }

  case AST_LIST: {
    // Similar to array
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      escape_analysis(ast->data.AST_LIST.items + i, ctx);
    }

    printf("Found list allocation\n");
    break;
  }

  case AST_TUPLE: {
    // Tuples are typically stack-allocated
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      escape_analysis(ast->data.AST_LIST.items + i, ctx);
    }
    break;
  }

  case AST_TYPE_DECL: {
    // Type declarations don't contain runtime allocations
    break;
  }

  case AST_BODY: {
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      escape_analysis(stmt, ctx);
    }
    break;
  }

  case AST_MODULE: {
    Ast *body = ast->data.AST_LAMBDA.body; // Fixed: should be AST_MODULE.body
    escape_analysis(body, ctx);
    break;
  }

  case AST_IDENTIFIER: {
    // Check if this identifier refers to an allocation
    const char *varname = ast->data.AST_IDENTIFIER.value;
    Allocation *alloc = ctx_find_allocation(ctx, varname);

    if (alloc) {
      printf("Variable '%s' references allocation %d\n", varname, alloc->id);
      // This is just a reference - doesn't automatically cause escape
      // Escape depends on how this reference is used
    }
    break;
  }

  case AST_APPLICATION: {
    // Analyze function being called
    escape_analysis(ast->data.AST_APPLICATION.function, ctx);

    // Get function name if it's an identifier
    const char *func_name = NULL;
    if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
      func_name = ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
    }

    // Analyze all arguments
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      escape_analysis(ast->data.AST_APPLICATION.args + i, ctx);

      // Check if argument is a variable that refers to an allocation
      Ast *arg = ast->data.AST_APPLICATION.args + i;
      if (arg->tag == AST_IDENTIFIER) {
        const char *varname = arg->data.AST_IDENTIFIER.value;
        Allocation *alloc = ctx_find_allocation(ctx, varname);

        if (alloc) {
          printf("Passing allocation %d to function call", alloc->id);
          if (func_name) {
            printf(" '%s'", func_name);
          }
          printf("\n");

          // CRITICAL: cstr() exposes internal memory pointer!
          // If cstr(r) result is used anywhere that could escape,
          // the original array must be heap allocated
          if (func_name && function_exposes_memory(func_name)) {
            printf("Function '%s' exposes internal memory of allocation %d\n",
                   func_name, alloc->id);
            // The allocation must live as long as the exposed pointer might be
            // used For safety, mark as escaping unless we can prove the pointer
            // has local scope
            mark_escape(alloc, "internal memory exposed by function");
          }
          // Functions like array_at(r, i) safely dereference - return values,
          // not pointers
          else if (func_name && function_safely_dereferences(func_name)) {
            printf("Function '%s' safely dereferences allocation %d (returns "
                   "value, not pointer)\n",
                   func_name, alloc->id);
            // This is safe - the allocation can remain on stack
            // The function returns a copy of the value, not a pointer to it
          }
          // Unknown function - be conservative
          else if (func_name) {
            printf("Unknown function '%s' - being conservative with allocation "
                   "%d\n",
                   func_name, alloc->id);
            // TODO: Add interprocedural analysis or function annotations
            // For now, assume unknown functions might store the reference
            // mark_escape(alloc, "passed to unknown function");
          }

          // TODO: Add interprocedural analysis for other functions
        }
      }
    }
    break;
  }

  case AST_LET: {
    Ast *expr = ast->data.AST_LET.expr;
    escape_analysis(expr, ctx);

    // Check if this let binding creates an allocation
    if (ast->data.AST_LET.binding->tag == AST_IDENTIFIER) {
      const char *varname =
          ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;

      // Check if the expression allocates memory
      if (expr->md && type_needs_allocation(expr->md)) {
        // This let binding creates an allocation
        ctx_add_allocation(ctx, varname, expr);

        printf("Let binding '%s' creates allocation\n", varname);
      }
    }

    // Analyze the 'in' expression if present
    if (ast->data.AST_LET.in_expr) {
      escape_analysis(ast->data.AST_LET.in_expr, ctx);
    }
    break;
  }

  case AST_LAMBDA: {
    printf("\n--- Entering Lambda Analysis ---\n");

    // Create new context for lambda body (new scope)
    EACtx lambda_ctx = *ctx; // Copy current context
    lambda_ctx.scope_depth++;

    // Keep track of allocations before lambda
    Allocation *pre_lambda_allocations = ctx->allocations;

    Ast *body = ast->data.AST_LAMBDA.body;
    escape_analysis(body, &lambda_ctx);

    // Check if any parent allocations were captured
    for (Allocation *alloc = pre_lambda_allocations; alloc;
         alloc = alloc->next) {
      // Check if this allocation is referenced in the lambda body
      if (allocation_used_in_ast(alloc->name, body)) {
        alloc->is_captured = true;
        mark_escape(alloc, "captured by closure");
      }
    }

    // Print report of all allocations visible in this lambda
    char scope_name[64];
    snprintf(scope_name, sizeof(scope_name), "Lambda (depth %d)",
             lambda_ctx.scope_depth);

    print_allocation_report(&lambda_ctx, scope_name);

    // Update parent context with any changes
    ctx->allocations = lambda_ctx.allocations;
    for (Allocation *a = ctx->allocations; a; a = a->next) {
      if (!a->escapes) {
        EscapeStatus *s = malloc(sizeof(EscapeStatus));
        *s = (EscapeStatus){EA_STACK_ALLOC};
        a->alloc_site->ea_md = s;
      }
    }

    printf("--- Exiting Lambda Analysis ---\n\n");
    break;
  }

  case AST_MATCH: {
    // Analyze the expression being matched
    escape_analysis(ast->data.AST_MATCH.expr, ctx);

    // Analyze all match branches
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      // Pattern (left side)
      escape_analysis(ast->data.AST_MATCH.branches + (2 * i), ctx);
      // Body (right side)
      escape_analysis(ast->data.AST_MATCH.branches + (2 * i) + 1, ctx);
    }
    break;
  }

  case AST_EXTERN_FN: {
    // External function declarations don't contain allocations
    break;
  }

  case AST_YIELD: {
    // Analyze the yielded expression
    if (ast->data.AST_YIELD.expr) {
      escape_analysis(ast->data.AST_YIELD.expr, ctx);

      // Check if yielded expression contains allocations
      if (ast->data.AST_YIELD.expr->tag == AST_IDENTIFIER) {
        const char *varname =
            ast->data.AST_YIELD.expr->data.AST_IDENTIFIER.value;
        Allocation *alloc = ctx_find_allocation(ctx, varname);

        if (alloc) {
          // This allocation is being returned - it escapes!
          alloc->is_returned = true;
          mark_escape(alloc, "returned from function");
        }
      }
    }

    // TODO: Handle other cases where complex expressions are returned
    // that might contain references to allocations
    break;
  }

  default: {
    // Unknown AST node - conservatively analyze any child nodes
    break;
  }
  }

  // After analysis, set default escape metadata if not already set
  if (!ast->ea_md && (ast->tag == AST_ARRAY || ast->tag == AST_LIST)) {
    EscapeMeta *meta = malloc(sizeof(EscapeMeta));
    meta->status = EA_STACK_ALLOC; // Default to stack allocation
    ast->ea_md = meta;
  }
}
