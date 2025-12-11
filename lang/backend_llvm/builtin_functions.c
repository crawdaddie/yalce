#include "backend_llvm/builtin_functions.h"
#include "adt.h"
#include "application.h"

#include "./coroutines/coroutines.h"
#include "backend_llvm/array.h"
#include "backend_llvm/common.h"
#include "function.h"
#include "input.h"
#include "list.h"
#include "module.h"
#include "strings.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include "util.h"
#include "llvm-c/Target.h"
#include <dlfcn.h>
#include <llvm-c/Core.h>
#include <stdlib.h>
#include <string.h>

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

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
                      new_symbol(STYPE_FUNCTION, expr->type, func,
                                 type_to_llvm_type(expr->type, ctx, module));

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

  Type *free_arg_type = type->data.T_FN.from;

  LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(ret, ctx, module);

  LLVMTypeRef llvm_from_type_ref =
      type_to_llvm_type(type->data.T_FN.from, ctx, module);

  LLVMTypeRef curried_fn_type = LLVMFunctionType(
      llvm_return_type_ref, (LLVMTypeRef[]){llvm_from_type_ref}, 1, 0);

  START_FUNC(module, "anon_curried_binop", curried_fn_type);

  LLVMValueRef saved_arg = codegen(saved_arg_ast, ctx, module, builder);
  LLVMValueRef free_arg = LLVMGetParam(func, 0);
  LLVMValueRef binop_res;

  switch (ret->kind) {
  case T_INT:
  case T_UINT64: {
    binop_res =
        LLVMBuildBinOp(builder, iop, saved_arg,
                       handle_type_conversions(free_arg, free_arg_type, ret,
                                               ctx, module, builder),
                       "curried_binop");
    break;
  }
  case T_NUM: {
    binop_res =
        LLVMBuildBinOp(builder, fop, saved_arg,

                       handle_type_conversions(free_arg, free_arg_type, ret,
                                               ctx, module, builder),
                       "curried_binop");
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
                         ast->type, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);

  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  ARITHMETIC_BINOP("+", LLVMFAdd, LLVMAdd);
}

LLVMValueRef MinusHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFSub, LLVMSub,
                         ast->type, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("-", LLVMFSub, LLVMSub);
}

LLVMValueRef MulHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFMul, LLVMMul,
                         ast->type, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("*", LLVMFMul, LLVMMul);
}

LLVMValueRef DivHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFDiv, LLVMSDiv,
                         ast->type, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("/", LLVMFDiv, LLVMSDiv);
}

LLVMValueRef ModHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.len < 2) {
    return curried_binop(ast->data.AST_APPLICATION.args, LLVMFRem, LLVMSRem,
                         ast->type, ctx, module, builder);
  }

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;

  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;

  ARITHMETIC_BINOP("%", LLVMFRem, LLVMSRem);
}

LLVMValueRef gte_val(LLVMValueRef val, LLVMValueRef from, Type *type,
                     JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  LLVMValueRef l = val;
  LLVMValueRef r = from;

  switch (type->kind) {
  case T_INT:
  case T_CHAR:
  case T_UINT64: {
    return LLVMBuildICmp(builder, LLVMIntSGE, l, r, "gte_int");
  }
  case T_NUM: {
    return LLVMBuildFCmp(builder, LLVMRealOGE, l, r, "gte_num");
  }
  default: {
    fprintf(stderr, "Error: unrecognized operands for ord binop\n");
    return NULL;
  }
  }
}

LLVMValueRef lte_val(LLVMValueRef val, LLVMValueRef from, Type *type,
                     JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  LLVMValueRef l = val;
  LLVMValueRef r = from;

  switch (type->kind) {
  case T_INT:
  case T_CHAR:
  case T_UINT64: {
    return LLVMBuildICmp(builder, LLVMIntSLE, l, r, "lte_int");
  }
  case T_NUM: {
    return LLVMBuildFCmp(builder, LLVMRealOLE, l, r, "lte_num");
  }
  default: {
    fprintf(stderr, "Error: unrecognized operands for ord binop\n");
    return NULL;
  }
  }
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
      fprintf(stderr, "Error: unrecognized operands for ord binop\n");         \
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

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  // printf("ord\n");
  // print_type(lt);
  // print_type(rt);
  // print_type_env(ctx->env);
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

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
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

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
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

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->type);
  fn_type = resolve_type_in_env(fn_type, ctx->env);
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
  LLVMDumpType(llvm_el_type);
  printf("\n");
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

  printf("\n");
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

  if (type->kind == T_VAR) {
    type = resolve_type_in_env(type, ctx->env);
  }

  switch (type->kind) {
  case T_VAR: {
    // if (type->is_recursive_type_ref) {
    //   Type t = *type;
    //   t.is_recursive_type_ref = false;
    //   return _codegen_equality(&t, l, r, ctx, module, builder);
    // }

    return _FALSE;
  }
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

    if (is_sum_type(type)) {
      return sum_type_eq(type, l, r, ctx, module, builder);
    }

    return cons_equality(type, l, r, ctx, module, builder);
  }
  }

  return _FALSE;
}

LLVMValueRef EqAppHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *fn_type = ast->data.AST_APPLICATION.function->type;
  Type *lt = fn_type->data.T_FN.from;

  lt = resolve_type_in_env(lt, ctx->env);
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  rt = resolve_type_in_env(rt, ctx->env);
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

  Type *fn_type = ast->data.AST_APPLICATION.function->type;
  Type *lt = fn_type->data.T_FN.from;
  lt = resolve_type_in_env(lt, ctx->env);
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  rt = resolve_type_in_env(rt, ctx->env);

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

  case T_BOOL: {
    return LLVMBuildZExt(builder, val, LLVMInt32Type(), "cast_bool_to_int32");
  }

  case T_NUM: {
    return LLVMBuildFPToSI(builder, val, LLVMInt32Type(),
                           "cast_double_to_int32");
  }

  case T_INT: {
    return val;
  }

  case T_UINT64: {
    // return LLVMBuildUIToFP(builder, val, LLVMDoubleType(),
    //                        "cast_uint64_to_double");
    //
    return LLVMBuildTrunc(builder, val, LLVMInt32Type(), "trunc_to_i32");
  }

  default:
    return val;
  }
}

LLVMValueRef char_constructor(LLVMValueRef val, Type *from_type,
                              LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_NUM: {
    // return LLVMBuildFPToSI(builder, val, LLVMInt32Type(),
    // "cast_double_to_int");
    return NULL;
  }

  case T_INT: {
    // return LLVMBuildI;
    return LLVMBuildTrunc(builder, val, LLVMInt8Type(), "trunc_to_i8");
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

LLVMValueRef CharConstructorHandler(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return char_constructor(
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder),
      ast->data.AST_APPLICATION.args->type, module, builder);
}

LLVMValueRef int_constructor_handler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  return int_constructor(
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder),
      ast->data.AST_APPLICATION.args->type, module, builder);
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

  Type *arr_type = array_ast->type;
  Type *el_type = arr_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx, module);

  LLVMValueRef array = codegen(array_ast, ctx, module, builder);

  return codegen_get_array_size(builder, array, llvm_el_type);
}

LLVMValueRef ArrayAtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  Type *ret_type = ast->type;
  Ast *array_ast = ast->data.AST_APPLICATION.args;
  Ast *idx_ast = ast->data.AST_APPLICATION.args + 1;
  LLVMValueRef array = codegen(array_ast, ctx, module, builder);
  LLVMValueRef idx = codegen(idx_ast, ctx, module, builder);

  if (ret_type->kind == T_FN) {
    return get_array_element(builder, array, idx, GENERIC_PTR);
  }

  LLVMTypeRef el_type = type_to_llvm_type(ret_type, ctx, module);

  if (!el_type) {
    fprintf(stderr, "Error: no array element type found\n");
    return NULL;
  }

  LLVMValueRef el = get_array_element(builder, array, idx, el_type);
  return el;
}

LLVMValueRef ArraySetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  Type *ret_type = ast->type;
  Ast *idx_ast = ast->data.AST_APPLICATION.args + 1;
  Ast *array_ast = ast->data.AST_APPLICATION.args;
  Ast *val_ast = ast->data.AST_APPLICATION.args + 2;
  LLVMValueRef array = codegen(array_ast, ctx, module, builder);
  LLVMValueRef idx = codegen(idx_ast, ctx, module, builder);
  LLVMValueRef val = codegen(val_ast, ctx, module, builder);
  Type *el_type = ret_type->data.T_CONS.args[0];

  return set_array_element(builder, array, idx, val,
                           el_type->kind == T_FN || is_coroutine_type(el_type)
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

LLVMValueRef AddrOfHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  Type *t = ast->type;
  LLVMTypeRef llvm_type = type_to_llvm_type(t, ctx, module);

  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  return val;
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
  return uint64_constructor(in, ast->data.AST_APPLICATION.args->type, module,
                            builder);
}

LLVMValueRef char_cons_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  Type *from_type = ast->data.AST_APPLICATION.args->type;

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
  Type *t = ast->type;
  LLVMValueRef val = codegen(arg, ctx, module, builder);
  return val;
}

LLVMValueRef SerializeBlobHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

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
  Type *t = ast->type;
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

  Type *t = ast->data.AST_APPLICATION.args->type;
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
  Type *array_type = array_like_ast->type;
  if (is_array_type(array_type)) {
    LLVMValueRef arr = codegen(array_like_ast, ctx, module, builder);
    LLVMValueRef idx =
        codegen(index_ast->data.AST_LIST.items, ctx, module, builder);
    Type *_el_type = array_type->data.T_CONS.args[0];
    LLVMTypeRef el_type = type_to_llvm_type(_el_type, ctx, module);

    return get_array_element(builder, arr, idx, el_type);
  }
}

LLVMValueRef SizeOfHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  Type *t = NULL;
  TICtx _ctx = {.env = ctx->env};

  Ast *expr = ast->data.AST_APPLICATION.args;
  t = expr->type;

  unsigned size;

  if (!t) {
    size = 0;
  }

  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef llvm_type = type_to_llvm_type(t, ctx, module);

  if (!llvm_type) {
    size = 0;
  } else {
    size = LLVMStoreSizeOfType(target_data, llvm_type);
  }

  return LLVMConstInt(LLVMInt32Type(), size, 0);
}

LLVMValueRef TypeOfHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  // return NULLL
  // print_type(ast->type);
}

LLVMValueRef AsBytesHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  bool on_stack = false;
  if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC) {
    on_stack = true;
  }

  LLVMTypeRef char_type = LLVMInt8Type();
  LLVMTypeRef struct_type = string_struct_type(LLVMPointerType(char_type, 0));
  LLVMValueRef str = LLVMGetUndef(struct_type);

  Type *t = ast->data.AST_APPLICATION.args->type;

  switch (t->kind) {
  case T_INT: {
    int width = 4;

    LLVMValueRef value =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

    LLVMTypeRef array_type = LLVMArrayType(char_type, width);

    LLVMValueRef byte_array_ptr;
    if (on_stack && ctx->coro_ctx == NULL) {
      byte_array_ptr = LLVMBuildAlloca(builder, array_type, "int_bytes_stack");
    } else {
      byte_array_ptr = LLVMBuildMalloc(builder, array_type, "int_bytes_heap");
    }

    LLVMValueRef byte_ptr = LLVMBuildBitCast(
        builder, byte_array_ptr, LLVMPointerType(char_type, 0), "byte_ptr");

    for (int i = 0; i < width; i++) {
      LLVMValueRef shift_amount = LLVMConstInt(LLVMInt32Type(), i * 8, 0);
      LLVMValueRef shifted =
          LLVMBuildLShr(builder, value, shift_amount, "shift");
      LLVMValueRef byte = LLVMBuildTrunc(builder, shifted, char_type, "byte");

      LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), i, 0)};
      LLVMValueRef elem_ptr =
          LLVMBuildGEP2(builder, char_type, byte_ptr, indices, 1, "elem_ptr");
      LLVMBuildStore(builder, byte, elem_ptr);
    }

    str = LLVMBuildInsertValue(builder, str, byte_ptr, 1, "insert_data");
    str = LLVMBuildInsertValue(builder, str,
                               LLVMConstInt(LLVMInt32Type(), width, 0), 0,
                               "insert_size");
    return str;
  }

  case T_UINT64: {
    int width = 8;

    LLVMTypeRef char_type = LLVMInt8Type();

    LLVMValueRef value =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

    LLVMTypeRef array_type = LLVMArrayType(char_type, width);

    LLVMValueRef byte_array_ptr;
    if (on_stack && ctx->coro_ctx == NULL) {
      byte_array_ptr = LLVMBuildAlloca(builder, array_type, "int_bytes_stack");
    } else {
      byte_array_ptr = LLVMBuildMalloc(builder, array_type, "int_bytes_heap");
    }

    LLVMValueRef byte_ptr = LLVMBuildBitCast(
        builder, byte_array_ptr, LLVMPointerType(char_type, 0), "byte_ptr");

    for (int i = 0; i < 8; i++) {
      LLVMValueRef shift_amount = LLVMConstInt(LLVMInt32Type(), i * 8, 0);
      LLVMValueRef shifted =
          LLVMBuildLShr(builder, value, shift_amount, "shift");
      LLVMValueRef byte = LLVMBuildTrunc(builder, shifted, char_type, "byte");

      LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), i, 0)};
      LLVMValueRef elem_ptr =
          LLVMBuildGEP2(builder, char_type, byte_ptr, indices, 1, "elem_ptr");
      LLVMBuildStore(builder, byte, elem_ptr);
    }

    str = LLVMBuildInsertValue(builder, str, byte_ptr, 1, "insert_data");
    str = LLVMBuildInsertValue(builder, str,
                               LLVMConstInt(LLVMInt32Type(), width, 0), 0,
                               "insert_size");
    return str;
  }

  case T_NUM: {

    int width = 8;

    LLVMValueRef value =
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
    value = LLVMBuildBitCast(builder, value, LLVMInt64Type(), "double_as_int");

    LLVMTypeRef array_type = LLVMArrayType(char_type, width);

    LLVMValueRef byte_array_ptr;
    if (on_stack && ctx->coro_ctx == NULL) {
      byte_array_ptr = LLVMBuildAlloca(builder, array_type, "int_bytes_stack");
    } else {
      byte_array_ptr = LLVMBuildMalloc(builder, array_type, "int_bytes_heap");
    }

    LLVMValueRef byte_ptr = LLVMBuildBitCast(
        builder, byte_array_ptr, LLVMPointerType(char_type, 0), "byte_ptr");

    for (int i = 0; i < 8; i++) {
      LLVMValueRef shift_amount = LLVMConstInt(LLVMInt64Type(), i * 8, 0);
      LLVMValueRef shifted =
          LLVMBuildLShr(builder, value, shift_amount, "shift");
      LLVMValueRef byte = LLVMBuildTrunc(builder, shifted, char_type, "byte");

      LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), i, 0)};
      LLVMValueRef elem_ptr =
          LLVMBuildGEP2(builder, char_type, byte_ptr, indices, 1, "elem_ptr");
      LLVMBuildStore(builder, byte, elem_ptr);
    }

    str = LLVMBuildInsertValue(builder, str, byte_ptr, 1, "insert_data");
    str = LLVMBuildInsertValue(builder, str,
                               LLVMConstInt(LLVMInt32Type(), width, 0), 0,
                               "insert_size");
    return str;
  }

  case T_CHAR: {
  }
  default: {
    return NULL;
  }
  }
}

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

  // JITSymbol *sym = new_symbol(STYPE_TOP_LEVEL_VAR, &t_none,
  //                             codegen_none(LLVMVoidType(), builder), NULL);
  // ht_set_hash(stack, "None", hash_string("None", 4), sym);

  t_uint64.constructor = uint64_constructor;
  t_num.constructor = double_constructor;
  t_int.constructor = int_constructor;
  // t_char.constructor = char_constructor;

#define FN_SYMBOL(id, type, val)                                               \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_FUNCTION, type, val, NULL);              \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  });

  GENERIC_FN_SYMBOL("+", &arithmetic_scheme, SumHandler);
  GENERIC_FN_SYMBOL("-", &arithmetic_scheme, MinusHandler);
  GENERIC_FN_SYMBOL("*", &arithmetic_scheme, MulHandler);
  GENERIC_FN_SYMBOL("/", &arithmetic_scheme, DivHandler);
  GENERIC_FN_SYMBOL("%", &arithmetic_scheme, ModHandler);
  GENERIC_FN_SYMBOL(">", &ord_scheme, GtHandler);
  GENERIC_FN_SYMBOL(">=", &ord_scheme, GteHandler);
  GENERIC_FN_SYMBOL("<", &ord_scheme, LtHandler);
  GENERIC_FN_SYMBOL("<=", &ord_scheme, LteHandler);
  GENERIC_FN_SYMBOL("==", &ord_scheme, EqAppHandler);
  GENERIC_FN_SYMBOL("!=", &ord_scheme, NeqHandler);
  GENERIC_FN_SYMBOL("&&", &logical_op_scheme, LogicalAndHandler);
  GENERIC_FN_SYMBOL("||", &logical_op_scheme, LogicalOrHandler);

  GENERIC_FN_SYMBOL("array_at", &array_at_scheme, ArrayAtHandler);
  GENERIC_FN_SYMBOL("array_size", &array_size_scheme, ArraySizeHandler);
  GENERIC_FN_SYMBOL("array_succ", &array_id_scheme, ArraySuccHandler);
  GENERIC_FN_SYMBOL("array_set", &array_set_scheme, ArraySetHandler);
  GENERIC_FN_SYMBOL("array_fill_const", &array_fill_const_scheme,
                    ArrayFillConstHandler);
  GENERIC_FN_SYMBOL("array_fill", &array_fill_scheme, ArrayFillHandler);
  GENERIC_FN_SYMBOL("array_range", &array_range_scheme, ArrayRangeHandler);
  GENERIC_FN_SYMBOL("array_offset", &array_offset_scheme, ArrayOffsetHandler);

  GENERIC_FN_SYMBOL("Some", &opt_scheme, SomeConsHandler);

  GENERIC_FN_SYMBOL("::", &list_prepend_scheme, ListPrependHandler);
  GENERIC_FN_SYMBOL("list_concat", &list_concat_scheme, ListConcatHandler);

  GENERIC_FN_SYMBOL("str", &str_fmt_scheme, StringFmtHandler);
  GENERIC_FN_SYMBOL("print", &t_builtin_print, PrintHandler);
  // GENERIC_FN_SYMBOL("Coroutine", &cor_scheme, CorConsHandler);

  GENERIC_FN_SYMBOL("list_empty", NULL, ListEmptyHandler);

  GENERIC_FN_SYMBOL("dlopen", &dlopen_type, DlOpenHandler);

  GENERIC_FN_SYMBOL("cstr", &cstr_scheme, CStrHandler);
  GENERIC_FN_SYMBOL("sizeof", &sizeof_scheme, SizeOfHandler);

  GENERIC_FN_SYMBOL("Char", NULL, CharConstructorHandler);
  GENERIC_FN_SYMBOL("Int", NULL, int_constructor_handler);
  GENERIC_FN_SYMBOL("Uint64", NULL, uint64_constructor_handler);
  GENERIC_FN_SYMBOL("Array", NULL, ArrayConstructorHandler);

  // FN_SYMBOL()

  // GENERIC_FN_SYMBOL("cor_counter", &t_cor_counter_fn_sig, CorCounterHandler);
  // GENERIC_FN_SYMBOL("cor_status", &t_cor_status_fn_sig, CorStatusHandler);
  // GENERIC_FN_SYMBOL("cor_promise", NULL, CorGetPromiseValHandler);
  GENERIC_FN_SYMBOL("cor_last_val", NULL, CorGetLastValHandler);
  GENERIC_FN_SYMBOL("cor_loop", &cor_loop_scheme, CorLoopHandler);
  GENERIC_FN_SYMBOL("cor_map", &cor_map_scheme, CorMapHandler);
  GENERIC_FN_SYMBOL("cor_stop", &cor_stop_scheme, CorStopHandler);
  GENERIC_FN_SYMBOL("iter_of_list", &iter_of_list_scheme, CorOfListHandler);
  GENERIC_FN_SYMBOL("iter_of_array", &iter_of_array_scheme, CorOfArrayHandler);
  GENERIC_FN_SYMBOL("play_routine", &play_routine_scheme, PlayRoutineHandler);
  GENERIC_FN_SYMBOL("cor_current", &cor_current_scheme, CurrentCorHandler);
  GENERIC_FN_SYMBOL("cor_try_opt", &cor_try_opt_scheme, CorUnwrapOrEndHandler);

  GENERIC_FN_SYMBOL("asbytes", &asbytes_scheme, AsBytesHandler);

  GENERIC_FN_SYMBOL("typeof", &typeof_scheme, TypeOfHandler);
  return ctx->env;
}
