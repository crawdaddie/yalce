#include "./infer_binding.h"
#include "./common.h"
#include "./type.h"
#include "./unification.h"
#include "parse.h"
#include <string.h>

void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...);

Type *bind_pattern_recursive(Ast *pattern, Type *pattern_type, TypeEnv **env,
                             TICtx *ctx);

Type *apply_substitution(Subst *subst, Type *t);

// Implementation of infer_list_binding using recursive pattern binding
Type *infer_list_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx) {
  printf("=== LIST PREPEND DESTRUCTURING ===\n");

  // Step 1: Infer the type of the value expression
  Type *val_type = infer(val, ctx);
  if (!val_type) {
    return type_error(ctx, val, "Cannot infer type of list value");
  }

  printf("Value type: ");
  print_type(val_type);
  printf("\n");

  // Step 2: Use recursive pattern binding to handle the complex pattern
  TypeEnv *new_env = ctx->env;
  Type *pattern_result =
      bind_pattern_recursive(binding, val_type, &new_env, ctx);
  if (!pattern_result) {
    return type_error(ctx, binding, "List pattern binding failed");
  }

  printf("Pattern binding successful\n");

  // Step 3: Infer body in new environment
  if (!body) {
    return &t_void;
  }

  TICtx body_ctx = {
      .env = new_env, .subst = ctx->subst, .err_stream = ctx->err_stream};

  Type *body_type = infer(body, &body_ctx);
  if (!body_type) {
    return type_error(ctx, body,
                      "Cannot infer body type in list destructuring");
  }

  // Step 4: Update context and return
  ctx->subst = body_ctx.subst;

  printf("=== LIST DESTRUCTURING COMPLETE ===\n");
  printf("Body type: ");
  print_type(body_type);
  printf("\n");

  return body_type;
}
bool is_list_prepend_ast(Ast *ast) {
  return (ast->tag == AST_APPLICATION) &&
         (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
          CHARS_EQ(
              ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
              "::"));
}

// Recursively bind patterns (handles nested destructuring)
Type *bind_pattern_recursive(Ast *pattern, Type *pattern_type, TypeEnv **env,
                             TICtx *ctx) {
  printf("=== BINDING PATTERN RECURSIVE ===\n");
  printf("Pattern AST tag: %d\n", pattern->tag);
  printf("Pattern type: ");
  print_type(pattern_type);
  printf("\n");

  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    // Simple variable binding: x
    const char *var_name = pattern->data.AST_IDENTIFIER.value;

    if (var_name[0] == '_') {
      printf("Wildcard pattern - no binding\n");
      return pattern_type;
    }

    printf("Binding variable %s to type: ", var_name);
    print_type(pattern_type);
    printf("\n");

    Scheme var_scheme = {.vars = NULL, .type = pattern_type};
    *env = env_extend(*env, var_name, var_scheme.vars, var_scheme.type);

    return pattern_type;
  }

  case AST_TUPLE: {
    // Tuple destructuring: (x, y)
    int num_elements = pattern->data.AST_LIST.len;
    printf("Tuple pattern with %d elements\n", num_elements);

    // Create expected tuple type with fresh variables
    Type **expected_types = talloc(sizeof(Type *) * num_elements);
    for (int i = 0; i < num_elements; i++) {
      expected_types[i] = next_tvar();
    }
    Type *expected_tuple = create_tuple_type(num_elements, expected_types);

    // Unify with actual pattern type
    UnifyResult ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
    if (unify(pattern_type, expected_tuple, &ur)) {
      printf("ERROR: Tuple pattern type mismatch\n");
      return NULL;
    }

    // Solve constraints
    Subst *solution = NULL;
    if (ur.constraints) {
      solution = solve_constraints(ur.constraints);
      if (!solution) {
        printf("ERROR: Cannot solve tuple pattern constraints\n");
        return NULL;
      }
    }

    // Apply substitutions
    Subst *combined_subst = compose_subst(solution, ur.subst);
    ctx->subst = compose_subst(combined_subst, ctx->subst);

    // Recursively bind each element
    for (int i = 0; i < num_elements; i++) {
      Ast *element_pattern = &pattern->data.AST_LIST.items[i];
      Type *element_type = apply_substitution(ctx->subst, expected_types[i]);

      printf("Processing tuple element %d:\n", i);
      Type *result =
          bind_pattern_recursive(element_pattern, element_type, env, ctx);
      if (!result) {
        return NULL;
      }
    }

    return apply_substitution(ctx->subst, pattern_type);
  }

  case AST_APPLICATION: {
    if (is_list_prepend_ast(pattern)) {
      // List prepend pattern: head::tail
      printf("List prepend pattern\n");

      Ast *head_pattern = &pattern->data.AST_APPLICATION.args[0];
      Ast *tail_pattern = &pattern->data.AST_APPLICATION.args[1];

      // Create expected list type: [element_type]
      Type *element_type = next_tvar();
      Type *list_type = create_list_type_of_type(element_type);

      printf("Expected list type: ");
      print_type(list_type);
      printf("\n");

      // Unify with actual pattern type
      UnifyResult ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
      if (unify(pattern_type, list_type, &ur)) {
        printf("ERROR: List pattern type mismatch\n");
        return NULL;
      }

      // Solve constraints
      Subst *solution = NULL;
      if (ur.constraints) {
        solution = solve_constraints(ur.constraints);
        if (!solution) {
          printf("ERROR: Cannot solve list pattern constraints\n");
          return NULL;
        }
      }

      // Apply substitutions
      Subst *combined_subst = compose_subst(solution, ur.subst);
      ctx->subst = compose_subst(combined_subst, ctx->subst);

      // Get concrete element and list types
      Type *concrete_element_type =
          apply_substitution(ctx->subst, element_type);
      Type *concrete_list_type = apply_substitution(ctx->subst, list_type);

      printf("Concrete element type: ");
      print_type(concrete_element_type);
      printf("\n");

      // Recursively bind head to element type
      printf("Binding head pattern:\n");
      Type *head_result =
          bind_pattern_recursive(head_pattern, concrete_element_type, env, ctx);
      if (!head_result) {
        return NULL;
      }

      // Recursively bind tail to list type
      printf("Binding tail pattern:\n");
      Type *tail_result =
          bind_pattern_recursive(tail_pattern, concrete_list_type, env, ctx);
      if (!tail_result) {
        return NULL;
      }

      return concrete_list_type;
    } else {
      printf("ERROR: Unsupported application pattern\n");
      return NULL;
    }
  }

  default:
    printf("ERROR: Unsupported pattern type: %d\n", pattern->tag);
    return NULL;
  }
}
Type *infer_pattern_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx) {
  printf("=== PATTERN BINDING (unified approach) ===\n");

  // Step 1: Infer value type (same as infer_let_simple)
  Type *vtype = infer(val, ctx);
  if (!vtype) {
    return type_error(ctx, val, "Cannot infer value type");
  }

  printf("Value type: ");
  print_type(vtype);
  printf("\n");

  // Step 2: Apply current substitutions (same as infer_let_simple)
  Subst *s1 = ctx->subst;
  TypeEnv *env_subst = apply_subst_env(s1, ctx->env);
  Type *vtype_subst = apply_substitution(s1, vtype);

  printf("Value type after substitution: ");
  print_type(vtype_subst);
  printf("\n");

  // Step 3: Handle different binding patterns
  TypeEnv *new_env = env_subst;

  if (binding->tag == AST_IDENTIFIER) {
    // Simple binding: let x = val
    printf("Simple identifier binding\n");

    Scheme gen_type_scheme = generalize(vtype_subst, env_subst);
    new_env = env_extend(env_subst, binding->data.AST_IDENTIFIER.value,
                         gen_type_scheme.vars, gen_type_scheme.type);
  } else {
    // Complex pattern binding: let (x, y) = val or let x::xs = val
    printf("Complex pattern binding\n");

    Type *pattern_result =
        bind_pattern_recursive(binding, vtype_subst, &new_env, ctx);
    if (!pattern_result) {
      return type_error(ctx, binding, "Pattern binding failed");
    }

    // Note: We don't generalize in pattern bindings typically,
    // because each variable gets its concrete type from the pattern
  }

  // Step 4: Handle body (same as infer_let_simple)
  if (body) {
    TICtx body_ctx = {.env = new_env, .subst = ctx->subst};
    Type *body_type = infer(body, &body_ctx);
    if (!body_type) {
      return type_error(ctx, body, "Cannot infer body type");
    }

    Subst *s2 = body_ctx.subst;
    Subst *final_subst = compose_subst(s2, ctx->subst);
    ctx->subst = final_subst;
    return body_type;
  }

  // Step 5: No body case (same as infer_let_simple)
  return vtype_subst;
}
