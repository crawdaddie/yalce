#include "backend_llvm/binop.h"
#include "backend_llvm/common.h"
#include "parse.h"
#include "serde.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

// clang-format off
static int int_ops_map[] = {
  [TOKEN_PLUS] =      LLVMAdd,
  [TOKEN_MINUS] =     LLVMSub,
  [TOKEN_STAR] =      LLVMMul,
  [TOKEN_SLASH] =     LLVMSDiv,
  [TOKEN_MODULO] =    LLVMSRem,

  [TOKEN_LT] =        LLVMIntSLT,
  [TOKEN_LTE] =       LLVMIntSLE,
  [TOKEN_GT] =        LLVMIntSGT,
  [TOKEN_GTE] =       LLVMIntSGE,
  [TOKEN_EQUALITY] =  LLVMIntEQ,
  [TOKEN_NOT_EQUAL] = LLVMIntNE,
};

static int float_ops_map[] = {
  [TOKEN_PLUS] =      LLVMFAdd,
  [TOKEN_MINUS] =     LLVMFSub,
  [TOKEN_STAR] =      LLVMFMul,   
  [TOKEN_SLASH] =     LLVMFDiv,
  [TOKEN_MODULO] =    LLVMFRem,

  [TOKEN_LT] =        LLVMRealOLT,
  [TOKEN_LTE] =       LLVMRealOLE,
  [TOKEN_GT] =        LLVMRealOGT,
  [TOKEN_GTE] =       LLVMRealOGE,
  [TOKEN_EQUALITY] =  LLVMRealOEQ,
  [TOKEN_NOT_EQUAL] = LLVMRealONE,

};

// clang-format on

LLVMValueRef codegen_int_binop(LLVMBuilderRef builder, token_type op,
                               LLVMValueRef l, LLVMValueRef r) {

  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    return LLVMBuildBinOp(builder, int_ops_map[op], l, r, "");
  }
  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL: {
    return LLVMBuildICmp(builder, int_ops_map[op], l, r, "");
  }
  default:
    return NULL;
  }
}

LLVMValueRef codegen_float_binop(LLVMBuilderRef builder, token_type op,
                                 LLVMValueRef l, LLVMValueRef r) {
  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    return LLVMBuildBinOp(builder, float_ops_map[op], l, r, "float_binop");
  }

  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL: {
    return LLVMBuildFCmp(builder, float_ops_map[op], l, r, "");
  }
  }
}

static LLVMValueRef cast_to_float(LLVMBuilderRef builder, LLVMValueRef i) {
  return LLVMBuildSIToFP(builder, i, LLVMDoubleType(), "cast_int_to_double");
}

static bool is_llvm_float(LLVMValueRef v) {
  LLVMTypeKind kind = LLVMGetTypeKind(LLVMTypeOf(v));
  return kind == LLVMDoubleTypeKind || kind == LLVMFloatTypeKind;
}

static bool is_llvm_int(LLVMValueRef v) {
  LLVMTypeKind kind = LLVMGetTypeKind(LLVMTypeOf(v));
  return kind == LLVMIntegerTypeKind;
}

bool is_num_op(token_type op) {
  return (op >= TOKEN_PLUS) && (op <= TOKEN_MODULO);
}
bool is_ord_op(token_type op) { return (op >= TOKEN_LT) && (op <= TOKEN_GTE); }

typedef LLVMValueRef (*NumTypeClassMethod)(LLVMValueRef, Type *, LLVMValueRef,
                                           Type *, LLVMModuleRef,
                                           LLVMBuilderRef);

LLVMValueRef typeclass_num_binop_impl(token_type op, TypeClass *num,
                                      LLVMValueRef lval, Type *ltype,
                                      LLVMValueRef rval, Type *rtype,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  int op_index = op - TOKEN_PLUS;
  NumTypeClassMethod *method_ptr = get_typeclass_method(num, op_index);

  if (method_ptr == NULL) {
    fprintf(stderr, "Invalid operation index for typeclass %s\n", num->name);
    return NULL;
  }

  NumTypeClassMethod method = *method_ptr;

  if (method == NULL) {
    fprintf(stderr, "typeclass %s method not implemented for op %d\n",
            num->name, op);
    return NULL;
  }
  return method(lval, ltype, rval, rtype, module, builder);
}

LLVMValueRef typeclass_ord_binop_impl(token_type op, TypeClass *num,
                                      LLVMValueRef lval, Type *ltype,
                                      LLVMValueRef rval, Type *rtype,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef codegen_binop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef l = codegen(ast->data.AST_BINOP.left, ctx, module, builder);
  Type *ltype = ast->data.AST_BINOP.left->md;
  LLVMValueRef r = codegen(ast->data.AST_BINOP.right, ctx, module, builder);
  Type *rtype = ast->data.AST_BINOP.right->md;

  if (l == NULL || r == NULL) {
    return NULL;
  }

  token_type op = ast->data.AST_BINOP.op;
  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    TypeClass *tc_impl = typeclass_impl(ast->md, &TCNum);
    if ((tc_impl != NULL) && !(is_arithmetic(ast->md))) {

      return typeclass_num_binop_impl(op, typeclass_impl(ast->md, &TCNum), l,
                                      ltype, r, rtype, module, builder);
    }
    break;
  }

  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE: {
    TypeClass *tc_impl = typeclass_impl(ast->md, &TCOrd);
    if ((tc_impl != NULL) && !(is_arithmetic(ast->md))) {
      return typeclass_ord_binop_impl(op, typeclass_impl(ast->md, &TCNum), l,
                                      ltype, r, rtype, module, builder);
    }
    break;
  }
  }

  if (!(is_arithmetic(ltype) && is_arithmetic(rtype))) {
    return NULL;
  }

  // builtin numeric types:
  int type_check = ((ltype->kind == T_NUM) << 1) | (rtype->kind == T_NUM);

  if (is_llvm_int(l) && is_llvm_int(r)) {
    return codegen_int_binop(builder, op, l, r);
  }

  if (is_llvm_int(l) && is_llvm_float(r)) {

    return codegen_float_binop(builder, op, cast_to_float(builder, l), r);
  }

  if (is_llvm_float(l) && is_llvm_int(r)) {

    return codegen_float_binop(builder, op, l, cast_to_float(builder, r));
  }

  if (is_llvm_float(l) && is_llvm_float(r)) {

    return codegen_float_binop(builder, op, l, r);
  }

  return NULL;
}
