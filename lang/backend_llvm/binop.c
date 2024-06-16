#include "backend_llvm/binop.h"
#include "common.h"
#include "parse.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

// assert(LLVMFRem - LLVMAdd == nkSmmFRem - nkSmmAdd);
// LLVMValueRef res = NULL;
//
// switch (expr->kind) {
// case nkSmmAdd: case nkSmmFAdd: case nkSmmSub: case nkSmmFSub:
// case nkSmmMul: case nkSmmFMul: case nkSmmUDiv: case nkSmmSDiv: case
// nkSmmFDiv: case nkSmmURem: case nkSmmSRem: case nkSmmFRem:
// 	{
// 		LLVMValueRef left = processExpression(data, expr->left, a);
// 		LLVMValueRef right = processExpression(data, expr->right, a);
// 		res = LLVMBuildBinOp(data->builder, expr->kind - nkSmmAdd +
// LLVMAdd, left, right, ""); 		break;
// 	}

// clang-format off
static int int_ops_map[] = {
  [TOKEN_PLUS] = LLVMAdd,
  [TOKEN_MINUS] = LLVMSub,
  [TOKEN_STAR] = LLVMMul,
  [TOKEN_SLASH] = LLVMSDiv,
  [TOKEN_MODULO] = LLVMSRem,

  [TOKEN_LT] = LLVMIntSLT,
  [TOKEN_LTE] = LLVMIntSLE,
  [TOKEN_GT] = LLVMIntSGT,
  [TOKEN_GTE] = LLVMIntSGE,
  [TOKEN_EQUALITY] = LLVMIntEQ,
  [TOKEN_NOT_EQUAL] = LLVMIntNE,
};

static int float_ops_map[] = {
  [TOKEN_PLUS] = LLVMFAdd,
  [TOKEN_MINUS] = LLVMFSub,
  [TOKEN_STAR] = LLVMFMul,   
  [TOKEN_SLASH] = LLVMFDiv,
  [TOKEN_MODULO] = LLVMFRem,

  [TOKEN_LT] = LLVMRealOLT,
  [TOKEN_LTE] = LLVMRealOLE,
  [TOKEN_GT] = LLVMRealOGT,
  [TOKEN_GTE] = LLVMRealOGE,
  [TOKEN_EQUALITY] = LLVMRealOEQ,
  [TOKEN_NOT_EQUAL] = LLVMRealONE,

};

// clang-format on

LLVMValueRef codegen_binop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef l = codegen(ast->data.AST_BINOP.left, ctx, module, builder);
  LLVMValueRef r = codegen(ast->data.AST_BINOP.right, ctx, module, builder);

  // fprintf(stderr, "binop l: ");
  // LLVMDumpValue(l);
  // fprintf(stderr, "\n");
  //
  // fprintf(stderr, "binop r: ");
  // LLVMDumpValue(r);
  // fprintf(stderr, "\n");

  if (l == NULL || r == NULL) {
    return NULL;
  }
  token_type op = ast->data.AST_BINOP.op;

  if (LLVMTypeOf(l) == LLVMInt32Type() && LLVMTypeOf(l) == LLVMInt32Type()) {
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
    }
  }

  if (LLVMTypeOf(l) == LLVMFloatType() && LLVMTypeOf(l) == LLVMFloatType()) {
    switch (op) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MODULO: {
      return LLVMBuildBinOp(builder, float_ops_map[ast->data.AST_BINOP.op], l,
                            r, "");
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
}
