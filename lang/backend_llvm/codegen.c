#include "backend_llvm/codegen.h"
#include "backend_llvm/codegen_binop.h"
#include "backend_llvm/codegen_function.h"
#include "backend_llvm/codegen_match.h"
#include "backend_llvm/codegen_symbols.h"
#include "codegen_list.h"
#include "codegen_tuple.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      val = codegen(stmt, ctx, module, builder);
    }
    return val;
  }
  case AST_INT: {
    return LLVMConstInt(LLVMInt32Type(), ast->data.AST_INT.value, true);
  }

  case AST_NUMBER: {
    return LLVMConstReal(LLVMDoubleType(), ast->data.AST_NUMBER.value);
  }

  case AST_STRING: {
    char *chars = ast->data.AST_STRING.value;
    int length = ast->data.AST_STRING.length;
    ObjString vstr = (ObjString){
        .chars = chars, .length = length, .hash = hash_string(chars, length)};
    return LLVMBuildGlobalStringPtr(builder, chars, ".str");
  }

  case AST_BOOL: {
    return LLVMConstInt(LLVMInt1Type(), ast->data.AST_BOOL.value, false);
  }
  case AST_BINOP: {
    return codegen_binop(ast, ctx, module, builder);
  }

  case AST_TUPLE: {
    return codegen_tuple(ast, ctx, module, builder);
  }

  case AST_LIST: {
    return codegen_list(ast, ctx, module, builder);
  }

  case AST_LET: {
    return codegen_assignment(ast, ctx, module, builder);
  }
  case AST_IDENTIFIER: {
    return codegen_identifier(ast, ctx, module, builder);
  }

  case AST_LAMBDA: {
    return codegen_fn(ast, ctx, module, builder);
  }

  case AST_APPLICATION: {
    return codegen_fn_application(ast, ctx, module, builder);
  }

  case AST_EXTERN_FN: {
    return codegen_extern_fn(ast, ctx, module, builder);
  }
  case AST_MATCH: {
    return codegen_match(ast, ctx, module, builder);
  }
  }

  return NULL;
}
