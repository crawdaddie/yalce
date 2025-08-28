#include "./infer_lambda.h"
#include "./infer_binding.h"
#include "serde.h"
#include "types/type_expressions.h"
#include "types/unification.h"

void initial_lambda_signature(int num_params, Type **param_types,
                              AstList *type_annotation_list, TICtx *ctx) {
  for (int i = 0; i < num_params;
       i++, type_annotation_list =
                type_annotation_list ? type_annotation_list->next : NULL) {

    Ast *type_annotation =
        type_annotation_list ? type_annotation_list->ast : NULL;

    param_types[i] =
        type_annotation
            ? instantiate(compute_type_expression(type_annotation, ctx), ctx)
            : next_tvar();
  }
}
void bind_lambda_params(Ast *ast, Type **param_types, TICtx *ctx) {
  int i = 0;
  for (AstList *pl = ast->data.AST_LAMBDA.params; pl; pl = pl->next, i++) {
    Ast *param_pattern = pl->ast;
    Type *param_type = param_types[i];

    if (!bind_pattern_recursive(param_pattern, param_type, ctx)) {
      type_error(ctx, param_pattern, "Cannot bind lambda parameter");
      return;
    }
  }
}

void bind_recursive_ref(Ast *ast, Type *function_type, TICtx *ctx) {
  if (ast->data.AST_LAMBDA.fn_name.chars == NULL) {
    return;
  }

  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;

  // Create an identifier AST node for the recursive function name
  Ast fn_name_ast = {
      .tag = AST_IDENTIFIER,
      .data.AST_IDENTIFIER = {.value = fn_name,
                              .length = ast->data.AST_LAMBDA.fn_name.length}};

  // Bind the function name to its type in the environment for recursive calls
  bind_pattern_recursive(&fn_name_ast, function_type, ctx);
}

void unify_recursive_ref(Ast *ast, Type *recursive_fn_type, Type *result_type,
                         TICtx *ctx) {

  // If this is a recursive function, unify the recursive reference with the
  // final type
  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    TICtx unify_ctx = {};
    if (unify(recursive_fn_type, result_type, &unify_ctx)) {
      type_error(ctx, ast, "Recursive function type mismatch");
    }

    // Apply the unification results
    if (unify_ctx.subst) {
      ctx->subst = compose_subst(unify_ctx.subst, ctx->subst);
      *result_type = *apply_substitution(unify_ctx.subst, result_type);
    }
  }
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  Ast *body = ast->data.AST_LAMBDA.body;
  int num_params = ast->data.AST_LAMBDA.len;

  Type *param_types[num_params];
  initial_lambda_signature(num_params, param_types,
                           ast->data.AST_LAMBDA.type_annotations, ctx);

  TICtx lambda_ctx = *ctx;
  bind_lambda_params(ast, param_types, &lambda_ctx);

  // Create a fresh type variable for the recursive function reference
  Type *recursive_fn_type = next_tvar();
  bind_recursive_ref(ast, recursive_fn_type, &lambda_ctx);

  Type *body_type = infer(body, &lambda_ctx);
  if (!body_type) {
    return type_error(ctx, body, "Cannot infer lambda body type");
  }

  for (int i = 0; i < num_params; i++) {
    param_types[i] = apply_substitution(lambda_ctx.subst, param_types[i]);
  }
  body_type = apply_substitution(lambda_ctx.subst, body_type);

  // Build the final function type
  Type *result_type = body_type;
  for (int i = num_params - 1; i >= 0; i--) {
    result_type = type_fn(param_types[i], result_type);
  }

  // If this is a recursive function, unify the recursive reference with the
  // final type
  unify_recursive_ref(ast, recursive_fn_type, result_type, &lambda_ctx);

  ctx->subst = lambda_ctx.subst;

  return result_type;
}

// Trace example for λ(x, y). x + y:
/*
1. Parameter pattern: (x, y)
2. Create fresh type: t1 for the tuple
3. bind_pattern_recursive((x, y), t1, &env):
   - Expects tuple type: (t2, t3)
   - Unify: t1 ~ (t2, t3) → t1 := (t2, t3)
   - Bind: x : t2, y : t3
   - New environment: {x : t2, y : t3}
4. Infer body: x + y
   - Lookup + : ∀a[Arithmetic]. a → a → a → instantiate to t4 → t4 → t4
   - Generate constraints: t2 := t4, t3 := t4
   - If integers: solve to t2 := Int, t3 := Int, t4 := Int
   - Body type: Int
5. Apply substitutions:
   - t1 becomes (Int, Int)
   - Parameter type: (Int, Int)
6. Build result: (Int, Int) → Int

*/
