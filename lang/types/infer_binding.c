#include "./infer_binding.h"
#include "./common.h"
#include "./type.h"
#include "./unification.h"
#include "parse.h"
#include "serde.h"
#include <string.h>

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

Type *list_cons_bindings(Ast *pattern, Type *pattern_type, TICtx *ctx) {

  // Create expected list type: [element_type]
  Type *element_type = next_tvar();
  Type *list_type = create_list_type_of_type(element_type);

  while (is_list_cons_operator(pattern)) {
    Ast *head = pattern->data.AST_APPLICATION.args;
    Type *head_type = bind_pattern_recursive(head, element_type, ctx);
    pattern = pattern->data.AST_APPLICATION.args + 1;
  }

  bind_pattern_recursive(pattern, list_type, ctx);
  TICtx _ctx = *ctx;
  if (unify(list_type, pattern_type, &_ctx)) {
    return NULL;
  }
  Subst *subst = solve_constraints(_ctx.constraints);

  ctx->env = apply_subst_env(subst, ctx->env);

  return list_type;
}

// Recursively bind patterns (handles nested destructuring)
Type *bind_pattern_recursive(Ast *pattern, Type *pattern_type, TICtx *ctx) {
  if (pattern_type == NULL) {
    return NULL;
  }

  TypeEnv **env = &ctx->env;

  if (ast_is_placeholder_id(pattern)) {
    return pattern_type;
  }
  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    const char *var_name = pattern->data.AST_IDENTIFIER.value;

    Scheme *ex_scheme = lookup_scheme(ctx->env, var_name);

    if (ex_scheme) {
      return instantiate(ex_scheme, ctx);
    }
    Scheme var_scheme = {.vars = NULL, .type = pattern_type};
    ctx->env = env_extend(ctx->env, var_name, var_scheme.vars, var_scheme.type);

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

    TICtx ur = {};
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

      Type *result = bind_pattern_recursive(element_pattern, element_type, ctx);

      if (!result) {
        return NULL;
      }
    }

    return apply_substitution(ctx->subst, pattern_type);
  }
  case AST_LIST: {
    // List destructuring: [x, y]
    int num_elements = pattern->data.AST_LIST.len;
    if (num_elements == 0) {
      return pattern_type;
    }

    // // Create expected tuple type with fresh variables
    // Type *lhs_el_type = next_tvar();
    //
    // Type *expected_ltype = create_list_type_of_type(lhs_el_type);
    //
    // // Unify with actual pattern type
    // TICtx ur = {};
    // if (unify(pattern_type, expected_ltype, &ur)) {
    //   fprintf(stderr, "ERROR: List pattern type mismatch\n");
    //   return NULL;
    // }
    //
    // // Solve constraints
    // Subst *solution = NULL;
    // if (ur.constraints) {
    //   solution = solve_constraints(ur.constraints);
    //   if (!solution) {
    //     fprintf(stderr, "ERROR: Cannot solve tuple pattern constraints\n");
    //     return NULL;
    //   }
    // }
    //
    // // Apply substitutions
    // Subst *combined_subst = compose_subst(solution, ur.subst);
    // ctx->subst = compose_subst(combined_subst, ctx->subst);
    //
    // // Recursively bind each element
    // for (int i = 0; i < num_elements; i++) {
    //   Ast *element_pattern = &pattern->data.AST_LIST.items[i];
    //   Type *element_type = apply_substitution(ctx->subst, lhs_el_type);
    //
    //   Type *result = bind_pattern_recursive(element_pattern, element_type,
    //   ctx); if (!result) {
    //     return NULL;
    //   }
    // }
    //
    // return apply_substitution(ctx->subst, pattern_type);
  }

  case AST_APPLICATION: {
    if (is_list_cons_operator(pattern)) {
      return list_cons_bindings(pattern, pattern_type, ctx);
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
                                     vt->data.T_CONS.args[i], ctx);
        }
        res_type->data.T_CONS.args[idx] = vt;

        return res_type;
      }
    }

    fprintf(stderr, "ERROR: Unsupported application pattern\n");
    return NULL;
  }

  default:
    return infer(pattern, ctx);
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
  ctx->env = env_subst;

  if (binding->tag == AST_IDENTIFIER) {
    // Simple binding: let x = val

    Scheme gen_type_scheme = generalize(vtype_subst, env_subst);
    ctx->env = env_extend(env_subst, binding->data.AST_IDENTIFIER.value,
                          gen_type_scheme.vars, gen_type_scheme.type);
  } else {
    // Complex pattern binding: let (x, y) = val or let x::xs = val

    Type *pattern_result = bind_pattern_recursive(binding, vtype_subst, ctx);

    // print_type(pattern_result);
    // print_type(vtype_subst);

    if (!pattern_result) {
      return type_error(ctx, binding, "Pattern binding failed");
    }
  }

  // Step 4: Handle body (same as infer_let_simple)
  if (body) {
    TICtx body_ctx = *ctx;
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
