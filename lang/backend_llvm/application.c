#include "backend_llvm/application.h"
#include "adt.h"
#include "builtin_functions.h"
#include "closures.h"
#include "coroutines.h"
#include "function.h"
#include "modules.h"
#include "serde.h"
#include "symbols.h"
#include "types.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (to_type->kind == T_CONS && to_type->alias) {

    Ast id = (Ast){
        AST_IDENTIFIER,
        .data = {.AST_IDENTIFIER = {to_type->alias, strlen(to_type->alias)}}};

    // TODO: once all constructors are type symbol (ie first-class) we can add
    // them directly to the type instead of looking them up in the env
    JITSymbol *constructor_sym = lookup_id_ast(&id, ctx);

    if (constructor_sym && constructor_sym->type == STYPE_GENERIC_CONSTRUCTOR) {
      Type f =
          (Type){T_FN, .data = {.T_FN = {.from = from_type, .to = to_type}}};

      LLVMTypeRef fn_type = type_to_llvm_type(&f, ctx, module);
      LLVMValueRef cons_val = specific_fns_lookup(
          constructor_sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns,
          from_type);

      return LLVMBuildCall2(builder, fn_type, cons_val, (LLVMValueRef[]){val},
                            1, "constructor");
    }
  }

  if (!to_type->constructor) {
    return val;
  }

  ConsMethod constructor = to_type->constructor;

  return constructor(val, from_type, module, builder);
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

typedef struct ArgValList {
  LLVMValueRef val;
  LLVMTypeRef llvm_type;
  struct ArgValList *next;
} ArgValList;

LLVMValueRef call_callable_rec(int num_args_processed,
                               ArgValList *args_processed, Ast *ast,
                               Type *callable_type,
                               LLVMTypeRef llvm_callable_type,
                               LLVMValueRef callable, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len == 0) {
    LLVMValueRef arg_vals[num_args_processed];
    ArgValList *avl = args_processed;
    for (int i = num_args_processed - 1; i >= 0; i--, avl = avl->next) {
      LLVMValueRef val = avl->val;
      arg_vals[i] = val;
    }
    char name[32];
    if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
      snprintf(name, 32, "call.%s",
               ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);
    } else {
      sprintf(name, "call.record_member");
    }
    return LLVMBuildCall2(builder, llvm_callable_type, callable, arg_vals,
                          num_args_processed, name);
  }

  if (is_closure(callable_type)) {
    LLVMValueRef arg_vals[num_args_processed];
    ArgValList *avl = args_processed;
    for (int i = num_args_processed - 1; i >= 0; i--, avl = avl->next) {
      LLVMValueRef val = avl->val;
      arg_vals[i] = val;
    }

    LLVMValueRef resolved_closure_struct =
        LLVMBuildCall2(builder, llvm_callable_type, callable, arg_vals,
                       num_args_processed, "");

    LLVMValueRef closure_fn = LLVMBuildExtractValue(
        builder, resolved_closure_struct, 0, "closure_fn_impl");

    LLVMValueRef closure_env = LLVMBuildExtractValue(
        builder, resolved_closure_struct, 1, "closure_env");

    LLVMTypeRef llvm_rec_type = closure_record_type(callable_type, ctx, module);
    ArgValList argl = {.val = closure_env,
                       .llvm_type = LLVMPointerType(llvm_rec_type, 0),
                       .next = NULL};

    LLVMValueRef resolved_callable = closure_fn;
    Type _ct = *callable_type;
    _ct.closure_meta = NULL;

    return call_callable_rec(
        1, &argl, ast, &_ct,
        closure_fn_type(callable_type, llvm_rec_type, ctx, module),
        resolved_callable, ctx, module, builder);
  }

  Type *to_type = callable_type->data.T_FN.from;

  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *from_type = ast->data.AST_APPLICATION.args->md;
  from_type = deep_copy_type(from_type);
  from_type = resolve_type_in_env(from_type, ctx->env);

  val = handle_type_conversions(val, from_type, to_type, ctx, module, builder);

  ArgValList argl = {.val = val,
                     .llvm_type = type_to_llvm_type(to_type, ctx, module),
                     .next = args_processed};
  ast->data.AST_APPLICATION.args++;
  ast->data.AST_APPLICATION.len--;
  return call_callable_rec(num_args_processed + 1, &argl, ast,
                           callable_type->data.T_FN.to, llvm_callable_type,
                           callable, ctx, module, builder);
}

LLVMValueRef call_callable(Ast *ast, Type *callable_type, LLVMValueRef callable,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  if (!callable) {
    fprintf(stderr, "Error: callable not found for\n");
    print_location(ast);
    return NULL;
  }

  LLVMTypeRef llvm_callable_type =
      type_to_llvm_type(callable_type, ctx, module);

  Ast _ast = *ast;

  if (ast->data.AST_APPLICATION.args->tag == AST_VOID) {
    _ast.data.AST_APPLICATION.len = 0;
  }

  return call_callable_rec(0, NULL, &_ast, callable_type, llvm_callable_type,
                           callable, ctx, module, builder);
}

Type *resolve_sym_type(Type *exp, Type *sym_type, TypeEnv *env) {
  TICtx ctx = {};
  unify(exp, sym_type, &ctx);
  Subst *subst = solve_constraints(ctx.constraints);
  env = create_env_from_subst(env, subst);
  Type *res = deep_copy_type(sym_type);
  return resolve_type_in_env(res, env);
}

bool is_closure_symbol(JITSymbol *sym) { return is_closure(sym->symbol_type); }
LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  // TODO: this function is extraordinarily ugly - refactor to something a bit
  // more easy to read with logical sequence of cases

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->md;

  if (is_generic(expected_fn_type)) {
    expected_fn_type = deep_copy_type(expected_fn_type);
    expected_fn_type = resolve_type_in_env(expected_fn_type, ctx->env);
    Type *ex = expected_fn_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len;
         i++, ex = ex->data.T_FN.to) {
      if (ast->data.AST_APPLICATION.args[i].tag == AST_IDENTIFIER) {
        JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.args + i, ctx);
        if (sym->type == STYPE_FUNCTION &&
            is_closure(
                sym->symbol_type)) { // detect closure set in current scope if
                                     // can also be bound to a normal function
          ex->data.T_FN.from = sym->symbol_type;
        }
      }
    }
  }

  Type *res_type = ast->md;

  if (is_closure(res_type) && application_is_partial(ast)) {
    return codegen_create_closure(ast, ctx, module, builder);
  }

  LLVMValueRef callable;
  Type *callable_type = expected_fn_type;

  // x.mem a ??
  if (ast->data.AST_APPLICATION.function->tag == AST_RECORD_ACCESS &&
      !is_module_ast(
          ast->data.AST_APPLICATION.function->data.AST_RECORD_ACCESS.record)) {
    callable =
        codegen(ast->data.AST_APPLICATION.function, ctx, module, builder);

    return call_callable(ast, callable_type, callable, ctx, module, builder);
  }

  const char *sym_name =
      ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (!sym) {
    fprintf(stderr, "Error callable symbol %s not found in scope %d\n",
            sym_name, ctx->stack_ptr);
    return NULL;
  }

  if (is_closure_symbol(sym)) {
    return call_closure_sym(ast, callable_type, sym, ctx, module, builder);
  }

  if (is_sum_type(callable_type) && sym->type == STYPE_VARIANT_TYPE) {
    return codegen_adt_member_with_args(callable_type, sym->llvm_type, ast,
                                        sym_name, ctx, module, builder);
  }

  Type *symbol_type = sym->symbol_type;
  if (sym->type == STYPE_GENERIC_FUNCTION &&
      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {

    return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(
        ast, ctx, module, builder);
  }

  if (is_coroutine_constructor_type(symbol_type)) {
    LLVMValueRef instance_ptr =
        coro_create(sym, callable_type, ast, ctx, module, builder);
    return instance_ptr;
  } else if (is_coroutine_type(symbol_type)) {
    return coro_resume(sym, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_FUNCTION && !is_closure(sym->symbol_type)) {
    callable_type =
        resolve_sym_type(expected_fn_type, sym->symbol_type, ctx->env);

    callable = get_specific_callable(sym, callable_type, ctx, module, builder);

    return call_callable(ast, callable_type, callable, ctx, module, builder);
  }

  if (sym->type == STYPE_GENERIC_CONSTRUCTOR) {
    Type *from_type = ast->data.AST_APPLICATION.args->md;
    Type exp = (Type){
        T_FN, .data = {.T_FN = {.from = from_type, .to = expected_fn_type}}};
    callable_type = &exp;
    callable = get_specific_callable(sym, &exp, ctx, module, builder);
    return call_callable(ast, &exp, callable, ctx, module, builder);
  }

  if (sym->type == STYPE_FUNCTION) {
    callable_type = sym->symbol_type;

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);
    return res;
  }

  if (sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    callable_type = sym->symbol_type;
    callable = instantiate_extern_fn_sym(sym, ctx, module, builder);
    LLVMValueRef res =
        call_callable(ast, callable_type, callable, ctx, module, builder);
    return res;
  }

  return NULL;
}
