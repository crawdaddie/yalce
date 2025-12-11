#include "backend_llvm/application.h"
#include "./coroutines/coroutines.h"
#include "adt.h"
#include "closures.h"
#include "function.h"
#include "function_extern.h"
#include "modules.h"
#include "symbols.h"
#include "types.h"
#include "types/infer_application.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);
LLVMValueRef handle_constructor_module_conversion(
    LLVMValueRef val, JITSymbol *constructor_sym, Type *from_type,
    Type *to_type, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {

  Type *constructor_method_tscheme = NULL;

  int index;
  char *cons_method_name = find_constructor_method(
      constructor_sym->symbol_type, 1, (Type *[]){from_type}, &index,
      &constructor_method_tscheme);

  if (!cons_method_name) {
    fprintf(
        stderr,
        "Error: could not find constructor method for type conversion to %s",
        to_type->alias);
    return val;
  }

  JITSymbol *method =
      find_in_ctx(cons_method_name, strlen(cons_method_name),
                  constructor_sym->symbol_data.STYPE_MODULE.ctx);
  Type *exp_type = type_fn(from_type, to_type);

  LLVMValueRef callable = NULL;

  if (method->type == STYPE_GENERIC_FUNCTION) {
    callable = get_specific_callable(
        method, exp_type, constructor_sym->symbol_data.STYPE_MODULE.ctx, module,
        builder);
  } else if (method->type == STYPE_FUNCTION) {
    callable = method->val;
  }

  if (!callable) {
    fprintf(stderr, "Error: callable for constructor %s not found\n",
            cons_method_name);
    return NULL;
  }

  LLVMTypeRef fn_type = codegen_fn_type(exp_type, 1, ctx, module);

  return LLVMBuildCall2(builder, fn_type, callable, (LLVMValueRef[]){val}, 1,
                        "convert_arg_via_cons");
}

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  if (types_equal(from_type, to_type)) {
    return val;
  }

  if (is_pointer_type(to_type) && to_type->data.T_CONS.num_args == 1 &&
      types_equal(to_type->data.T_CONS.args[0], from_type)) {
    LLVMTypeRef lvft = type_to_llvm_type(from_type, ctx, module);
    LLVMValueRef alloca = LLVMBuildAlloca(builder, lvft, "tmp_alloca");
    LLVMBuildStore(builder, val, alloca);
    return alloca;
  }

  JITSymbol *constructor_sym = NULL;

  if (to_type->kind == T_CONS) {
    const char *name =
        to_type->alias ? to_type->alias : to_type->data.T_CONS.name;

    constructor_sym = find_in_ctx(name, strlen(name), ctx);

    if (constructor_sym && constructor_sym->type == STYPE_MODULE) {
      return handle_constructor_module_conversion(
          val, constructor_sym, from_type, to_type, ctx, module, builder);
    }
  }

  if (to_type->constructor) {
    ConsMethod constructor = to_type->constructor;
    return constructor(val, from_type, module, builder);
  }

  return val;
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

  // Check if callable is already a closure struct value (not a function
  // returning a closure) This happens when we have a closure bound to a
  // variable and we're calling it
  if (is_closure(callable_type) && num_args_processed == 0 &&
      LLVMGetTypeKind(LLVMTypeOf(callable)) == LLVMStructTypeKind) {
    // callable is the closure struct { ptr, ptr }
    // Extract function and environment, then continue processing
    LLVMValueRef closure_fn =
        LLVMBuildExtractValue(builder, callable, 0, "closure_fn_value");

    LLVMValueRef closure_env =
        LLVMBuildExtractValue(builder, callable, 1, "closure_env_value");

    LLVMTypeRef llvm_rec_type = closure_record_type(callable_type, ctx, module);
    ArgValList argl = {.val = closure_env,
                       .llvm_type = LLVMPointerType(llvm_rec_type, 0),
                       .next = NULL};

    Type _ct = *callable_type;
    _ct.closure_meta = NULL;

    return call_callable_rec(
        1, &argl, ast, &_ct,
        closure_fn_type(callable_type, llvm_rec_type, ctx, module), closure_fn,
        ctx, module, builder);
  }

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

    LLVMValueRef x = LLVMBuildCall2(builder, llvm_callable_type, callable,
                                    arg_vals, num_args_processed, name);
    return x;
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

  Type *from_type = ast->data.AST_APPLICATION.args->type;
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
    print_ast_err(ast->data.AST_APPLICATION.function);
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

bool is_closure_symbol(JITSymbol *sym) {
  return sym->symbol_type && is_closure(sym->symbol_type);
}

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  // TODO: this function is extraordinarily ugly - refactor to something a bit
  // more easy to read with logical sequence of cases

  Type *expected_fn_type = ast->data.AST_APPLICATION.function->type;

  if (is_generic(expected_fn_type)) {
    expected_fn_type = deep_copy_type(expected_fn_type);
    expected_fn_type = resolve_type_in_env(expected_fn_type, ctx->env);
    Type *ex = expected_fn_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len;
         i++, ex = ex->data.T_FN.to) {
      if (ast->data.AST_APPLICATION.args[i].tag == AST_IDENTIFIER) {
        JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.args + i, ctx);
        if (sym && sym->type == STYPE_FUNCTION &&
            is_closure(
                sym->symbol_type)) { // detect closure set in current scope if
                                     // can also be bound to a normal function
          ex->data.T_FN.from = sym->symbol_type;
        }
      }
    }
  }

  Type *res_type = ast->type;

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

  const char *sym_name;
  if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    sym_name = ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
  } else if (ast->data.AST_APPLICATION.function->tag == AST_RECORD_ACCESS) {
    Ast *id = ast->data.AST_APPLICATION.function;
    while (id->tag == AST_RECORD_ACCESS) {
      id = id->data.AST_RECORD_ACCESS.member;
    }

    sym_name = id->data.AST_IDENTIFIER.value;

  } else {
    sym_name = "";
  }

  JITSymbol *sym = lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

  if (!sym) {

    fprintf(stderr, "Error callable symbol %s not found in scope %d\n",
            sym_name, ctx->stack_ptr);
    print_location(ast->data.AST_APPLICATION.function);
    return NULL;
  }

  if (is_closure_symbol(sym)) {
    return call_closure_sym(ast, callable_type, sym, ctx, module, builder);
  }

  if (is_sum_type(callable_type) && sym->type == STYPE_VARIANT_TYPE) {
    return codegen_adt_member_with_args(callable_type, sym->llvm_type, ast,
                                        sym_name, ctx, module, builder);
  }

  if (sym->type == STYPE_VARIANT_TYPE) {
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
    Type *from_type = ast->data.AST_APPLICATION.args->type;
    Type exp = (Type){
        T_FN, .data = {.T_FN = {.from = from_type, .to = expected_fn_type}}};
    callable_type = &exp;
    callable = get_specific_callable(sym, &exp, ctx, module, builder);
    return call_callable(ast, &exp, callable, ctx, module, builder);
  }

  // if (sym->type == STYPE_MODULE) {
  //   printf("use constructor module\n");
  //   print_ast(ast);
  //   // sym->symbol_data.STYPE_MODULE.
  //   return NULL;
  // }

  if (sym->type == STYPE_FUNCTION) {
    callable_type = sym->symbol_type;

    LLVMValueRef res =
        call_callable(ast, callable_type, sym->val, ctx, module, builder);
    return res;
  }

  if (sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    callable_type = sym->symbol_type;
    callable = instantiate_extern_fn_sym(sym, ctx, module, builder);
    // printf("\n\nAPPLICATION\n");
    // print_ast(ast);
    // LLVMDumpValue(callable);
    // print_type(callable_type);
    LLVMValueRef res =
        call_callable(ast, callable_type, callable, ctx, module, builder);
    return res;
  }

  return NULL;
}
