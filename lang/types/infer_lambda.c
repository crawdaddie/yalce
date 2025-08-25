#include "./infer_lambda.h"
#include "./infer_binding.h"
#include "serde.h"
#include "types/type_expressions.h"

Type *infer_lambda(Ast *ast, TICtx *ctx) {

  Ast *body = ast->data.AST_LAMBDA.body;
  int num_params = ast->data.AST_LAMBDA.len;

  // Step 1: Create fresh type variables for each parameter pattern
  Type **param_types = talloc(sizeof(Type *) * num_params);
  AstList *type_annotations = ast->data.AST_LAMBDA.type_annotations;
  for (int i = 0; i < num_params;
       i++, type_annotations = type_annotations ? type_annotations->next
                                                : NULL) {

    if (type_annotations && type_annotations->ast) {
      // has annotated type
      Scheme *annotated_scheme =
          compute_type_expression(type_annotations->ast, ctx);
      param_types[i] = instantiate(annotated_scheme, ctx);
    } else {
      param_types[i] = next_tvar();
    }
  }

  // Step 2: Extend environment using pattern binding for each parameter
  TypeEnv *new_env = ctx->env;

  int i = 0;
  for (AstList *pl = ast->data.AST_LAMBDA.params,
               *tp = ast->data.AST_LAMBDA.type_annotations;
       pl; pl = pl->next, tp = tp ? tp->next : NULL, i++) {

    Ast *param_pattern = pl->ast;
    Type *param_type = param_types[i];

    // Use bind_pattern_recursive to handle any pattern type
    Type *pattern_result =
        bind_pattern_recursive(param_pattern, param_type, &new_env, ctx);

    if (!pattern_result) {
      return type_error(ctx, param_pattern,
                        "Cannot bind lambda parameter pattern %d", i);
    }
  }
  const char *name = ast->data.AST_LAMBDA.fn_name.chars;
  Type *rec_fn_type = NULL;

  if (name) {
    rec_fn_type = next_tvar();

    Ast rec_fn_name_binding = {
        AST_IDENTIFIER,
        {.AST_IDENTIFIER = {.value = name,
                            .length = ast->data.AST_LAMBDA.fn_name.length}}};

    bind_pattern_recursive(&rec_fn_name_binding, rec_fn_type, &new_env, ctx);
  }

  // Step 3: Infer body type in extended environment
  TICtx body_ctx = {
      .env = new_env, .subst = ctx->subst, .err_stream = ctx->err_stream};

  Type *body_type = infer(body, &body_ctx);
  if (!body_type) {
    return type_error(ctx, body, "Cannot infer lambda body type");
  }

  // Step 4: Apply substitutions from body inference to parameter types
  Subst *s1 = body_ctx.subst;
  Type **param_types_subst = talloc(sizeof(Type *) * num_params);
  for (int i = 0; i < num_params; i++) {
    param_types_subst[i] = apply_substitution(s1, param_types[i]);
  }

  // Step 5: Build curried function type (right-associative)
  Type *result_type = body_type;

  for (int i = num_params - 1; i >= 0; i--) {
    result_type = type_fn(param_types_subst[i], result_type);
  }

  // Step 6: Update context with final substitutions
  ctx->subst = s1;

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
