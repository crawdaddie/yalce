#include "./type_expressions.h"
#include "./builtins.h"
#include "./inference.h"
#include "serde.h"
#include "types/type_ser.h"
#include <string.h>

static const char *current_type_decl_name = NULL;
static TypeEnv *current_type_decl_env = NULL;
static TypeEnv *current_type_var_env = NULL;

static Type *compute_type_expression_inner(Ast *expr, TICtx *ctx);

static Type *lookup_or_bind_type_var(const char *name) {
  TypeEnv *bound = lookup_type_ref(current_type_var_env, name);
  if (bound) {
    return bound->type;
  }

  Type *type = tvar(name);
  TypeEnv *entry = t_alloc(sizeof(TypeEnv));
  *entry = (TypeEnv){.name = name, .type = type, .next = current_type_var_env};
  current_type_var_env = entry;
  return type;
}

Type *create_sum_type(int len, Type **members) {
  Type *sum = empty_type();
  sum->kind = T_SUM;
  sum->data.T_CONS.name =
      current_type_decl_name ? current_type_decl_name : TYPE_NAME_VARIANT;
  sum->data.T_CONS.args = members;
  sum->data.T_CONS.num_args = len;
  return sum;
}
int bind_type_in_ctx(Ast *binding, Type *type, binding_md bmd_type, TICtx *ctx);

Type *compute_fn_type(Ast *expr, TICtx *ctx) {
  Ast *sig = expr;

  int num_params = 0;

  while (sig->tag == AST_FN_SIGNATURE || sig->tag == AST_LIST) {
    num_params++;
    sig = sig->data.AST_LIST.items + 1;
  }
  sig = expr;

  int is_variadic = false;
  Type *param_types[num_params];
  for (int i = 0; i < num_params; i++) {
    Ast *p = sig->data.AST_LIST.items;
    Type *t = compute_type_expression_inner(p, ctx);
    if (t && t->kind == T_CONS &&
        strcmp(t->data.T_CONS.name, "Variadic") == 0) {
      is_variadic = true;
      t = t->data.T_CONS.args[0];
    }
    param_types[i] = t;
    sig = sig->data.AST_LIST.items + 1;
  }
  Type *ret = compute_type_expression_inner(sig, ctx);
  Type *f = create_type_multi_param_fn(num_params, param_types, ret);

  if (is_variadic) {
    Type **x = t_alloc(sizeof(Type *));
    x[0] = f;
    f = create_cons_type("Variadic", 1, x);
  }

  return f;
}

static Type *compute_type_expression_inner(Ast *expr, TICtx *ctx) {
  switch (expr->tag) {
  case AST_FN_SIGNATURE: {
    return compute_fn_type(expr, ctx);
    break;
  }
  case AST_LET: {
    // Handle constrained type annotations like T: (Double ... -> Double).
    // ast_assoc builds an AST_LET with binding=identifier, expr=constraint.
    // The constraint type (e.g. Variadic(Double -> Double)) is stored on the
    // type variable's meta field.  During application inference, when a lambda
    // is constrained against this tvar, the variadic template is expanded to
    // match the lambda's arity and a constraint is added.
    Ast *binding = expr->data.AST_LET.binding;
    Ast *constraint_expr = expr->data.AST_LET.expr;
    if (binding && binding->tag == AST_IDENTIFIER) {
      const char *name = binding->data.AST_IDENTIFIER.value;
      Type *tvar_type = lookup_or_bind_type_var(name);
      if (constraint_expr) {
        Type *constraint = compute_type_expression_inner(constraint_expr, ctx);
        if (constraint) {
          tvar_type->meta = constraint;
        }
      }
      return tvar_type;
    }
    return NULL;
  }
  case AST_VOID: {
    return &t_void;
  }

  case AST_IDENTIFIER: {
    const char *name = expr->data.AST_IDENTIFIER.value;
    if (current_type_decl_name && strcmp(name, current_type_decl_name) == 0) {
      return trec(name, current_type_decl_env);
    }
    // if (CHARS_EQ(name, TYPE_NAME_PTR)) {
    //   return deep_copy_type(&t_ptr);
    // }

    Type *builtin_type = lookup_builtin_type(name);

    if (builtin_type) {
      return builtin_type;
    }

    TypeEnv *type_ref = lookup_type_ref(ctx->env, name);

    if (type_ref) {
      if ((type_ref->md.type == BT_TYPE_DECL ||
           type_ref->md.type == BT_TYPE_CONSTRUCTOR) &&
          type_ref->type) {
        return resolve_type_in_env(
            // deep_copy_type(type_ref->type),
            type_ref->type, ctx->env);
      }
      return type_ref->type;
    }

    // Resolve via the type-var name env (seeded by parametrized modules).
    TypeEnv *tv_ref = lookup_type_ref(current_type_var_env, name);
    if (tv_ref) {
      return tv_ref->type;
    }

    return lookup_or_bind_type_var(name);
  }
  case AST_TUPLE: {
    // print_ast("compute tuple??\n");
    // print_ast(expr);
    int len = expr->data.AST_LIST.len;
    Type **members = t_alloc(sizeof(Type *) * len);
    const char **names = NULL;
    if (expr->data.AST_LIST.items[0].tag == AST_LET) {
      names = t_alloc(sizeof(char *) * len);
    }

    for (int i = 0; i < len; i++) {

      Ast *mem_ast = expr->data.AST_LIST.items + i;

      if (mem_ast->tag == AST_LET) {
        names[i] = mem_ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;
        mem_ast = mem_ast->data.AST_LET.expr;
      }

      Type *mem = compute_type_expression_inner(mem_ast, ctx);

      members[i] = mem;
    }

    Type *tuple_type = create_tuple_type(len, members);

    if (names) {
      tuple_type->data.T_CONS.names = names;
    }

    return tuple_type;
  }

  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    Type **members = t_alloc(sizeof(Type *) * len);
    const char **names = malloc(sizeof(char *) * len);
    for (int i = 0; i < len; i++) {
      const char *name;
      Ast *mem_ast = expr->data.AST_LIST.items + i;
      if (mem_ast->tag == AST_IDENTIFIER) {
        name = mem_ast->data.AST_IDENTIFIER.value;
        members[i] =
            create_cons_type(mem_ast->data.AST_IDENTIFIER.value, 0, NULL);
      } else {
        Ast *item = mem_ast;

        if (item->tag == AST_BINOP &&
            item->data.AST_BINOP.left->tag == AST_IDENTIFIER) {

          name = item->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
        }

        Type *sch =
            compute_type_expression_inner(expr->data.AST_LIST.items + i, ctx);
        if (!sch) {
          return NULL;
        }
        members[i] = sch;
      }

      names[i] = name;

      if (members[i]->kind == T_CONS || members[i]->kind == T_SUM) {
        members[i]->data.T_CONS.name = name;
      }
    }

    Type *sum_type = create_sum_type(len, members);
    sum_type->data.T_CONS.names = names;

    // computed->data.T_CONS.names =
    //     malloc(sizeof(char *) * computed->data.T_CONS.num_args);
    // for (int i = 0; i < expr->data.AST_LIST.len; i++) {
    //   Ast *name = i
    // }
    return sum_type;
  }

  case AST_BINOP: {
    token_type op = expr->data.AST_BINOP.op;
    if (op == TOKEN_OF) {
      Ast *container_ast = expr->data.AST_BINOP.left;
      Ast *contained_ast = expr->data.AST_BINOP.right;

      Type *container = compute_type_expression_inner(container_ast, ctx);
      if (container->kind == T_VAR) {
        container->kind = T_CONS;
      }

      if (!container) {
        return type_error(container_ast, "could not find type");
      }

      Type *contained = compute_type_expression_inner(contained_ast, ctx);

      if (is_pointer_type(container)) {
        container = deep_copy_type(container);
        container->data.T_CONS.args = t_alloc(sizeof(Type *));
        container->data.T_CONS.args[0] = contained;
        container->data.T_CONS.num_args = 1;
        return container;
      }

      if (is_list_type(container)) {
        return create_list_type_of_type(contained);
      }

      if (is_option_type(container)) {
        return create_option_type(contained);
      }

      container = deep_copy_type(container);
      container->data.T_CONS.args = t_alloc(sizeof(Type *));
      container->data.T_CONS.args[0] = contained;
      container->data.T_CONS.num_args = 1;

      return container;
    }
    //
    //   Scheme *container = lookup_scheme(
    //       ctx->env, expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
    //
    //   if (!container) {
    //
    //     fprintf(stderr, "Error: could not find type %s\n",
    //             expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
    //     return NULL;
    //   }
    //
    //   Type *contained =
    //       compute_type_expression(expr->data.AST_BINOP.right, ctx);
    //
    //   Type *inst = instantiate_scheme_with_args(
    //       container, expr->data.AST_BINOP.right, ctx);
    //
    //   return inst;
    // }
  }
  case AST_LAMBDA: {
    for (AstList *ps = expr->data.AST_LAMBDA.params; ps; ps = ps->next) {
      Ast *p = ps->ast;
      current_type_var_env =
          env_extend(current_type_var_env, p->data.AST_IDENTIFIER.value,
                     tvar(p->data.AST_IDENTIFIER.value));
    }
    return compute_type_expression_inner(expr->data.AST_LAMBDA.body, ctx);
  }
  default: {
    break;
  }
  }

  return NULL;
}

Type *compute_type_expression(Ast *expr, TICtx *ctx) {
  // Save and clear the type-var name env, UNLESS a caller has explicitly
  // seeded it (parametrized-module inference). A seeded env must persist so
  // the body's type annotations resolve the module's generic tvars by name.
  TypeEnv *saved_type_var_env = current_type_var_env;
  bool seeded = current_type_var_env && current_type_var_env->is_module_seed;
  if (!seeded) {
    current_type_var_env = NULL;
  }
  Type *result = compute_type_expression_inner(expr, ctx);
  current_type_var_env = saved_type_var_env;
  return result;
}

static bool type_decl_introduces_constructors(Ast *expr) {
  return expr && expr->tag == AST_LIST;
}

static bool type_contains_unwrapped_recursive_ref(Type *type,
                                                  const char *name) {
  if (!type || !name) {
    return false;
  }

  switch (type->kind) {
  case T_RECURSIVE_REF:
    return type->data.T_RECURSIVE_REF.name &&
           strcmp(type->data.T_RECURSIVE_REF.name, name) == 0;

  case T_VAR:
    return type->is_recursive_type_ref && type->data.T_VAR.name &&
           strcmp(type->data.T_VAR.name, name) == 0;

  case T_FN:
    return false;

  case T_CONS:
  case T_SUM:
    if (is_array_type(type) || is_list_type(type) || is_pointer_type(type) ||
        is_coroutine_type(type)) {
      return false;
    }

    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (type_contains_unwrapped_recursive_ref(type->data.T_CONS.args[i],
                                                name)) {
        return true;
      }
    }
    return false;

  default:
    return false;
  }
}

void compute_lambda_param_types(AstList *annotations, size_t len, Type **out,
                                TICtx *ctx) {
  TypeEnv *saved_type_var_env = current_type_var_env;
  // Preserve a module seed (set by parametrized-module inference) so that
  // type annotations inside member lambda params (e.g. `fn x: (a) -> ...`
  // within `add`) resolve `a` to the SAME module tvar as the body's other
  // annotations. Without this, each lambda's param annotations would get a
  // fresh tvar, breaking the shared-`a invariant across module members.
  bool seeded = current_type_var_env && current_type_var_env->is_module_seed;
  if (!seeded) {
    current_type_var_env = NULL;
  }
  for (size_t i = 0; i < len && annotations;
       i++, annotations = annotations->next) {
    if (annotations->ast) {
      out[i] = compute_type_expression_inner(annotations->ast, ctx);
    } else {
      out[i] = NULL;
    }
  }
  current_type_var_env = saved_type_var_env;
}

void compute_module_param_types(AstList *annotations, size_t len, Type **out,
                                TICtx *ctx) {
  // Unlike compute_lambda_param_types, do NOT reset current_type_var_env to
  // NULL. The caller (parametrized-module inference) seeds it with the
  // module's generic tvars so they remain resolvable by name, and leaves
  // the seeded env in place for the body's type-expression resolution to
  // inherit.
  for (size_t i = 0; i < len && annotations;
       i++, annotations = annotations->next) {
    if (annotations->ast) {
      out[i] = compute_type_expression_inner(annotations->ast, ctx);
    } else {
      out[i] = NULL;
    }
  }
}

void set_type_var_env(TypeEnv *env) {
  // Mark the head (and thus the whole chain) as a module seed so
  // compute_type_expression preserves it across its per-call resets.
  if (env) {
    env->is_module_seed = true;
  }
  current_type_var_env = env;
}
TypeEnv *get_type_var_env(void) { return current_type_var_env; }

int bind_pattern(Ast *pattern, Type *value_type, TICtx *ctx);

Type *infer_type_declaration(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;

  // if (expr->tag == AST_LAMBDA) {
  //   expr = expr->data.AST_LAMBDA.body;
  // }

  const char *saved_type_decl_name = current_type_decl_name;
  TypeEnv *saved_type_decl_env = current_type_decl_env;
  TypeEnv *decl_env = NULL;

  if (binding->tag == AST_IDENTIFIER) {
    current_type_decl_name = binding->data.AST_IDENTIFIER.value;
    decl_env = t_alloc(sizeof(TypeEnv));
    memset(decl_env, 0, sizeof(TypeEnv));
    decl_env->name = current_type_decl_name;
    decl_env->md.type = BT_TYPE_DECL;
    current_type_decl_env = decl_env;
  }

  Type *computed = compute_type_expression(expr, ctx);
  current_type_decl_name = saved_type_decl_name;
  current_type_decl_env = saved_type_decl_env;

  if (!computed) {
    fprintf(stderr, "Error: could not compute type expression");
    print_ast_err(expr);
    return NULL;
  }

  bool introduces_constructors = type_decl_introduces_constructors(expr);

  if (binding->tag == AST_IDENTIFIER && computed->kind == T_CONS) {
    const char *type_name = binding->data.AST_IDENTIFIER.value;
    computed = deep_copy_type(computed);
    computed->alias = type_name;
  }

  if (binding->tag == AST_IDENTIFIER &&
      (computed->kind == T_CONS || computed->kind == T_SUM) &&
      type_contains_unwrapped_recursive_ref(
          computed, binding->data.AST_IDENTIFIER.value)) {
    return type_error(ast,
                      "recursive type references must be behind Array/List or "
                      "another pointer-like boundary");
  }

  if (decl_env) {
    decl_env->type = computed;
  }

  if (binding->tag == AST_IDENTIFIER && computed->kind == T_SUM &&
      introduces_constructors) {
    const char *type_name = binding->data.AST_IDENTIFIER.value;
    computed->data.T_CONS.name = type_name;
  }

  if (bind_pattern(binding, computed, ctx)) {
    type_error(ast, "Unsupported let binding shape");
    return NULL;
  }
  if (binding->tag == AST_IDENTIFIER && ctx->env &&
      strcmp(ctx->env->name, binding->data.AST_IDENTIFIER.value) == 0) {
    ctx->env->md.type = BT_TYPE_DECL;
  }
  if (is_sum_type(computed) && introduces_constructors) {
    for (int i = 0; i < computed->data.T_CONS.num_args; i++) {
      Type *mem = computed->data.T_CONS.args[i];
      Type *ctor_type = computed;
      if (mem->kind == T_CONS && mem->data.T_CONS.num_args > 0) {
        ctor_type = create_type_multi_param_fn(mem->data.T_CONS.num_args,
                                               mem->data.T_CONS.args, computed);
      }
      ctx->env = env_extend(ctx->env, mem->data.T_CONS.name, ctor_type);
      ctx->env->md.type = BT_TYPE_CONSTRUCTOR;
    }
  }

  return computed;
}
