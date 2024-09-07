#include "backend_llvm/binop.h"
#include "backend_llvm/common.h"
#include "backend_llvm/types.h"
#include "parse.h"
#include "serde.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_binop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  Type *result_type = ast->md;
  result_type = resolve_tc_rank(result_type);
  if (is_generic(result_type)) {
    result_type = resolve_generic_type(result_type, ctx->env);
  }

  token_type op = ast->data.AST_BINOP.op;

  LLVMValueRef l = codegen(ast->data.AST_BINOP.left, ctx, module, builder);
  Type *ltype = ast->data.AST_BINOP.left->md;
  ltype = resolve_tc_rank(ltype);
  if (is_generic(ltype)) {
    ltype = resolve_generic_type(ltype, ctx->env);
  }

  LLVMValueRef r = codegen(ast->data.AST_BINOP.right, ctx, module, builder);

  Type *rtype = ast->data.AST_BINOP.right->md;
  rtype = resolve_tc_rank(rtype);
  if (is_generic(rtype)) {
    rtype = resolve_generic_type(rtype, ctx->env);
  }

  Method method;
  switch (op) {
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL: {
    // typeclass Eq
    method = result_type->implements[0]->methods[op - TOKEN_EQUALITY];
    break;
  }
  case TOKEN_LT:
  case TOKEN_GT:
  case TOKEN_LTE:
  case TOKEN_GTE: {
    // typeclass Ord
    method = result_type->implements[1]->methods[op - TOKEN_LT];
    break;
  }
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    // typeclass Arithmetic
    method = result_type->implements[2]->methods[op - TOKEN_PLUS];
    break;
  }

  default:
    return NULL;
  }

  Type *expected_ltype = method.signature->data.T_FN.from;
  if (!(types_equal(ltype, expected_ltype))) {
    l = attempt_value_conversion(l, ltype, expected_ltype, module, builder);
  }

  Type *expected_rtype = method.signature->data.T_FN.to->data.T_FN.from;

  if (!(types_equal(rtype, expected_rtype))) {
    r = attempt_value_conversion(r, rtype, expected_rtype, module, builder);
  }

  LLVMBinopMethod binop_method = method.method;
  return binop_method(l, r, module, builder);
}
