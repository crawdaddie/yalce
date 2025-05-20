#include "backend_llvm/codegen.h"
#include "backend_llvm/application.h"
#include "backend_llvm/array.h"
#include "backend_llvm/function.h"
#include "backend_llvm/list.h"
#include "backend_llvm/match.h"
#include "backend_llvm/strings.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/tuple.h"
#include "backend_llvm/types.h"
#include "coroutines.h"
#include "loop.h"
#include "module.h"
#include "modules.h"
#include "types/inference.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef GenericConsConstructorHandler(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  Type *expected_type = ast->md;
  if (expected_type->kind == T_CONS) {
    LLVMTypeRef struct_type = named_struct_type(
        expected_type->data.T_CONS.name, expected_type, ctx->env, module);

    LLVMValueRef tuple = LLVMGetUndef(struct_type);
    // LLVMConstNull(struct_type);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Ast *arg = ast->data.AST_APPLICATION.args + i;
      LLVMValueRef item_val = codegen(arg, ctx, module, builder);
      tuple = LLVMBuildInsertValue(builder, tuple, item_val, i, "");
    }

    return tuple;
  } else {
    fprintf(stderr,
            "Not Implemented error - constructor handler for non cons types\n");
    return NULL;
  }
}

LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *t = ast->md;
  LLVMTypeRef ret = LLVMVoidType();
  // if (t->kind == T_FN && is_generic(t)) {
  //   ret = LLVMVoidType();
  // } else if (is_generic(t)) {
  //   ret = LLVMVoidType();
  // } else if (is_module(t)) {
  //   ret = LLVMInt32Type();
  // } else {
  //   ret = FIND_TYPE(t, ctx->env, module, ast);
  // }

  LLVMTypeRef funcType = LLVMFunctionType(ret, NULL, 0, 0);

  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);

  if (func == NULL) {
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);

  // if (body == NULL) {
  //   LLVMDeleteFunction(func);
  //   return NULL;
  // }

  // *ret_type = LLVMTypeOf(body);
  // if (types_equal(ast->md, &t_void)) {
  //   *ret_type = LLVMVoidType();
  //   LLVMBuildRetVoid(builder);
  // } else {
  //   LLVMBuildRet(builder, body);
  // }
  //
  LLVMBuildRetVoid(builder);

  return func;
}
Ast *__current_ast;

void print_codegen_location() { print_location(__current_ast); }
LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {

  __current_ast = ast;

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
      Type *t = item->md;

      if (t->kind == T_VAR) {
        // print_type(t);
        // print_type_env(ctx->env);
        t = env_lookup(ctx->env, t->data.T_VAR);
      }

      LLVMValueRef str_val =
          llvm_string_serialize(val, t, ctx, module, builder);

      strings_to_concat[i] = str_val;
    }
    LLVMValueRef concat_strings =
        stream_string_concat(strings_to_concat, len, module, builder);

    return concat_strings;
  }

  case AST_BOOL: {
    return LLVMConstInt(LLVMInt1Type(), ast->data.AST_BOOL.value, false);
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
    Type *tuple_type = ast->md;
    return codegen_tuple(ast, ctx, module, builder);
  }

  case AST_LIST: {
    return codegen_list(ast, ctx, module, builder);
  }

  case AST_ARRAY: {
    return codegen_create_array(ast, ctx, module, builder);
  }

  case AST_LET: {
    return codegen_let_expr(ast, ctx, module, builder);
  }

  case AST_IDENTIFIER: {
    return codegen_identifier(ast, ctx, module, builder);
  }

  case AST_LAMBDA: {
    return codegen_fn(ast, ctx, module, builder);
  }

  case AST_APPLICATION: {
    return codegen_application(ast, ctx, module, builder);
  }

  case AST_EXTERN_FN: {
    return codegen_extern_fn(ast, ctx, module, builder);
  }

  case AST_MATCH: {
    return codegen_match(ast, ctx, module, builder);
  }
  case AST_VOID: {
    return LLVMGetUndef(LLVMVoidType());
  }

  case AST_RECORD_ACCESS: {
    Ast *record = ast->data.AST_RECORD_ACCESS.record;

    Type *record_type = record->md;
    if (record_type->kind == T_CONS &&
        strcmp(record_type->data.T_CONS.name, TYPE_NAME_MODULE) == 0) {
      LLVMValueRef val = codegen_module_access(
          record, record_type, ast->data.AST_RECORD_ACCESS.index,
          ast->data.AST_RECORD_ACCESS.member, ast->md, ctx, module, builder);
      return val;
    }

    LLVMValueRef rec = codegen(record, ctx, module, builder);

    const char *member_name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;
    int member_idx = ast->data.AST_RECORD_ACCESS.index;

    if (member_idx < 0) {
      fprintf(stderr, "Error: no member %s in obj\n", member_name);
      return NULL;
    }

    return codegen_tuple_access(
        member_idx, rec, type_to_llvm_type(record_type, ctx->env, module),
        builder);
  }

  case AST_YIELD: {
    return codegen_yield(ast, ctx, module, builder);
  }
  case AST_EMPTY_LIST: {
    Type *t = ast->md;
    LLVMTypeRef lt = FIND_TYPE(t->data.T_CONS.args[0], ctx->env, module, ast);
    return null_node(llnode_type(lt));
  }

  case AST_TYPE_DECL: {
    Type *t = ast->md;
    if (t->kind == T_CREATE_NEW_GENERIC) {
      Type *tpl = t->data.T_CREATE_NEW_GENERIC.template;
      Type *resolved = t->data.T_CREATE_NEW_GENERIC.fn(tpl);
      if (resolved->kind == T_CONS) {
        t = resolved;
      }
    }

    if (is_generic(t) && t->kind == T_CONS) {
      const char *id = ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;

      JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, t, NULL, NULL);

      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
          GenericConsConstructorHandler;

      ht *stack = (ctx->frame->table);
      ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);
    } else if (!is_generic(t) && t->kind == T_CONS) {
      const char *id = ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;

      JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, t, NULL, NULL);

      sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
          GenericConsConstructorHandler;

      ht *stack = (ctx->frame->table);
      ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);
    } else if (!is_variant_type(t)) {
      fprintf(stderr,
              "Warning - constructor not implemented for type declaration ");
      print_ast_err(ast);
      // print_type_err(ast->md);
      return NULL;
    }

    break;
  }

  case AST_IMPORT: {
    codegen_import(ast, NULL, ctx, module, builder);
    return LLVMConstInt(LLVMInt32Type(), 1, 0);
  }
  // case AST_LOOP: {
  //   printf("codegen loop ????\n");
  //   print_ast(ast);
  //   print_type(ast->md);
  //   return NULL;
  // }
  case AST_LOOP: {
    return codegen_loop(ast, ctx, module, builder);
  }

  case AST_BINOP: {
    return NULL;
  }
  }

  return NULL;
}
