#include "./infer_binding.h"
#include "./common.h"
#include "./type.h"
#include "./unification.h"
#include "parse.h"
#include "serde.h"
#include <string.h>

// Implementation of infer_list_binding using recursive pattern binding
Type *infer_list_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx) {

  // Step 1: Infer the type of the value expression
  Type *val_type = infer(val, ctx);
  if (!val_type) {
    return type_error(ctx, val, "Cannot infer type of list value");
  }

  // Step 2: Use recursive pattern binding to handle the complex pattern
  TypeEnv *new_env = ctx->env;
  Type *pattern_result =
      bind_pattern_recursive(binding, val_type, &new_env, ctx);
  if (!pattern_result) {
    return type_error(ctx, binding, "List pattern binding failed");
  }

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

  ctx->subst = body_ctx.subst;

  return body_type;
}
bool is_list_cons_operator(Ast *ast) {
  return (ast->tag == AST_APPLICATION) &&
         (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
          CHARS_EQ(
              ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
              "::"));
}

Type *find_variant_member_idx(Type *v, const char *name, int *idx) {
  for (int i = 0; i < v->data.T_CONS.num_args; i++) {
    if (CHARS_EQ(v->data.T_CONS.args[i]->data.T_CONS.name, name)) {
      *idx = i;
      return v->data.T_CONS.args[i];
    }
  }
  return NULL;
}

// Recursively bind patterns (handles nested destructuring)
Type *bind_pattern_recursive(Ast *pattern, Type *pattern_type, TypeEnv **env,
                             TICtx *ctx) {
  if (ast_is_placeholder_id(pattern)) {
    return pattern_type;
  }
  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    const char *var_name = pattern->data.AST_IDENTIFIER.value;

    Scheme *ex_scheme = lookup_scheme(*env, var_name);
    if (ex_scheme) {
      return pattern_type;
    }
    Scheme var_scheme = {.vars = NULL, .type = pattern_type};
    *env = env_extend(*env, var_name, var_scheme.vars, var_scheme.type);

    return pattern_type;
  }

  case AST_TUPLE: {
    int num_elements = pattern->data.AST_LIST.len;

    // Create expected tuple type with fresh variables
    Type **expected_types = talloc(sizeof(Type *) * num_elements);

    bool generic_tuple = false;
    for (int i = 0; i < num_elements; i++) {
      Type *inferred = infer(pattern->data.AST_LIST.items + i, ctx);
      if (is_generic(inferred)) {
        generic_tuple = true;
      }
      expected_types[i] = inferred;
    }

    Type *expected_tuple = create_tuple_type(num_elements, expected_types);
    if (!generic_tuple) {
      return expected_tuple;
    }

    UnifyResult ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
    if (unify(pattern_type, expected_tuple, &ur)) {
      fprintf(stderr, "ERROR: Tuple pattern type mismatch\n");
      return NULL;
    }

    // Solve constraints
    Subst *solution = NULL;
    if (ur.constraints) {
      solution = solve_constraints(ur.constraints);
      if (!solution) {
        fprintf(stderr, "ERROR: Cannot solve tuple pattern constraints\n");
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

      Type *result =
          bind_pattern_recursive(element_pattern, element_type, env, ctx);

      if (!result) {
        return NULL;
      }
    }

    return apply_substitution(ctx->subst, pattern_type);
  }
  case AST_LIST: {
    // List destructuring: [x, y]
    int num_elements = pattern->data.AST_LIST.len;

    // Create expected tuple type with fresh variables
    Type *lhs_el_type = next_tvar();

    Type *expected_ltype = create_list_type_of_type(lhs_el_type);

    // Unify with actual pattern type
    UnifyResult ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
    if (unify(pattern_type, expected_ltype, &ur)) {
      fprintf(stderr, "ERROR: List pattern type mismatch\n");
      return NULL;
    }

    // Solve constraints
    Subst *solution = NULL;
    if (ur.constraints) {
      solution = solve_constraints(ur.constraints);
      if (!solution) {
        fprintf(stderr, "ERROR: Cannot solve tuple pattern constraints\n");
        return NULL;
      }
    }

    // Apply substitutions
    Subst *combined_subst = compose_subst(solution, ur.subst);
    ctx->subst = compose_subst(combined_subst, ctx->subst);

    // Recursively bind each element
    for (int i = 0; i < num_elements; i++) {
      Ast *element_pattern = &pattern->data.AST_LIST.items[i];
      Type *element_type = apply_substitution(ctx->subst, lhs_el_type);

      Type *result =
          bind_pattern_recursive(element_pattern, element_type, env, ctx);
      if (!result) {
        return NULL;
      }
    }

    return apply_substitution(ctx->subst, pattern_type);
  }

  case AST_APPLICATION: {
    if (is_list_cons_operator(pattern)) {
      // List prepend pattern: head::rest
      Ast *head_pattern = &pattern->data.AST_APPLICATION.args[0];
      Ast *rest_pattern = &pattern->data.AST_APPLICATION.args[1];

      // Create expected list type: [element_type]
      Type *element_type = next_tvar();
      Type *list_type = create_list_type_of_type(element_type);

      // Unify with actual pattern type
      UnifyResult ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
      if (unify(pattern_type, list_type, &ur)) {
        printf("ERROR: List pattern type mismatch\n");
        return NULL;
      }

      Subst *solution = NULL;
      if (ur.constraints) {
        solution = solve_constraints(ur.constraints);
        if (!solution) {
          fprintf(stderr, "ERROR: Cannot solve list pattern constraints\n");
          return NULL;
        }
      }

      Subst *combined_subst = compose_subst(solution, ur.subst);
      ctx->subst = compose_subst(combined_subst, ctx->subst);

      // Get concrete element and list types
      Type *concrete_element_type =
          apply_substitution(ctx->subst, element_type);
      Type *concrete_list_type = apply_substitution(ctx->subst, list_type);

      // Recursively bind head to element type
      Type *head_result =
          bind_pattern_recursive(head_pattern, concrete_element_type, env, ctx);
      if (!head_result) {
        return NULL;
      }

      // Recursively bind tail to list type
      Type *tail_result =
          bind_pattern_recursive(rest_pattern, concrete_list_type, env, ctx);
      if (!tail_result) {
        return NULL;
      }

      return concrete_list_type;
    }

    if (pattern->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {

      const char *applicable_name =
          pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

      Scheme *scheme = lookup_scheme(
          *env,
          pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);

      Type *t = instantiate(scheme, ctx);

      if (is_variant_type(t)) {
        int idx;
        Type *vt = find_variant_member_idx(t, applicable_name, &idx);
        Type *res_type = deep_copy_type(t);
        for (int i = 0; i < vt->data.T_CONS.num_args; i++) {
          vt->data.T_CONS.args[i] =
              bind_pattern_recursive(pattern->data.AST_APPLICATION.args + i,
                                     vt->data.T_CONS.args[i], env, ctx);
        }
        res_type->data.T_CONS.args[idx] = vt;

        return res_type;
      }
    }

    fprintf(stderr, "ERROR: Unsupported application pattern\n");
    return NULL;
  }
  case AST_INT:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_CHAR:
  case AST_BOOL: {
    return infer(pattern, ctx);
  }

  default:
    fprintf(stderr, "ERROR: Unsupported pattern type: %d\n", pattern->tag);
    return NULL;
  }
}
Type *infer_pattern_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx) {
  // Step 1: Infer value type (same as infer_let_simple)
  Type *vtype = infer(val, ctx);
  if (!vtype) {
    return type_error(ctx, val, "Cannot infer value type");
  }

  // Step 2: Apply current substitutions (same as infer_let_simple)
  Subst *s1 = ctx->subst;
  TypeEnv *env_subst = apply_subst_env(s1, ctx->env);
  Type *vtype_subst = apply_substitution(s1, vtype);

  // Step 3: Handle different binding patterns
  TypeEnv *new_env = env_subst;

  if (binding->tag == AST_IDENTIFIER) {
    // Simple binding: let x = val

    Scheme gen_type_scheme = generalize(vtype_subst, env_subst);
    new_env = env_extend(env_subst, binding->data.AST_IDENTIFIER.value,
                         gen_type_scheme.vars, gen_type_scheme.type);
  } else {
    // Complex pattern binding: let (x, y) = val or let x::xs = val

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
