#include "backend_llvm/codegen.h"
#include "backend_llvm/codegen_binop.h"
#include "backend_llvm/codegen_function.h"
#include "backend_llvm/codegen_match.h"
#include "backend_llvm/codegen_symbols.h"
#include "codegen_list.h"
#include "codegen_tuple.h"
// #include "types/util.h"
#include "util.h"
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

  case AST_DOUBLE: {
    return LLVMConstReal(LLVMDoubleType(), ast->data.AST_DOUBLE.value);
  }

  case AST_STRING: {
    const char *chars = ast->data.AST_STRING.value;
    int length = ast->data.AST_STRING.length;
    ObjString vstr = (ObjString){
        .chars = chars, .length = length, .hash = hash_string(chars, length)};
    return LLVMBuildGlobalStringPtr(builder, chars, ".str");
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
    print_type_env(ctx->env);
    Type *t = env_lookup(ctx->env, "Option");
    print_type(t);
    printf("\n");

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
