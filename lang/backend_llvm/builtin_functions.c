#include "backend_llvm/builtin_functions.h"
#include "adt.h"
#include "application.h"
#include "backend_llvm/array.h"
#include "backend_llvm/common.h"
#include "function.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
#define ARITHMETIC_BINOP(_name, _flop, _iop)                                   \
  ({                                                                           \
    Type *ret = fn_return_type(fn_type);                                       \
    LLVMValueRef l =                                                           \
        codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);         \
    l = handle_type_conversions(l, lt, ret, module, builder);                  \
    LLVMValueRef r =                                                           \
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);     \
    r = handle_type_conversions(r, rt, ret, module, builder);                  \
    switch (ret->kind) {                                                       \
    case T_INT:                                                                \
    case T_UINT64: {                                                           \
      return LLVMBuildBinOp(builder, _iop, l, r, _name "_int");                \
    }                                                                          \
    case T_NUM: {                                                              \
      return LLVMBuildBinOp(builder, _flop, l, r, _name "_num");               \
    }                                                                          \
    }                                                                          \
  })

LLVMValueRef SumHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);

  Type *ret = fn_return_type(fn_type);
  fn_type->data.T_FN.to->data.T_FN.to = resolve_type_in_env(ret, ctx->env);

  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);

  ARITHMETIC_BINOP("add", LLVMFAdd, LLVMAdd);

  return NULL;
}

LLVMValueRef MinusHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);

  Type *ret = fn_return_type(fn_type);
  fn_type->data.T_FN.to->data.T_FN.to = resolve_type_in_env(ret, ctx->env);

  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);

  ARITHMETIC_BINOP("sub", LLVMFSub, LLVMSub);
  return NULL;
}

LLVMValueRef MulHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);

  Type *ret = fn_return_type(fn_type);
  fn_type->data.T_FN.to->data.T_FN.to = resolve_type_in_env(ret, ctx->env);

  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);

  ARITHMETIC_BINOP("mul", LLVMFMul, LLVMMul);
  return NULL;
}

LLVMValueRef DivHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  // LLVMSDiv,

  Type *fn_type = ast->data.AST_APPLICATION.function->md;
  Type *lt = fn_type->data.T_FN.from;
  Type *rt = (fn_type->data.T_FN.to)->data.T_FN.from;
  ARITHMETIC_BINOP("div", LLVMFDiv, LLVMSDiv);
  return NULL;
}

LLVMValueRef ModHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);

  Type *ret = fn_return_type(fn_type);
  fn_type->data.T_FN.to->data.T_FN.to = resolve_type_in_env(ret, ctx->env);

  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);

  ARITHMETIC_BINOP("mod", LLVMFRem, LLVMSRem);
  return NULL;
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
    l = handle_type_conversions(l, lt, target_type, module, builder);          \
    LLVMValueRef r =                                                           \
        codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);     \
    r = handle_type_conversions(r, rt, target_type, module, builder);          \
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

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);
  ORD_BINOP("gt", LLVMRealOGT, LLVMIntSGT);
}
LLVMValueRef GteHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);
  ORD_BINOP("gte", LLVMRealOGE, LLVMIntSGE);
}

LLVMValueRef LtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder) {

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);
  ORD_BINOP("lt", LLVMRealOLT, LLVMIntSLT);
}

LLVMValueRef LteHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  Type *fn_type = deep_copy_type(ast->data.AST_APPLICATION.function->md);
  Type *lt = resolve_type_in_env(fn_type->data.T_FN.from, ctx->env);
  Type *rt =
      resolve_type_in_env((fn_type->data.T_FN.to)->data.T_FN.from, ctx->env);
  ORD_BINOP("lte", LLVMRealOLE, LLVMIntSLE);
}

LLVMValueRef _codegen_equality(Type *type, LLVMValueRef l, LLVMValueRef r,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef cons_equality(Type *type, LLVMValueRef tuple1, LLVMValueRef tuple2,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMTypeRef tuple_type = type_to_llvm_type(type, ctx->env, module);

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

    if ((strcmp(type->data.T_CONS.name, "Variant") == 0) &&
        (type->data.T_CONS.num_args == 2) &&
        (strcmp(type->data.T_CONS.args[0]->data.T_CONS.name, "Some") == 0) &&
        (strcmp(type->data.T_CONS.args[1]->data.T_CONS.name, "None") == 0)) {
      return option_eq(type, l, r, ctx, module, builder);
    }
    if (is_option_type(type)) {
      return option_eq(type, l, r, ctx, module, builder);
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
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
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

  l = handle_type_conversions(l, lt, target_type, module, builder);
  LLVMValueRef r =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  r = handle_type_conversions(r, rt, target_type, module, builder);

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
  Type *rt = fn_type->data.T_FN.to->data.T_FN.from;
  Type *target_type;
  if (get_typeclass_rank(lt, "eq") >= get_typeclass_rank(rt, "eq")) {
    target_type = lt;
  } else {
    target_type = rt;
  }
  LLVMValueRef l =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  l = handle_type_conversions(l, lt, target_type, module, builder);
  LLVMValueRef r =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  r = handle_type_conversions(r, rt, target_type, module, builder);

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

LLVMValueRef uint64_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_INT: {
    // Create uint64 type
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
  LLVMTypeRef llvm_el_type = type_to_llvm_type(el_type, ctx->env, module);

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
  return get_array_element(builder, array, idx,
                           type_to_llvm_type(ret_type, ctx->env, module));
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

LLVMValueRef LogicalAndHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  // short-circuiting and-operator
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");
  LLVMValueRef arg1 =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMBuildCondBr(builder, arg1, then_block, else_block);

  /* Then block */
  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_result =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  /* Else block */
  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef else_result = _FALSE;
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  /* Merge block */
  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");
  LLVMValueRef incoming_vals[] = {then_result, else_result};
  LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

  return phi;
}

LLVMValueRef LogicalOrHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  // short-circuiting or-operator
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

  // Evaluate first argument
  LLVMValueRef arg1 =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  // If arg1 is true, skip to then_block (short-circuit)
  // If arg1 is false, evaluate second argument in else_block
  LLVMBuildCondBr(builder, arg1, then_block, else_block);

  /* Then block - short circuit with true */
  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_result = _TRUE; // Short circuit with true
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);

  /* Else block - evaluate second argument */
  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef else_result =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);

  /* Merge block */
  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");
  LLVMValueRef incoming_vals[] = {then_result, else_result};
  LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

  return phi;
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

  t_uint64.constructor = uint64_constructor;
  t_uint64.constructor_size = sizeof(ConsMethod);

  t_num.constructor = double_constructor;
  t_num.constructor_size = sizeof(ConsMethod);

#define FN_SYMBOL(id, type, val)                                               \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_FUNCTION, type, val, NULL);              \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  });

  FN_SYMBOL("print", &t_builtin_print,
            get_extern_fn("print",
                          type_to_llvm_type(&t_builtin_print, ctx->env, module),
                          module));

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_AT, &t_array_at_fn_sig, ArrayAtHandler);
  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_SIZE, &t_array_size_fn_sig,
                    ArraySizeHandler);

  GENERIC_FN_SYMBOL("||", &t_builtin_or, LogicalOrHandler);
  GENERIC_FN_SYMBOL("&&", &t_builtin_and, LogicalAndHandler);
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
