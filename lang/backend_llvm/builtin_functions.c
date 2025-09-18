#include "backend_llvm/builtin_functions.h"
#include "adt.h"
#include "application.h"
#include "backend_llvm/array.h"
#include "backend_llvm/common.h"
#include "backend_llvm/coroutine_extensions.h"
#include "backend_llvm/coroutine_scheduling.h"
#include "common.h"
#include "coroutines.h"
#include "function.h"
#include "input.h"
#include "list.h"
#include "module.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "types/builtins.h"
#include "types/common.h"
#include "types/inference.h"
#include "util.h"
#include <dlfcn.h>
#include <llvm-c/Core.h>
#include <stdlib.h>
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef create_constructor_methods(Ast *trait, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  const char *name = trait->data.AST_TRAIT_IMPL.type.chars;

  Type *out_type = env_lookup(ctx->env, name);

  Type *constructor_type = type_fn(tvar("cons.Input"), out_type);

  JITSymbol *constructor_sym =
      new_symbol(STYPE_GENERIC_CONSTRUCTOR, constructor_type, NULL, NULL);

  Ast *impl = trait->data.AST_TRAIT_IMPL.impl;

  if (impl->tag == AST_MODULE) {
    // print_ast(impl);

    if (impl->data.AST_LAMBDA.body->tag != AST_BODY) {
      Ast *expr = impl->data.AST_LAMBDA.body;
      LLVMValueRef func = codegen(expr, ctx, module, builder);
      constructor_sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
          specific_fns_extend(
              constructor_sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns,
              expr->md, func);
    } else {

      AST_LIST_ITER(
          impl->data.AST_LAMBDA.body->data.AST_BODY.stmts, ({
            Ast *expr = l->ast;
            LLVMValueRef func = codegen(expr, ctx, module, builder);
            constructor_sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns =
                specific_fns_extend(constructor_sym->symbol_data
                                        .STYPE_GENERIC_FUNCTION.specific_fns,
                                    expr->md, func);
          }));
    }
  }
  uint64_t hash_id = trait->data.AST_TRAIT_IMPL.type.hash;
  ht_set_hash(ctx->frame->table, name, hash_id, constructor_sym);
  out_type->constructor = constructor_sym;
  return NULL;
}

LLVMValueRef create_arithmetic_typeclass_methods(Ast *trait, JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder) {
  Ast *impl = trait->data.AST_TRAIT_IMPL.impl;

  if (impl->tag != AST_MODULE) {
    fprintf(stderr, "Arithmetic trait for %s not correctly implemented\n",
            trait->data.AST_TRAIT_IMPL.type);
    return NULL;
  }

  ObjString type = trait->data.AST_TRAIT_IMPL.type;

  AST_LIST_ITER(impl->data.AST_LAMBDA.body->data.AST_BODY.stmts, ({
                  Ast *stmt = l->ast;

                  if (stmt->tag != AST_LET) {
                    continue;
                  }
                  Ast *expr = stmt->data.AST_LET.expr;
                  Ast *binding = stmt->data.AST_LET.binding;
                  const char *name = binding->data.AST_IDENTIFIER.value;
                  LLVMValueRef func = codegen(expr, ctx, module, builder);

                  int total_chars = strlen(type.chars) + 1 + 1;
                  char chars[total_chars];
                  if (CHARS_EQ(name, "add")) {
                    sprintf(chars, "%s.%s", type.chars, "+");
                  } else if (CHARS_EQ(name, "sub")) {
                    sprintf(chars, "%s.%s", type.chars, "-");
                  } else if (CHARS_EQ(name, "mul")) {
                    sprintf(chars, "%s.%s", type.chars, "*");
                  } else if (CHARS_EQ(name, "div")) {
                    sprintf(chars, "%s.%s", type.chars, "/");
                  } else if (CHARS_EQ(name, "mod")) {
                    sprintf(chars, "%s.%s", type.chars, "%");
                  }

                  JITSymbol *method_sym =
                      new_symbol(STYPE_FUNCTION, expr->md, func,
                                 type_to_llvm_type(expr->md, ctx, module));

                  ht_set_hash(ctx->frame->table, chars,
                              hash_string(chars, total_chars), method_sym);
                }));

  return NULL;
}

JITSymbol *get_typeclass_method(char *type_name, char *op, JITLangCtx *ctx) {
  int total_chars = strlen(type_name) + strlen(op) + 1;
  char chars[total_chars];
  sprintf(chars, "%s.%s", type_name, op);
  JITSymbol *sym = find_in_ctx(chars, total_chars, ctx);
  if (!sym) {
    fprintf(stderr, "symbol %s not found\n", chars);
    return NULL;
  }
  return sym;
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

Type *find_in_env_if_generic(Type *t, TypeEnv *env) {
  if (t->kind == T_VAR) {
    Type *l = env_lookup(env, t->data.T_VAR);
    if (l && t->kind != T_VAR) {
      return l;
    }
    if (l && t->kind == T_VAR) {
      return find_in_env_if_generic(l, env);
    }
  }
  return t;
}

#define ARITHMETIC_BINOP(_name, _flop, _iop)                                   \
  ({                                                                           \
    Type *ret = fn_return_type(fn_type);                                       \
    ret = find_in_env_if_generic(ret, ctx->env);                               \
    switch (ret->kind) {                                                       \
    case T_INT:                                                                \
    case T_UINT64: {                                                           \
      LLVMValueRef l =                                                         \
          codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);       \
      l = handle_type_conversions(l, lt, ret, ctx, module, builder);           \
      LLVMValueRef r =                                                         \
          codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);   \
      r = handle_type_conversions(r, rt, ret, ctx, module, builder);           \
      return LLVMBuildBinOp(builder, _iop, l, r, _name "_int");                \
    }                                                                          \
    case T_NUM: {                                                              \
      LLVMValueRef l =                                                         \
          codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);       \
      l = handle_type_conversions(l, lt, ret, ctx, module, builder);           \
      LLVMValueRef r =                                                         \
          codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);   \
      r = handle_type_conversions(r, rt, ret, ctx, module, builder);           \
      return LLVMBuildBinOp(builder, _flop, l, r, _name "_num");               \
    }                                                                          \
    default: {                                                                 \
      if (ret->alias) {                                                        \
        JITSymbol *sym = get_typeclass_method(ret->alias, _name, ctx);         \
        if (!sym) {                                                            \
          return NULL;                                                         \
        }                                                                      \
        if (sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler) {         \
          return sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler(      \
              ast, ctx, module, builder);                                      \
        }                                                                      \
        if (sym->type == STYPE_FUNCTION) {                                     \
          return call_callable(ast, sym->symbol_type, sym->val, ctx, module,   \
                               builder);                                       \
        }                                                                      \
      }                                                                        \
      return NULL;                                                             \
    }                                                                          \
    }                                                                          \
  })

LLVMValueRef curried_binop(Ast *saved_arg_ast, LLVMOpcode fop, LLVMOpcode iop,
                           Type *type, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  Type *ret = type->data.T_FN.to;
  LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(ret, ctx, module);

  LLVMTypeRef llvm_from_type_ref =
      type_to_llvm_type(type->data.T_FN.from, ctx, module);

  LLVMTypeRef curried_fn_type = LLVMFunctionType(
      llvm_return_type_ref, (LLVMTypeRef[]){llvm_from_type_ref}, 1, 0);

  START_FUNC(module, "anon_curried_value", curried_fn_type);

  LLVMValueRef saved_arg = codegen(saved_arg_ast, ctx, module, builder);
  LLVMValueRef free_arg = LLVMGetParam(func, 0);
  LLVMValueRef binop_res;

  switch (ret->kind) {
  case T_INT:
  case T_UINT64: {
    binop_res =
        LLVMBuildBinOp(builder, iop, saved_arg, free_arg, "curried_binop");
    break;
  }
  case T_NUM: {
    binop_res =
        LLVMBuildBinOp(builder, fop, saved_arg, free_arg, "curried_binop");
    break;
  }
  default: {
    return NULL;
  }
  }

  LLVMBuildRet(builder, binop_res);

  END_FUNC
  return func;
}

LLVMValueRef SumHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFAdd, LLVMAdd,
                         ast->md, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  // print_type_env(ctx->env);
  // print_type(fn_type);
  // Type *lt = ast->data.AST_APPLICATION.args[0].md;
  // Type *rt = ast->data.AST_APPLICATION.args[1].md;
  // print_type(lt);
  // print_type(rt);
  // fn_type->data.T_FN.to->data.T_FN.from;

  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  ARITHMETIC_BINOP("+", LLVMFAdd, LLVMAdd);
}

LLVMValueRef MinusHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFSub, LLVMSub,
                         ast->md, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("-", LLVMFSub, LLVMSub);
}

LLVMValueRef MulHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFMul, LLVMMul,
                         ast->md, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("*", LLVMFMul, LLVMMul);
}

LLVMValueRef DivHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFDiv, LLVMSDiv,
                         ast->md, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("/", LLVMFDiv, LLVMSDiv);
}

LLVMValueRef ModHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFRem, LLVMSRem,
                         ast->md, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;

  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("%", LLVMFRem, LLVMSRem);
}

#define ORD_BINOP(_name, _flop, _iop)                                          \
  ({                                                                           \
    Type *target_type;                                                         \
    if (get_typeclass_rank(lt, "ord") >= get_typeclass_rank(rt, "ord")) {      \
      target_type = lt;                                                        \
    } else {                                                                   \
      target_type = rt;                                                        \
    }                                                                          \
    LLVMValueRef l =                                                           \
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);         \
    l = handle_type_conversions(l, lt, target_type, ctx, module, builder);     \
    LLVMValueRef r =                                                           \
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);     \
    r = handle_type_conversions(r, rt, target_type, ctx, module, builder);     \
    switch (target_type->kind) {                                               \
    case T_INT:                                                                \
    case T_UINT64: {                                                           \
      return LLVMBuildICmp(builder, _iop, l, r, _name "_int");                 \
    }                                                                          \
    case T_NUM: {                                                              \
      return LLVMBuildFCmp(builder, _flop, l, r, _name "_num");                \
    }                                                                          \
    default: {                                                                 \
      fprintf(stderr, "Error: unrecognized operands for ord binop");           \
      return NULL;                                                             \
    }                                                                          \
    }                                                                          \
  })

LLVMValueRef GtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    fprintf(stderr,
            "Not Implemented error: currying > binop - eg ((>) 1)\n%s:%d\n",
            __FILE__, __LINE__);
    return NULL;
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  ORD_BINOP(">", LLVMRealOGT, LLVMIntSGT);
}
LLVMValueRef GteHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    fprintf(stderr,
            "Not Implemented error: currying > binop - eg ((>=) 1)\n%s:%d\n",
            __FILE__, __LINE__);
    return NULL;
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to;
  ORD_BINOP(">=", LLVMRealOGE, LLVMIntSGE);
}

LLVMValueRef LtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    fprintf(stderr,
            "Not Implemented error: currying < binop - eg ((<) 1)\n%s:%d\n",
            __FILE__, __LINE__);
    return NULL;
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;

  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  ORD_BINOP("<", LLVMRealOLT, LLVMIntSLT);
}

LLVMValueRef LteHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    fprintf(stderr,
            "Not Implemented error: currying <= binop - eg ((<=) 1)\n%s:%d\n",
            __FILE__, __LINE__);
    return NULL;
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  fn_type = resolve_type_in_env_mut(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;

  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ORD_BINOP("<=", LLVMRealOLE, LLVMIntSLE);
}

LLVMValueRef _codegen_equality(Type *type, LLVMValueRef l, LLVMValueRef r,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef array_eq(LLVMValueRef arr1, LLVMValueRef arr2, Type *arr_type,
                      JITLangCtx *ctx, LLVMModuleRef module,
                      LLVMBuilderRef builder) {

  // Get the element type from the array type
  LLVMTypeRef element_type = type_to_llvm_type(arr_type, ctx, module);

  // Get sizes of both arrays
  LLVMValueRef size1 = codegen_get_array_size(builder, arr1, element_type);
  LLVMValueRef size2 = codegen_get_array_size(builder, arr2, element_type);

  // Compare sizes
  LLVMValueRef sizes_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, size1, size2, "sizes_equal");

  // Get data pointers from both arrays
  LLVMValueRef data1 =
      LLVMBuildExtractValue(builder, arr1, 1, "get_array1_data_ptr");
  LLVMValueRef data2 =
      LLVMBuildExtractValue(builder, arr2, 1, "get_array2_data_ptr");

  // Get or declare strncmp function
  LLVMTypeRef strncmp_args[] = {
      LLVMPointerType(LLVMInt8Type(), 0), // const char*
      LLVMPointerType(LLVMInt8Type(), 0), // const char*
      LLVMInt32Type()                     // size_t (using i32)
  };
  LLVMTypeRef strncmp_type =
      LLVMFunctionType(LLVMInt32Type(), strncmp_args, 3, 0);
  LLVMValueRef strncmp_func = LLVMGetNamedFunction(module, "strncmp");
  if (!strncmp_func) {
    strncmp_func = LLVMAddFunction(module, "strncmp", strncmp_type);
  }

  // Cast data pointers to i8* for strncmp
  LLVMValueRef data1_i8 = LLVMBuildPointerCast(
      builder, data1, LLVMPointerType(LLVMInt8Type(), 0), "data1_i8");
  LLVMValueRef data2_i8 = LLVMBuildPointerCast(
      builder, data2, LLVMPointerType(LLVMInt8Type(), 0), "data2_i8");

  // Call strncmp(data1, data2, size1)
  LLVMValueRef strncmp_args_vals[] = {data1_i8, data2_i8, size1};
  LLVMValueRef strncmp_result =
      LLVMBuildCall2(builder, strncmp_type, strncmp_func, strncmp_args_vals, 3,
                     "strncmp_result");

  // Check if strncmp returned 0 (strings are equal)
  LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
  LLVMValueRef contents_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, strncmp_result, zero, "contents_equal");

  // AND the size equality with content equality
  LLVMValueRef final_result =
      LLVMBuildAnd(builder, sizes_equal, contents_equal, "arrays_equal");

  return final_result;
}

LLVMValueRef cons_equality(Type *type, LLVMValueRef tuple1, LLVMValueRef tuple2,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMTypeRef tuple_type = type_to_llvm_type(type, ctx, module);
  if (is_array_type(type)) {
    return array_eq(tuple1, tuple2, type, ctx, module, builder);
  }

  LLVMTypeRef type1 = LLVMTypeOf(tuple1);
  LLVMTypeRef type2 = LLVMTypeOf(tuple2);

  // Create basic blocks for the comparison loop
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);

  LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(function, "tuple_eq_end");
  LLVMValueRef result_phi = LLVMBuildPhi(builder, LLVMInt1Type(), "eq_result");

  // Initialize result to true
  LLVMValueRef is_equal = LLVMConstInt(LLVMInt1Type(), 1, 0); // true
  unsigned element_count = type->data.T_CONS.num_args;

  for (unsigned i = 0; i < element_count; i++) {

    // Extract elements to compare
    LLVMValueRef val1 = codegen_tuple_access(i, tuple1, tuple_type, builder);
    LLVMValueRef val2 = codegen_tuple_access(i, tuple2, tuple_type, builder);
    Type *el_type = type->data.T_CONS.args[i];

    // Compare the current elements
    LLVMValueRef elem_eq =
        _codegen_equality(el_type, val1, val2, ctx, module, builder);

    // Create a new block for the next comparison (if needed)
    LLVMBasicBlockRef next_block = NULL;
    if (i < element_count - 1) {
      next_block = LLVMAppendBasicBlock(function, "tuple_eq_next");
    }

    // If elements are not equal, short-circuit to end with false
    LLVMBuildCondBr(builder, elem_eq, (next_block ? next_block : end_block),
                    end_block);

    // Update is_equal for phi node
    is_equal = LLVMBuildAnd(builder, is_equal, elem_eq, "running_eq");

    if (next_block) {
      LLVMPositionBuilderAtEnd(builder, next_block);
    }
  }

  // Position builder at end block for phi node
  LLVMPositionBuilderAtEnd(builder, end_block);

  // Add phi node incoming values
  LLVMAddIncoming(result_phi, &is_equal, &current_block, 1);

  return result_phi;
}

LLVMValueRef option_eq(Type *type, LLVMValueRef l, LLVMValueRef r,
                       JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  Type *t = type_of_option(type);
  if (is_generic(t)) {
    *t = t_int;
  }

  LLVMValueRef tag1 =
      LLVMBuildExtractValue(builder, l, 0, "option_eq_get_tag_l");
  LLVMValueRef tag2 =
      LLVMBuildExtractValue(builder, r, 0, "option_eq_get_tag_r");
  LLVMValueRef phi = LLVM_IF_ELSE(
      builder, codegen_option_is_none(r, builder),
      LLVMBuildICmp(builder, LLVMIntEQ, tag1, tag2, "none-type-tags-equal"),
      _codegen_equality(
          t, LLVMBuildExtractValue(builder, l, 1, "option_eq_get_val_l"),
          LLVMBuildExtractValue(builder, r, 1, "option_eq_get_val_r"), ctx,
          module, builder));

  return phi;
}

#define MUT_VAL(_llvm_type, _val)                                              \
  ({                                                                           \
    LLVMValueRef mut_val =                                                     \
        LLVMBuildAlloca(builder, _llvm_type, "mut_val_alloca");                \
    LLVMBuildStore(builder, _val, mut_val);                                    \
    mut_val;                                                                   \
  })
LLVMValueRef list_eq(Type *type, LLVMValueRef l, LLVMValueRef r,
                     JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  Type *el_type = type->data.T_CONS.args[0];
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx, module);
  LLVMTypeRef llvm_list_node_type = llnode_type(llvm_el_type);

  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef cond_block =
      LLVMAppendBasicBlock(current_function, "loop.cond");
  LLVMBasicBlockRef body_block =
      LLVMAppendBasicBlock(current_function, "loop.body");
  LLVMBasicBlockRef inc_block =
      LLVMAppendBasicBlock(current_function, "loop.inc");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlock(current_function, "loop.after");

  LLVMValueRef is_eq_alloca =
      MUT_VAL(LLVMInt1Type(), LLVMConstInt(LLVMInt1Type(), 1, 0));

  LLVMValueRef l_iterator = MUT_VAL(LLVMPointerType(llvm_list_node_type, 0), l);
  LLVMValueRef r_iterator = MUT_VAL(LLVMPointerType(llvm_list_node_type, 0), r);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);

  LLVMValueRef l_current =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0),
                     l_iterator, "l_current");
  LLVMValueRef r_current =
      LLVMBuildLoad2(builder, LLVMPointerType(llvm_list_node_type, 0),
                     r_iterator, "r_current");

  LLVMValueRef l_is_null = ll_is_null(l_current, llvm_el_type, builder);
  LLVMValueRef r_is_null = ll_is_null(r_current, llvm_el_type, builder);

  LLVMValueRef both_null =
      LLVMBuildAnd(builder, l_is_null, r_is_null, "both_null");

  LLVMBuildCondBr(builder, both_null, after_block, body_block);

  LLVMPositionBuilderAtEnd(builder, body_block);

  LLVMValueRef one_null =
      LLVMBuildXor(builder, l_is_null, r_is_null, "one_null");

  LLVMBasicBlockRef compare_elements_block =
      LLVMAppendBasicBlock(current_function, "compare_elements");
  LLVMBasicBlockRef set_false_block =
      LLVMAppendBasicBlock(current_function, "set_false");

  LLVMBuildCondBr(builder, one_null, set_false_block, compare_elements_block);

  LLVMPositionBuilderAtEnd(builder, set_false_block);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_eq_alloca);
  LLVMBuildBr(builder, after_block);

  LLVMPositionBuilderAtEnd(builder, compare_elements_block);

  LLVMValueRef l_data_ptr = LLVMBuildStructGEP2(builder, llvm_list_node_type,
                                                l_current, 0, "l_data_ptr");

  LLVMValueRef r_data_ptr = LLVMBuildStructGEP2(builder, llvm_list_node_type,
                                                r_current, 0, "r_data_ptr");

  LLVMValueRef l_data = ll_get_head_val(l_current, llvm_el_type, builder);
  LLVMValueRef r_data = ll_get_head_val(r_current, llvm_el_type, builder);
  // INSERT_PRINTF(2, "compare %d %d\n", l_data, r_data);

  LLVMValueRef elements_equal =
      _codegen_equality(el_type, l_data, r_data, ctx, module, builder);

  LLVMValueRef current_eq =
      LLVMBuildLoad2(builder, LLVMInt1Type(), is_eq_alloca, "current_eq");
  LLVMValueRef new_eq =
      LLVMBuildAnd(builder, current_eq, elements_equal, "new_eq");
  LLVMBuildStore(builder, new_eq, is_eq_alloca);

  LLVMBuildCondBr(builder, elements_equal, inc_block, after_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);

  LLVMValueRef l_next = ll_get_next(l_current, llvm_el_type, builder);

  LLVMValueRef r_next = ll_get_next(r_current, llvm_el_type, builder);

  LLVMBuildStore(builder, l_next, l_iterator);
  LLVMBuildStore(builder, r_next, r_iterator);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  return LLVMBuildLoad2(builder, LLVMInt1Type(), is_eq_alloca, "list_els_eq");
}

LLVMValueRef _codegen_equality(Type *type, LLVMValueRef l, LLVMValueRef r,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  switch (type->kind) {
  case T_BOOL:
  case T_INT:
  case T_UINT64: {
    return LLVMBuildICmp(builder, LLVMIntEQ, l, r, "eq_int");
  }
  case T_CHAR: {
    return LLVMBuildICmp(builder, LLVMIntEQ, l, r, "eq_char");
  }
  case T_NUM: {
    return LLVMBuildFCmp(builder, LLVMRealOEQ, l, r, "eq_num");
  }
  case T_CONS: {

    if ((strcmp(type->data.T_CONS.name, TYPE_NAME_VARIANT) == 0) &&
        (type->data.T_CONS.num_args == 2) &&
        (strcmp(type->data.T_CONS.args[0]->data.T_CONS.name, "Some") == 0) &&
        (strcmp(type->data.T_CONS.args[1]->data.T_CONS.name, "None") == 0)) {
      return option_eq(type, l, r, ctx, module, builder);
    }

    if (is_option_type(type)) {
      return option_eq(type, l, r, ctx, module, builder);
    }

    if (is_list_type(type)) {
      return list_eq(type, l, r, ctx, module, builder);
    }

    return cons_equality(type, l, r, ctx, module, builder);
  }
  }

  return NULL;
}

LLVMValueRef EqAppHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *fn_type = ast->data.AST_APPLICATION.function->md;
  Type *lt = fn_type->data.T_FN.from;

  lt = resolve_type_in_env_mut(lt, ctx->env);
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  rt = resolve_type_in_env_mut(rt, ctx->env);
  Type *target_type;

  if (types_equal(lt, rt)) {
    target_type = lt;
  } else if (get_typeclass_rank(lt, "eq") >= get_typeclass_rank(rt, "eq")) {
    target_type = lt;
  } else {
    target_type = rt;
  }

  LLVMValueRef l =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  l = handle_type_conversions(l, lt, target_type, ctx, module, builder);
  LLVMValueRef r =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  r = handle_type_conversions(r, rt, target_type, ctx, module, builder);

  return _codegen_equality(target_type, l, r, ctx, module, builder);
}

LLVMValueRef CharHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  return NULL;
}

LLVMValueRef NeqHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  Type *fn_type = ast->data.AST_APPLICATION.function->md;
  Type *lt = fn_type->data.T_FN.from;
  lt = resolve_type_in_env_mut(lt, ctx->env);
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  rt = resolve_type_in_env_mut(rt, ctx->env);

  Type *target_type;
  if (get_typeclass_rank(lt, "eq") >= get_typeclass_rank(rt, "eq")) {
    target_type = lt;
  } else {
    target_type = rt;
  }
  LLVMValueRef l =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  l = handle_type_conversions(l, lt, target_type, ctx, module, builder);
  LLVMValueRef r =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  r = handle_type_conversions(r, rt, target_type, ctx, module, builder);

  return LLVMBuildNot(
      builder, _codegen_equality(target_type, l, r, ctx, module, builder),
      "not_eq");
}

LLVMValueRef double_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_NUM: {
    return val;
  }

  case T_INT: {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(),
                           "cast_int_to_double");
  }

  case T_UINT64: {
    return LLVMBuildUIToFP(builder, val, LLVMDoubleType(),
                           "cast_uint64_to_double");
  }

  default:
    return NULL;
  }
}

LLVMValueRef int_constructor(LLVMValueRef val, Type *from_type,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_NUM: {
    return LLVMBuildFPToSI(builder, val, LLVMInt32Type(), "cast_double_to_int");
  }

  case T_INT: {
    return val;
  }

  case T_UINT64: {
    // return LLVMBuildUIToFP(builder, val, LLVMDoubleType(),
    //                        "cast_uint64_to_double");
    return NULL;
  }

  default:
    return NULL;
  }
}

LLVMValueRef int_constructor_handler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  return int_constructor(
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder), &t_int,
      module, builder);
}
LLVMValueRef double_constructor_handler(Ast *ast, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  return double_constructor(
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder), &t_int,
      module, builder);
}

LLVMValueRef uint64_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_INT: {
    LLVMTypeRef uint64Type = LLVMInt64Type();

    // Perform zero extension to convert i32 to i64
    // Zero extension is appropriate for unsigned integers
    LLVMValueRef ext = LLVMBuildZExt(builder, val, uint64Type, "extended");
    return ext;
  }

  default:
    return NULL;
  }
}

LLVMValueRef ArraySizeHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  Ast *array_ast = ast->data.AST_APPLICATION.args;

  Type *arr_type = array_ast->md;
  Type *el_type = arr_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx, module);

  LLVMValueRef array = codegen(array_ast, ctx, module, builder);

  return codegen_get_array_size(builder, array, llvm_el_type);
}

LLVMValueRef ArrayAtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  Type *ret_type = ast->md;
  Ast *array_ast = ast->data.AST_APPLICATION.args;
  Ast *idx_ast = ast->data.AST_APPLICATION.args + 1;
  LLVMValueRef array = codegen(array_ast, ctx, module, builder);
  LLVMValueRef idx = codegen(idx_ast, ctx, module, builder);

  if (ret_type->kind == T_FN) {
    return get_array_element(builder, array, idx, GENERIC_PTR);
  }

  LLVMTypeRef el_type = type_to_llvm_type(ret_type, ctx, module);

  if (!el_type) {
    return NULL;
  }
  return get_array_element(builder, array, idx, el_type);
}

LLVMValueRef ArraySetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  Type *ret_type = ast->md;
  Ast *idx_ast = ast->data.AST_APPLICATION.args + 1;
  Ast *array_ast = ast->data.AST_APPLICATION.args;
  Ast *val_ast = ast->data.AST_APPLICATION.args + 2;
  LLVMValueRef array = codegen(array_ast, ctx, module, builder);
  LLVMValueRef idx = codegen(idx_ast, ctx, module, builder);
  LLVMValueRef val = codegen(val_ast, ctx, module, builder);
  // printf("array set find el type\n");
  // print_ast(val_ast);
  // print_type(ret_type->data.T_CONS.args[0]);
  Type *el_type = ret_type->data.T_CONS.args[0];

  return set_array_element(builder, array, idx, val,
                           el_type->kind == T_FN
                               ? GENERIC_PTR
                               : type_to_llvm_type(el_type, ctx, module));
}

LLVMValueRef SomeConsHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  Ast *contained_ast = ast->data.AST_APPLICATION.args;
  LLVMValueRef contained = codegen(contained_ast, ctx, module, builder);

  LLVMValueRef res = codegen_some(contained, builder);
  return res;
}

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)

#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

// short-circuiting and-operator
LLVMValueRef LogicalAndHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");
  LLVMValueRef arg1 =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMBuildCondBr(builder, arg1, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_result =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef else_result = _FALSE;
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");
  LLVMValueRef incoming_vals[] = {then_result, else_result};
  LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

  return phi;
}

// short-circuiting or-operator
LLVMValueRef LogicalOrHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

  LLVMValueRef arg1 =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMBuildCondBr(builder, arg1, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_result = _TRUE; // Short circuit with true
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef else_result =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");
  LLVMValueRef incoming_vals[] = {then_result, else_result};
  LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

  return phi;
}

Type *lookup_var_in_env(TypeEnv *env, Type *tvar) {
  if (tvar->kind == T_VAR) {
    while (tvar->kind == T_VAR) {
      tvar = env_lookup(env, tvar->data.T_VAR);
    }
  }
  return tvar;
}

LLVMValueRef FstHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  // Type *t = ast->md;
  // print_ast()
  // LLVMTypeRef llvm_type = type_to_llvm_type(t, ctx->env, module);
  // LLVMDumpType(llvm_type);
  // printf("\n");
  //
  // LLVMValueRef val =
  //     codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  //
  // return val;
  return NULL;
}

LLVMValueRef uint64_constructor_handler(Ast *ast, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  LLVMValueRef in =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  return uint64_constructor(in, ast->data.AST_APPLICATION.args->md, module,
                            builder);
}

LLVMValueRef char_cons_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  Type *from_type = ast->data.AST_APPLICATION.args->md;

  switch (from_type->kind) {
  case T_INT: {
    LLVMTypeRef char_type = LLVMInt8Type();

    LLVMValueRef ext =
        LLVMBuildTrunc(builder, val, char_type, "trunc_int_to_char");
    return ext;
  }

  default:
    return NULL;
  }
}
LLVMValueRef RefHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  Ast *arg = ast->data.AST_APPLICATION.args;
  Type *t = ast->md;
  LLVMValueRef val = codegen(arg, ctx, module, builder);
  return val;
}

LLVMValueRef SerializeBlobHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  // print_ast(ast);
  printf("path %s\n", ast->data.AST_APPLICATION.args->data.AST_STRING.value);
  print_type(ast->data.AST_APPLICATION.args[1].md);

  return NULL;
}

LLVMValueRef DlOpenHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  const char *path = ast->data.AST_APPLICATION.args->data.AST_STRING.value;
  const char *full_path;
  if (module_path == NULL) {
    full_path = path;
  } else {
    const char *_module_path = get_dirname(module_path);

    while (strncmp(path, "../", 3) == 0) {
      path = path + 3;
      _module_path = get_dirname(_module_path);
    }

    full_path = malloc(strlen(path) + strlen(_module_path) + 1);
    sprintf(full_path, "%s/%s", _module_path, path);
  }

  void *handle = dlopen(full_path, RTLD_GLOBAL | RTLD_LAZY);

  if (!handle) {
    fprintf(stderr, "Failed to load library globally: %s\n", dlerror());
    free(full_path);
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }
  fprintf(stderr, "loaded %s\n", full_path);

  free(full_path);
  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}

// LLVMValueRef DFAtOffsetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef
// module,
//                                LLVMBuilderRef builder) {
//   printf("df at offse!!! \n");
//   print_ast(ast);
//   print_type(ast->data.AST_APPLICATION.args->md);
//   print_type(ast->md);
//   Type *t = ast->md;
//   if (t->kind != T_CONS) {
//
//     fprintf(stderr,
//             "Error: value passed to df function must be struct (of
//             arrays)\n");
//     return NULL;
//   }
//   for (int i = 0; i < t->data.T_CONS.num_args; i++) {
//     if (!is_array_type(t->data.T_CONS.args[i])) {
//       fprintf(stderr,
//               "Error: value passed to df function must be struct of
//               arrays\n");
//       return NULL;
//     }
//   }
//   LLVMTypeRef df_type = type_to_llvm_type(t, ctx->env, module);
//   LLVMValueRef df_val =
//       codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
//
//   LLVMValueRef offset_val =
//       codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
//   LLVMDumpValue(df_val);
//   printf("\n");
//
//   return NULL;
// }
LLVMValueRef DFAtOffsetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *t = ast->md;
  if (t->kind != T_CONS) {
    fprintf(stderr,
            "Error: value passed to df function must be struct (of arrays)\n");
    return NULL;
  }
  for (int i = 0; i < t->data.T_CONS.num_args; i++) {
    if (!is_array_type(t->data.T_CONS.args[i])) {
      fprintf(stderr,
              "Error: value passed to df function must be struct of arrays\n");
      return NULL;
    }
  }
  LLVMTypeRef df_type = type_to_llvm_type(t, ctx, module);

  LLVMValueRef df_val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef offset_val =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMValueRef result = LLVMGetUndef(df_type);

  unsigned num_fields = LLVMCountStructElementTypes(df_type);

  for (unsigned i = 0; i < num_fields; i++) {
    LLVMValueRef array_struct =
        LLVMBuildExtractValue(builder, df_val, i, "array_struct");

    Type *array_type = t->data.T_CONS.args[i];
    Type *el_type = array_type->data.T_CONS.args[0];
    LLVMTypeRef element_type = type_to_llvm_type(el_type, ctx, module);

    LLVMTypeRef llvm_array_type = codegen_array_type(element_type);
    LLVMValueRef new_array_struct = LLVMGetUndef(llvm_array_type);

    LLVMValueRef current_size =
        LLVMBuildExtractValue(builder, array_struct, 0, "current_size");

    LLVMValueRef is_valid_offset = LLVMBuildICmp(
        builder, LLVMIntSLT, offset_val, current_size, "is_valid_offset");
    LLVMValueRef offset_mask =
        LLVMBuildZExt(builder, is_valid_offset, LLVMInt32Type(), "offset_mask");

    LLVMValueRef effective_offset =
        LLVMBuildMul(builder, offset_val, offset_mask, "effective_offset");

    LLVMValueRef new_size =
        LLVMBuildSub(builder, current_size, effective_offset, "new_size");

    LLVMValueRef data_ptr =
        LLVMBuildExtractValue(builder, array_struct, 1, "data_ptr");

    LLVMValueRef new_data_ptr =
        LLVMBuildGEP2(builder, element_type, data_ptr,
                      (LLVMValueRef[]){effective_offset}, 1, "new_data_ptr");

    new_array_struct = LLVMBuildInsertValue(builder, new_array_struct, new_size,
                                            0, "insert_new_size");
    new_array_struct = LLVMBuildInsertValue(
        builder, new_array_struct, new_data_ptr, 1, "insert_new_data_ptr");

    result = LLVMBuildInsertValue(builder, result, new_array_struct, i,
                                  "result_field");
  }

  return result;
}

LLVMValueRef DFRawFieldsHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  printf("df raw fields handler\n");
  print_ast(ast);
  Type *t = ast->data.AST_APPLICATION.args->md;
  if (t->kind != T_CONS) {
    fprintf(stderr,
            "Error: value passed to df function must be struct (of arrays)\n");
    return NULL;
  }
  for (int i = 0; i < t->data.T_CONS.num_args; i++) {
    if (!is_array_type(t->data.T_CONS.args[i])) {
      fprintf(stderr,
              "Error: value passed to df function must be struct of arrays\n");
      return NULL;
    }
  }
  LLVMTypeRef df_type = type_to_llvm_type(t, ctx, module);

  LLVMValueRef df_val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  // Get number of fields in the dataframe struct
  unsigned num_fields = t->data.T_CONS.num_args;

  // Create a void pointer type (i8*)
  LLVMTypeRef void_ptr_type = LLVMPointerType(LLVMInt8Type(), 0);

  LLVMValueRef size_const = LLVMConstInt(LLVMInt32Type(), num_fields, 0);
  LLVMValueRef result_array = LLVMBuildArrayAlloca(
      builder, void_ptr_type, size_const, "raw_fields_array");

  for (unsigned i = 0; i < num_fields; i++) {
    LLVMValueRef array_struct =
        LLVMBuildExtractValue(builder, df_val, i, "array_struct");

    LLVMValueRef data_ptr =
        LLVMBuildExtractValue(builder, array_struct, 1, "data_ptr");

    LLVMValueRef void_data_ptr =
        LLVMBuildBitCast(builder, data_ptr, void_ptr_type, "void_data_ptr");

    LLVMValueRef ptr_slot =
        LLVMBuildGEP2(builder, void_ptr_type, result_array,
                      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), i, 0)}, 1,
                      "element_ptr");

    LLVMBuildStore(builder, void_data_ptr, ptr_slot);
  }

  return result_array;
}
LLVMValueRef IndexAccessHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  Ast *array_like_ast = ast->data.AST_APPLICATION.function;
  Ast *index_ast = ast->data.AST_APPLICATION.args;
  Type *array_type = array_like_ast->md;

  if (is_array_type(array_type)) {
    LLVMValueRef arr = codegen(array_like_ast, ctx, module, builder);
    LLVMValueRef idx =
        codegen(index_ast->data.AST_LIST.items, ctx, module, builder);
    Type *_el_type = array_type->data.T_CONS.args[0];
    LLVMTypeRef el_type = type_to_llvm_type(_el_type, ctx, module);

    return get_array_element(builder, arr, idx, el_type);
  }
  fprintf(stderr, "Index access not implemented for this type\n");
  return NULL;
}

TypeEnv *initialize_builtin_funcs(JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  t_uint64.constructor = uint64_constructor;
  t_num.constructor = double_constructor;
  t_int.constructor = int_constructor;

  ht *stack = (ctx->frame->table);
#define GENERIC_FN_SYMBOL(id, _scheme, _builtin_handler)                       \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);     \
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =                  \
        _builtin_handler;                                                      \
    sym->symbol_data.STYPE_GENERIC_FUNCTION.scheme = _scheme;                  \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  GENERIC_FN_SYMBOL("+", &arithmetic_scheme, SumHandler);
  GENERIC_FN_SYMBOL("-", &arithmetic_scheme, MinusHandler);
  GENERIC_FN_SYMBOL("*", &arithmetic_scheme, MulHandler);
  GENERIC_FN_SYMBOL("/", &arithmetic_scheme, DivHandler);
  GENERIC_FN_SYMBOL("%", &arithmetic_scheme, ModHandler);

  GENERIC_FN_SYMBOL(">", &ord_scheme, GtHandler);
  GENERIC_FN_SYMBOL("<", &ord_scheme, LtHandler);
  GENERIC_FN_SYMBOL(">=", &ord_scheme, GteHandler);
  GENERIC_FN_SYMBOL("<=", &ord_scheme, LteHandler);

  GENERIC_FN_SYMBOL("==", &eq_scheme, EqAppHandler);
  GENERIC_FN_SYMBOL("!=", &eq_scheme, NeqHandler);

  GENERIC_FN_SYMBOL("||", &bool_or_scheme, LogicalOrHandler);
  GENERIC_FN_SYMBOL("&&", &bool_and_scheme, LogicalAndHandler);

  GENERIC_FN_SYMBOL("Some", &opt_scheme, SomeConsHandler);
  GENERIC_FN_SYMBOL("::", &list_prepend_scheme, ListPrependHandler);

  GENERIC_FN_SYMBOL("list_concat", &list_concat_scheme, ListConcatHandler);
  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_AT, &array_at_scheme, ArrayAtHandler);
  GENERIC_FN_SYMBOL("array_set", &array_set_scheme, ArraySetHandler);
  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_SIZE, &array_size_scheme, ArraySizeHandler);
  return NULL;
}

/*

TypeEnv *initialize_builtin_funcs(JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  ht *stack = (ctx->frame->table);
#define GENERIC_FN_SYMBOL(id, type, _builtin_handler)                          \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, type, NULL, NULL);     \
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =                  \
        _builtin_handler;                                                      \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  GENERIC_FN_SYMBOL("+", &t_add, SumHandler);
  GENERIC_FN_SYMBOL("-", &t_sub, MinusHandler);
  GENERIC_FN_SYMBOL("*", &t_mul, MulHandler);
  GENERIC_FN_SYMBOL("/", &t_div, DivHandler);
  GENERIC_FN_SYMBOL("%", &t_mod, ModHandler);
  GENERIC_FN_SYMBOL(">", &t_gt, GtHandler);
  GENERIC_FN_SYMBOL("<", &t_lt, LtHandler);
  GENERIC_FN_SYMBOL(">=", &t_gte, GteHandler);
  GENERIC_FN_SYMBOL("<=", &t_lte, LteHandler);
  GENERIC_FN_SYMBOL("==", &t_eq, EqAppHandler);
  GENERIC_FN_SYMBOL("!=", &t_neq, NeqHandler);
  GENERIC_FN_SYMBOL("Some", &t_option_of_var, SomeConsHandler);

  // JITSymbol *sym = new_symbol(STYPE_TOP_LEVEL_VAR, &t_none,
  //                             codegen_none(LLVMVoidType(), builder), NULL);
  // ht_set_hash(stack, "None", hash_string("None", 4), sym);


#define FN_SYMBOL(id, type, val)                                               \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_FUNCTION, type, val, NULL);              \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  });

  FN_SYMBOL("print", &t_builtin_print,
            get_extern_fn("print",
                          type_to_llvm_type(&t_builtin_print, ctx, module),
                          module));

  GENERIC_FN_SYMBOL("cstr", &t_builtin_cstr, CStrHandler
                    // get_extern_fn("cstr",
                    //               type_to_llvm_type(&t_builtin_cstr,
                    //               ctx->env, module), module)
  );

  // FN_SYMBOL("empty_coroutine", &t_empty_cor,
  //           get_extern_fn("empty_coroutine",
  //                         type_to_llvm_type(&t_empty_cor, ctx, module),
  //                         module));


  GENERIC_FN_SYMBOL("||", &t_builtin_or, LogicalOrHandler);
  GENERIC_FN_SYMBOL("&&", &t_builtin_and, LogicalAndHandler);

  GENERIC_FN_SYMBOL("cor_loop", &t_cor_loop_sig, CorLoopHandler);
  // GENERIC_FN_SYMBOL("cor_wrap_effect", &t_cor_wrap_effect_fn_sig,
  //                   WrapCoroutineWithEffectHandler);
  //
  GENERIC_FN_SYMBOL("cor_map", &t_cor_map_fn_sig, CorMapHandler);
  GENERIC_FN_SYMBOL("iter", NULL, IterHandler);
  //
  GENERIC_FN_SYMBOL("iter_of_list", &t_iter_of_list_sig, CorOfListHandler);
  GENERIC_FN_SYMBOL("iter_of_array", &t_iter_of_array_sig, CorOfArrayHandler);
  //
  // GENERIC_FN_SYMBOL("cor_replace", &t_cor_replace_fn_sig, CorReplaceHandler);
  GENERIC_FN_SYMBOL("cor_stop", &t_cor_stop_fn_sig, CorStopHandler);
  GENERIC_FN_SYMBOL("cor_counter", &t_cor_counter_fn_sig, CorCounterHandler);
  GENERIC_FN_SYMBOL("cor_status", &t_cor_status_fn_sig, CorStatusHandler);
  GENERIC_FN_SYMBOL("cor_promise", &t_cor_promise_fn_sig,
                    CorGetPromiseValHandler);
  GENERIC_FN_SYMBOL("cor_current", &t_current_cor_fn_sig, CurrentCorHandler);
  GENERIC_FN_SYMBOL("cor_unwrap_or_end", &t_cor_unwrap_or_end_sig,
                    CorUnwrapOrEndHandler);

  GENERIC_FN_SYMBOL("play_routine", &t_play_routine_sig, PlayRoutineHandler);

  // GENERIC_FN_SYMBOL("use_or_finish", &t_use_or_finish, UseOrFinishHandler);

  GENERIC_FN_SYMBOL("::", &t_list_prepend, ListPrependHandler);
  GENERIC_FN_SYMBOL("list_tail", &t_list_tail_sig, ListTailHandler);
  GENERIC_FN_SYMBOL("list_ref_set", &t_list_ref_set_sig, ListRefSetHandler);

  GENERIC_FN_SYMBOL("opt_map", &t_opt_map_sig, OptMapHandler);
  // GENERIC_FN_SYMBOL("run_in_scheduler", &t_run_in_scheduler_sig,
  //                   RunInSchedulerHandler);

  GENERIC_FN_SYMBOL("array_fill", &t_array_fill_sig, ArrayFillHandler);
  GENERIC_FN_SYMBOL("array_fill_const", &t_array_fill_const_sig,
                    ArrayFillConstHandler);
  GENERIC_FN_SYMBOL("array_succ", &t_array_identity_sig, ArraySuccHandler);
  GENERIC_FN_SYMBOL("array_range", &t_array_range_sig, ArrayRangeHandler);
  GENERIC_FN_SYMBOL("array_offset", &t_array_offset_sig, ArrayOffsetHandler);

  // GENERIC_FN_SYMBOL("array_view", &t_array_view_sig, ArrayViewHandler);
  GENERIC_FN_SYMBOL("Double", next_tvar(), double_constructor_handler);
  GENERIC_FN_SYMBOL("Int", next_tvar(), int_constructor_handler);
  GENERIC_FN_SYMBOL(TYPE_NAME_UINT64, next_tvar(), uint64_constructor_handler);
  GENERIC_FN_SYMBOL(TYPE_NAME_CHAR, next_tvar(), char_cons_handler);

  GENERIC_FN_SYMBOL("struct_set", &t_struct_set_sig, StructSetHandler);
  GENERIC_FN_SYMBOL("addrof", NULL, AddrOfHandler);
  GENERIC_FN_SYMBOL("fst", &t_fst_sig, FstHandler);
  GENERIC_FN_SYMBOL("save_pattern_to_file", next_tvar(), SerializeBlobHandler);
  GENERIC_FN_SYMBOL("df_offset", &t_df_offset_sig, DFAtOffsetHandler);
  GENERIC_FN_SYMBOL("df_raw_fields", &t_df_raw_fields_sig, DFRawFieldsHandler);
  GENERIC_FN_SYMBOL(TYPE_NAME_ARRAY, &t_array_cons_sig, ArrayConstructor);
  GENERIC_FN_SYMBOL("list_empty", NULL, ListEmptyHandler);
  GENERIC_FN_SYMBOL("dlopen", NULL, DlOpenHandler);
  // GENERIC_FN_SYMBOL("coroutine_end", &t_coroutine_end, CoroutineEndHandler);

  // GENERIC_FN_SYMBOL(TYPE_NAME_REF, NULL, RefHandler);

  // GENERIC_FN_SYMBOL("queue_append_right", &t_list_prepend,
  // ListPrependHandler);

  // GENERIC_FN_SYMBOL("Char", &t_builtin_char_of, CharHandler);
  // GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_DATA_PTR, &t_array_data_ptr_fn_sig);
  //
  // GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_INCR, &t_array_incr_fn_sig);
  //
  // GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_SLICE, &t_array_slice_fn_sig);
  //
  // GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_NEW, &t_array_new_fn_sig);
  //
  // GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_TO_LIST, &t_array_to_list_fn_sig);
  //

  return ctx->env;
}
*/
