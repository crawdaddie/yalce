#include "./trait.h"
#include "builtins.h"
#include "serde.h"
#include "type_ser.h"
#include <string.h>

Type *type_trait_impl(Ast *ast, TICtx *ctx) {

  Ast *impl = ast->data.AST_TRAIT_IMPL.impl;
  Type *impl_type = infer_expr(impl, ctx);
  if (!impl_type) {
    return NULL;
  }

  const char *trait_name = ast->data.AST_TRAIT_IMPL.trait_name.chars;

  Type *trait_template = env_lookup(ctx->env, trait_name);

  const char *type_name = ast->data.AST_TRAIT_IMPL.type.chars;

  Type *target = env_lookup(ctx->env, type_name);

  if (!target) {
    target = lookup_builtin_type(type_name);
  }

  if (!target) {
    type_error(ast, "cannot register trait %s: type %s not in scope",
               trait_name, type_name);
    return NULL;
  }

  TypeClass *tc = t_alloc(sizeof(TypeClass));
  *tc = (TypeClass){.name = trait_name, .module = impl_type};
  typeclasses_extend(target, tc);
  // print_ast(ast);
  // print_type(target);
  // print_type(impl_type);
  // print_type(trait_template);

  if (strcmp(trait_name, TYPE_NAME_TYPECLASS_FROM) == 0) {
    if (impl_type->kind == T_MODULE) {
      for (TypeEnv *te = impl_type->data.T_MODULE.env; te; te = te->next) {
        const char *mname = te->name;
        if (strncmp(mname, "from_", 5) != 0) {
          continue;
        }
        Type *mtype = te->type;
        if (!mtype || mtype->kind != T_FN) {
          continue;
        }
        Type *src = mtype->data.T_FN.from;
        if (!src || is_generic(src)) {
          continue;
        }
        TypeList *src_params = t_alloc(sizeof(TypeList));
        src_params->type = src;
        src_params->next = NULL;
        TypeClass *src_tc = t_alloc(sizeof(TypeClass));
        *src_tc = (TypeClass){
            .name = trait_name, .module = impl_type, .params = src_params};
        typeclasses_extend(target, src_tc);
      }
    }

    Type *a = tvar("a");
    Type *ctor_fn = type_fn(a, target);
    TypeList *scheme_vars = t_alloc(sizeof(TypeList));
    scheme_vars->type = a;
    scheme_vars->next = NULL;
    TypeList *from_params = t_alloc(sizeof(TypeList));
    from_params->type = a;
    from_params->next = NULL;
    Predicate *preds =
        predicate_append_applied(NULL, GenericFrom, target, from_params);
    ctx->env = env_extend_with_preds(ctx->env, type_name, ctor_fn, preds);
    ctx->env->scheme_vars = scheme_vars;
    ctx->env->can_generalize = true;
    ctx->env->needs_generalization = true;
  }

  ast->type = impl_type;
  return impl_type;
}
