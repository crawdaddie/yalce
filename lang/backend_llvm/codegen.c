#include "backend_llvm/codegen.h"
#include "backend_llvm/binop.h"
#include "backend_llvm/function.h"
#include "backend_llvm/list.h"
#include "backend_llvm/match.h"
#include "backend_llvm/strings.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/tuple.h"
#include "backend_llvm/types.h"
#include "backend_llvm/util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef ret = type_to_llvm_type(ast->md, ctx->env, module);

  LLVMTypeRef funcType = LLVMFunctionType(ret, NULL, 0, 0);

  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);

  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);

    return NULL;
  }

  *ret_type = LLVMTypeOf(body);
  LLVMBuildRet(builder, body);
  return func;
}

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

  case AST_DOUBLE: {
    return LLVMConstReal(LLVMDoubleType(), ast->data.AST_DOUBLE.value);
  }

  case AST_STRING: {
    return codegen_string(ast, ctx, module, builder);
  }

  case AST_CHAR: {
    const char ch = ast->data.AST_CHAR.value;
    return LLVMConstInt(LLVMInt8Type(), ch, 0);
  }

  case AST_FMT_STRING: {
    int len = ast->data.AST_LIST.len;
    LLVMValueRef strings_to_concat[len];
    for (int i = 0; i < len; i++) {
      Ast *item = ast->data.AST_LIST.items + i;

      LLVMValueRef val = codegen(item, ctx, module, builder);
      LLVMValueRef str_val =
          llvm_string_serialize(val, item->md, module, builder);
      strings_to_concat[i] = str_val;
    }
    LLVMValueRef concat_strings =
        stream_string_concat(strings_to_concat, len, module, builder);

    return concat_strings;
  }

  case AST_BOOL: {
    return LLVMConstInt(LLVMInt1Type(), ast->data.AST_BOOL.value, false);
  }

  case AST_BINOP: {
    return codegen_binop(ast, ctx, module, builder);
  }

  case AST_UNOP: {
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_STAR: {
      break;
    }

    case TOKEN_AMPERSAND: {
      break;
    }
    }
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

    // case AST_TYPE_DECL: {
    //   return codegen_type_declaration(ast, ctx, module, builder);
    // }
  }

  return NULL;
}
