#include "inference.h"
#include "serde.h"
#include "types/type_declaration.h"
#include "types/unification.h"
#include <stdlib.h>
#include <string.h>

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

static const char *fresh_tvar_name() {
  char *new_name = talloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

Type *next_tvar() {
  Type *tvar = talloc(sizeof(Type));
  *tvar = (Type){T_VAR, {.T_VAR = fresh_tvar_name()}};
  return tvar;
}
#define TRY_INFER(ast, env, msg)                                               \
  ({                                                                           \
    Type *t = infer(ast, env);                                                 \
    if (!t) {                                                                  \
      char *buf = malloc(sizeof(char) * 500);                                  \
      ast_to_sexpr(ast, buf);                                                  \
      if (msg)                                                                 \
        fprintf(stderr, "%s %s\n", msg, buf);                                  \
      free(buf);                                                               \
      return NULL;                                                             \
    }                                                                          \
    t;                                                                         \
  })

static char *op_to_name[] = {
    [TOKEN_PLUS] = "+",      [TOKEN_MINUS] = "-",      [TOKEN_STAR] = "*",
    [TOKEN_SLASH] = "/",     [TOKEN_MODULO] = "%",     [TOKEN_LT] = "<",
    [TOKEN_GT] = ">",        [TOKEN_LTE] = "<=",       [TOKEN_GTE] = ">=",
    [TOKEN_EQUALITY] = "==", [TOKEN_NOT_EQUAL] = "!=",
};
/*
Type *infer_binop_(Ast *ast, TypeEnv **env) {

  Type *lt = TRY_INFER(ast->data.AST_BINOP.left, env,
                       "Failure typechecking lhs of binop: ");

  Type *rt = TRY_INFER(ast->data.AST_BINOP.right, env,
                       "Failure typechecking rhs of binop: ");

  token_type op = ast->data.AST_BINOP.op;

  if (types_equal(lt, rt)) {
    return resolve_op_typeclass_in_type(lt, op);
  }

  if ((!is_generic(lt)) && (!is_generic(rt))) {
    return resolve_binop_typeclass(lt, rt, op);
  }

  Type *result_type = NULL;
  if (is_generic(lt) && (!is_generic(rt))) {
    result_type = lt;
  } else if (is_generic(rt) && (!is_generic(lt))) {
    result_type = rt;
  } else if (is_generic(lt) && is_generic(rt)) {
    result_type = rt;
  }

  if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
    add_typeclass(result_type, derive_arithmetic_for_type(result_type));
  } else if (op >= TOKEN_LT && op <= TOKEN_GTE) {
    add_typeclass(result_type, derive_ord_for_type(result_type));
  } else if (op >= TOKEN_EQUALITY && op <= TOKEN_NOT_EQUAL) {
    add_typeclass(result_type, derive_eq_for_type(result_type));
  }

  // TODO: find typeclass rank for non-generic operand and apply to result_type
  // as 'min rank'
  // for example x + 1.0 -> 'x [arithmetic]
  // the result type is at least the same rank as float
  // if x is an int, then the result should be a float
  // if x is something 'higher' than a float use that instead

  return result_type;
}
*/

Type *infer_binop(Ast *ast, TypeEnv **env) {

  token_type op = ast->data.AST_BINOP.op;
  Type *l = TRY_INFER(ast->data.AST_BINOP.left, env,
                      "Failure typechecking lhs of binop: ");

  Type *r = TRY_INFER(ast->data.AST_BINOP.right, env,
                      "Failure typechecking rhs of binop: ");

  int lmidx;
  TypeClass *ltc = find_op_typeclass_in_type(l, op, &lmidx);
  int rmidx;
  TypeClass *rtc = find_op_typeclass_in_type(r, op, &rmidx);

  TypeClass *tc;
  Method meth;

  if (ltc && rtc) {
    if (ltc->rank >= rtc->rank) {
      meth = ltc->methods[lmidx];
    } else {
      meth = rtc->methods[rmidx];
    }
  } else {

    switch (op) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MODULO: {

      if (!ltc) {
        ltc = derive_arithmetic_for_type(l);
        add_typeclass(l, ltc);
      }

      if (!rtc) {
        rtc = derive_arithmetic_for_type(r);
        add_typeclass(r, rtc);
      }

      Type *type = empty_type();
      type->kind = T_TYPECLASS_RESOLVE;

      type->data.T_TYPECLASS_RESOLVE.dependencies = talloc(sizeof(Type *) * 2);
      type->data.T_TYPECLASS_RESOLVE.dependencies[0] = l;
      type->data.T_TYPECLASS_RESOLVE.dependencies[1] = r;
      type->data.T_TYPECLASS_RESOLVE.comparison_tc = "arithmetic";
      return type;
    }

    case TOKEN_LT:
    case TOKEN_GT:
    case TOKEN_LTE:
    case TOKEN_GTE: {

      if (!ltc) {
        ltc = derive_ord_for_type(l);
        add_typeclass(l, ltc);
      }

      if (!rtc) {
        rtc = derive_ord_for_type(r);
        add_typeclass(r, rtc);
      }

      meth = ltc->methods[op - TOKEN_LT];
      break;
    }

    case TOKEN_EQUALITY:
    case TOKEN_NOT_EQUAL: {
      if (!ltc) {
        ltc = derive_eq_for_type(l);
        add_typeclass(l, ltc);
      }

      if (!rtc) {
        rtc = derive_eq_for_type(r);
        add_typeclass(r, rtc);
      }
      meth = ltc->methods[op - TOKEN_EQUALITY];
      break;
    }

    default: {
      fprintf(stderr, "Error op %d unrecognized as binop\n", op);
      return NULL;
    }
    }
  }
  return fn_return_type(meth.signature);
}

static TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }
  case AST_TUPLE: {
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      env = add_binding_to_env(env, binding->data.AST_LIST.items + i,
                               type->data.T_CONS.args[i]);
    }
    return env;
  }
  }
  return env;
}

/*

static TypeEnv *set_param_binding(Ast *ast, TypeEnv **env) {
    if (is_variant_member(test_type, *env)) {
    }
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    ast->md = next_tvar();
    const char *name = ast->data.AST_IDENTIFIER.value;
    *env = add_binding_to_env(*env, ast, ast->md);
    return *env;
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = talloc(sizeof(Type) * len);
    for (int i = 0; i < len; i++) {
      *env = set_param_binding(ast->data.AST_LIST.items + i, env);
      tuple_mems[i] = ast->data.AST_LIST.items[i].md;
      ast->md = talloc(sizeof(Type));
      *((Type *)ast->md) =
          (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, tuple_mems, len}}};
    }
    return *env;
  }
  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}
*/

static Type *binding_type(Ast *ast) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    return next_tvar();
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = malloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      tuple_mems[i] = binding_type(ast->data.AST_LIST.items + i);
    }
    Type *tup = empty_type();
    *tup = (Type){T_CONS,
                  {.T_CONS = {.name = TYPE_NAME_TUPLE,
                              .args = tuple_mems,
                              .num_args = len}}};
    return tup;
  }

  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}

static Type *infer_lambda(Ast *ast, TypeEnv **env) {

  int len = ast->data.AST_LAMBDA.len;
  TypeEnv *fn_scope_env = *env;

  Type *return_type = next_tvar();
  Type *fn;
  if (len == 1 && ast->data.AST_LAMBDA.params->tag == AST_VOID) {
    fn = &t_void;
    fn = type_fn(fn, return_type);
  } else {
    Type *param_types[len];

    for (int i = len - 1; i >= 0; i--) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      Type *ptype = binding_type(param_ast);
      param_types[i] = ptype;
      fn_scope_env = add_binding_to_env(fn_scope_env, param_ast, ptype);
    }
    fn = create_type_multi_param_fn(len, param_types, return_type);
  }

  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    fn_scope_env =
        env_extend(fn_scope_env, ast->data.AST_LAMBDA.fn_name.chars, fn);
  }

  Type *body_type = infer(ast->data.AST_LAMBDA.body, &fn_scope_env);

  TypeEnv **_env = env;
  Type *unified_ret = unify(return_type, body_type, _env);
  *return_type = *unified_ret;

  return fn;
}

Type *generic_cons(Type *generic_cons, int len, Ast *args, TypeEnv **env) {

  if (len > generic_cons->data.T_CONS.num_args) {
    fprintf(stderr, "Error, too many arguments to cons type");
    return NULL;
  }

  Type *type = copy_type(generic_cons);

  for (int i = 0; i < len; i++) {
    Type *t = infer(args + i, env);
    if (type->data.T_CONS.args[i]->kind == T_VAR) {
      const char *name = type->data.T_CONS.args[i]->data.T_VAR;
      Type *existing_binding = env_lookup(*env, name);
      if (existing_binding) {
        if (!types_equal(existing_binding, t)) {
          fprintf(stderr, "Error: generic type %s already bound in env to ",
                  name);
          print_type(existing_binding);
          printf(" != ");
          print_type(t);
          return NULL;
        }
      }

      *env = env_extend(*env, name, t);
    }
    type->data.T_CONS.args[i] = infer(args + i, env);
  }
  return type;
}

Type *replace_in(Type *type, Type *tvar, Type *replacement);
static Type *resolve_generic_variant(Type *t, TypeEnv *env) {
  while (env) {
    const char *key = env->name;
    Type tvar = {T_VAR, .data = {.T_VAR = key}};
    Type *replaced = replace_in(t, &tvar, env->type);
    t = replaced;

    env = env->next;
  }

  return t;
}

static Type *resolve_generic_fn(Type *t, TypeEnv *env) {
  while (env) {
    const char *key = env->name;
    Type tvar = {T_VAR, .data = {.T_VAR = key}};
    t = replace_in(t, &tvar, env->type);

    env = env->next;
  }

  return t;
}

static Type *resolve_generic_type(Type *t, TypeEnv *env) {
  while (env) {
    const char *key = env->name;
    Type tvar = {T_VAR, .data = {.T_VAR = key}};
    t = replace_in(t, &tvar, env->type);
    env = env->next;
  }

  return t;
}

static bool type_is_variant_member(Type *member, Type *variant) { return true; }

Type *infer_match(Ast *ast, TypeEnv **env) {
  Ast *with = ast->data.AST_MATCH.expr;
  TypeEnv *test_expr_env = *env;
  Type *with_type = infer(with, &test_expr_env);
  Type *result_type = NULL;

  int len = ast->data.AST_MATCH.len;
  Ast *branches = ast->data.AST_MATCH.branches;

  Type *res_type = NULL;
  Type *test_type = NULL;

  Type *variant = NULL;
  for (int i = 0; i < len; i++) {
    Ast *test_expr = branches + (2 * i);
    test_type = infer(test_expr, &test_expr_env);

    if (test_type == NULL) {
      fprintf(stderr, "could not resolve match test expr in branch %d\n", i);
      return NULL;
    }

    Type *_v = variant_lookup(*env, test_type);
    if (_v != NULL && variant == NULL) {
      variant = copy_type(_v);
    } else if (variant != NULL && _v != NULL) {
      unify(variant, _v, env);
    }

    TypeEnv *res_env = add_binding_to_env(*env, test_expr, test_type);
    Ast *result_expr = branches + (2 * i + 1);
    Type *_res_type = infer(result_expr, &res_env);

    if (res_type != NULL) {
      Type *unif_res = unify(res_type, _res_type, env);
      if (!unif_res) {
        fprintf(stderr, "Error cannot unify result types in match expression ");
        print_type_err(res_type);
        fprintf(stderr, " != ");
        print_type_err(_res_type);
        return NULL;
      }
      *res_type = *unif_res;
    }
    res_type = _res_type;
  }

  if (variant) {
    *with_type = *resolve_generic_variant(variant, test_expr_env);
  }

  return res_type;
}

typedef struct TypeMap {
  Type *key;
  Type *val;
  struct TypeMap *next;
} TypeMap;

TypeMap *constraints_map_extend(TypeMap *map, Type *key, Type *val) {
  switch (key->kind) {
  case T_VAR: {
    TypeMap *new_map = talloc(sizeof(TypeMap));
    new_map->key = key;
    new_map->val = val;
    new_map->next = map;
    return new_map;
  }
  }
  return map;
}
void print_constraints_map(TypeMap *map) {
  if (map) {
    print_type(map->key);
    printf(" : ");
    print_type(map->val);
    print_constraints_map(map->next);
  }
}
Type *resolve_tc_rank(Type *type) {
  const char *comparison_tc = type->data.T_TYPECLASS_RESOLVE.comparison_tc;
  Type *dep1 = type->data.T_TYPECLASS_RESOLVE.dependencies[0];
  Type *dep2 = type->data.T_TYPECLASS_RESOLVE.dependencies[1];
  dep1 = dep1->kind == T_TYPECLASS_RESOLVE ? resolve_tc_rank(dep1) : dep1;
  dep2 = dep2->kind == T_TYPECLASS_RESOLVE ? resolve_tc_rank(dep2) : dep2;
  TypeClass *tc1 = get_typeclass_by_name(dep1, comparison_tc);
  TypeClass *tc2 = get_typeclass_by_name(dep2, comparison_tc);
  if (tc1->rank >= tc2->rank) {
    return dep1;
  }
  return dep2;
}

Type *replace_in(Type *type, Type *tvar, Type *replacement) {

  switch (type->kind) {
  case T_TYPECLASS_RESOLVE: {

    type->data.T_TYPECLASS_RESOLVE.dependencies[0] = replace_in(
        type->data.T_TYPECLASS_RESOLVE.dependencies[0], tvar, replacement);

    type->data.T_TYPECLASS_RESOLVE.dependencies[1] = replace_in(
        type->data.T_TYPECLASS_RESOLVE.dependencies[1], tvar, replacement);

    if (!is_generic(type)) {
      return resolve_tc_rank(type);
    }

    return type;
  }

  case T_CONS: {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      type->data.T_CONS.args[i] =
          replace_in(type->data.T_CONS.args[i], tvar, replacement);
    }
    return type;
  }
  case T_FN: {
    type->data.T_FN.from = replace_in(type->data.T_FN.from, tvar, replacement);
    type->data.T_FN.to = replace_in(type->data.T_FN.to, tvar, replacement);
    return type;
  }

  case T_VAR: {
    if (strcmp(type->data.T_VAR, tvar->data.T_VAR) == 0) {
      return replacement;
    }
    return type;
  }
  default:
    return type;
  }
}

Type *constraints_map_lookup(TypeMap *map, Type *key) {
  while (map) {
    if (types_equal(map->key, key)) {
      return map->val;
    }

    if (occurs_check(map->key, key)) {
      return replace_in(key, map->key, map->val);
    }

    map = map->next;
  }
  return NULL;
}

Type *_infer_fn_application(Type *fn_type, Type **arg_types, int len,
                            TypeEnv **env) {
  fn_type = copy_type(fn_type);

  if (fn_type->kind != T_FN) {
    fprintf(stderr, "Error: Attempting to apply a non-function type ");
    print_type_err(fn_type);
    return NULL;
  }

  Type *a = fn_type;
  TypeMap *map = NULL;
  // TypeEnv *map = NULL;

  for (int i = 0; i < len; i++) {
    Type *app_arg_type = arg_types[i];

    Type *fn_arg_type = a->data.T_FN.from;

    Type *unif = unify(fn_arg_type, app_arg_type, env);

    map = constraints_map_extend(map, fn_arg_type, app_arg_type);

    if (a->kind != T_FN) {
      fprintf(stderr, "Error too may args (%d) passed to fn\n", len);
      print_type_err(fn_type);
      return NULL;
    }

    a = a->data.T_FN.to;
  }

  Type *app_result_type = a;

  if (app_result_type->kind == T_FN) {
    TypeMap *_map = map;
    while (_map) {
      app_result_type = replace_in(app_result_type, _map->key, _map->val);
      _map = _map->next;
    }
    return app_result_type;
  }

  if (is_generic(app_result_type)) {

    Type *lookup = constraints_map_lookup(map, app_result_type);

    // lookup = resolve_generic_variant(lookup, *env);

    if (lookup == NULL) {
      return resolve_generic_type(app_result_type, *env);
      // fprintf(stderr, "Error: constraint not found in constraint map\n");
      // print_type(app_result_type);
      // printf("\n");
    }
    return lookup;
  }

  return app_result_type;
}

Type *infer_fn_application(Ast *ast, TypeEnv **env) {
  Type *fn_type = infer(ast->data.AST_APPLICATION.function, env);

  int len = ast->data.AST_APPLICATION.len;
  Type *app_arg_types[len];
  for (int i = 0; i < len; i++) {
    app_arg_types[i] = TRY_INFER(ast->data.AST_APPLICATION.args + i, env,
                                 "could not infer application argument");
  }

  return _infer_fn_application(fn_type, app_arg_types, len, env);
}

Type *infer(Ast *ast, TypeEnv **env) {

  Type *type = NULL;
  switch (ast->tag) {
  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {

      stmt = ast->data.AST_BODY.stmts[i];
      TRY_INFER(stmt, env, "Failure typechecking body statement: ");
    }
    type = stmt->md;
    break;
  }
  case AST_INT: {
    type = &t_int;
    break;
  }

  case AST_DOUBLE: {
    type = &t_num;
    break;
  }

  case AST_BOOL: {
    type = &t_bool;
    break;
  }

  case AST_VOID: {
    type = &t_void;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }
  case AST_BINOP: {
    type = infer_binop(ast, env);
    break;
  }
  case AST_LET: {
    Ast *expr = ast->data.AST_LET.expr;
    Type *expr_type = TRY_INFER(expr, env, NULL);
    Ast *binding = ast->data.AST_LET.binding;
    *env = add_binding_to_env(*env, binding, expr_type);

    Ast *in_expr = ast->data.AST_LET.in_expr;
    if (in_expr) {
      Type *in_expr_type = TRY_INFER(in_expr, env, NULL);
      type = in_expr_type;
    } else {
      type = expr_type;
    }

    break;
  }
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(ast)) {
      type = next_tvar();
      break;
    }
    type = find_type_in_env(*env, ast->data.AST_IDENTIFIER.value);
    break;
  }
  case AST_TYPE_DECL: {
    type = type_declaration(ast, env);
    break;
  }

  case AST_LIST: {
    Type *element_type = TRY_INFER(
        ast->data.AST_LIST.items, env,
        "Could not infer type of list literal elements (first element)");

    int len = ast->data.AST_LIST.len;
    if (len == 0) {
      Type *el_type = next_tvar();
      type = talloc(sizeof(Type));
      *type = (Type){T_CONS, {.T_CONS = {"List", &el_type, 1}}};
    }

    Type *el_type;
    for (int i = 1; i < len; i++) {
      Ast *el = ast->data.AST_LIST.items + i;
      el_type = TRY_INFER(el, env, "Failure typechecking list literal element");

      if (!types_equal(element_type, el_type)) {
        fprintf(stderr, "Error typechecking list literal - all elements must "
                        "be of the same type\n");
        print_type_err(element_type);
        fprintf(stderr, " != ");
        print_type_err(el_type);
        return NULL;
      }
    }
    type = talloc(sizeof(Type));
    *type = (Type){T_CONS, {.T_CONS = {"List", &el_type, 1}}};

    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;

    Type **cons_args = talloc(sizeof(Type *) * arity);
    for (int i = 0; i < arity; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = TRY_INFER(member, env, "Error typechecking tuple item");
      cons_args[i] = mtype;
    }
    type = talloc(sizeof(Type));
    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, cons_args, arity}}};

    break;
  }
  case AST_FMT_STRING: {
    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      infer(member, env);
    }
    type = &t_string;
    break;
  }
  case AST_LAMBDA: {
    type = infer_lambda(ast, env);
    break;
  }

  case AST_EXTERN_FN: {
    int param_count = ast->data.AST_EXTERN_FN.len - 1;
    Ast *ret_type_ast = ast->data.AST_EXTERN_FN.signature_types + param_count;
    Type *ret_type =
        find_type_in_env(*env, ret_type_ast->data.AST_IDENTIFIER.value);

    Type **param_types;
    if (param_count == 0) {
      param_types = talloc(sizeof(Type *));
      *param_types = &t_void;
    } else {
      param_types = talloc(param_count * sizeof(Type *));

      for (int i = 0; i < param_count; i++) {
        Ast *param_ast = ast->data.AST_EXTERN_FN.signature_types + i;

        Type *param_type =
            find_type_in_env(*env, param_ast->data.AST_IDENTIFIER.value);

        if (!param_type) {
          fprintf(stderr, "Error declaring extern function: type %s not found",
                  param_ast->data.AST_IDENTIFIER.value);
        }
        param_types[i] = param_type;
      }
    }

    Type *fn;
    if (param_count == 1 && ast->data.AST_LAMBDA.params->tag == AST_VOID) {
      fn = type_fn(&t_void, ret_type);
    } else {
      fn = create_type_multi_param_fn(param_count, param_types, ret_type);
    }
    type = fn;
    break;
  }
  case AST_APPLICATION: {
    Type *t = infer(ast->data.AST_APPLICATION.function, env);

    if (t == NULL) {
      fprintf(stderr, "%s\n", "Failure could not infer type of callee ");
      return NULL;
    }

    if (is_generic(t) && t->kind == T_CONS) {
      type = generic_cons(t, ast->data.AST_APPLICATION.len,
                          ast->data.AST_APPLICATION.args, env);
      break;
    }
    if (t->kind == T_VAR) {

      type = next_tvar();
      int len = ast->data.AST_APPLICATION.len;
      Type *arg_types[len];
      for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
        arg_types[i] = infer(ast->data.AST_APPLICATION.args + i, env);
      }
      Type *fn = create_type_multi_param_fn(len, arg_types, type);
      *t = *fn;

      type = _infer_fn_application(fn, arg_types, len, env);
      break;
    }
    if (t->kind == T_FN) {
      TypeEnv *_env = *env;
      type = infer_fn_application(ast, &_env);
      type = resolve_generic_type(type, _env);
      break;
    }

    break;
  }

  case AST_MATCH: {
    type = infer_match(ast, env);
    break;
  }
  }

  ast->md = type;

  return type;
}
