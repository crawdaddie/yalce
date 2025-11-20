#include "backend_llvm/codegen.h"
#include "adt.h"
#include "backend_llvm/application.h"
#include "backend_llvm/array.h"
#include "backend_llvm/function.h"
#include "backend_llvm/function_extern.h"
#include "backend_llvm/list.h"
#include "backend_llvm/match.h"
#include "backend_llvm/strings.h"
#include "backend_llvm/symbols.h"
#include "backend_llvm/tuple.h"
#include "backend_llvm/types.h"
#include "builtin_functions.h"
#include "coroutines.h"
#include "loop.h"
#include "module.h"
#include "modules.h"
#include "types/common.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef GenericConsConstructorHandler(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  Type *expected_type = ast->type;
  if (expected_type->kind == T_CONS) {
    LLVMTypeRef struct_type = named_struct_type(expected_type->data.T_CONS.name,
                                                expected_type, ctx, module);

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

  Type *t = ast->type;
  LLVMTypeRef ret = LLVMVoidType();

  LLVMTypeRef funcType = LLVMFunctionType(ret, NULL, 0, 0);

  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);

  if (func == NULL) {
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);

  LLVMBuildRetVoid(builder);

  return func;
}

LLVMValueRef codegen_repl_top_level(Ast *ast, LLVMTypeRef *ret_type,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  Type *t = ast->type;
  LLVMTypeRef ret = LLVMVoidType();

  LLVMTypeRef funcType = LLVMFunctionType(ret, NULL, 0, 0);

  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);

  if (func == NULL) {
    return NULL;
  }
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);

  if (VALUE_IS_PRINTABLE(ast->type)) {

    LLVMValueRef str = stringify_value(body, ast->type, ctx, module, builder);
    print_str(str, ctx, module, builder);
  }

  LLVMBuildRetVoid(builder);

  return func;
}

Ast *__current_ast;
bool is_recursive_datatype(Type *t) {
  if (t->kind == T_VAR && t->is_recursive_type_ref) {
    return true;
  }
  if (t->kind == T_CONS) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      if (is_recursive_datatype(t->data.T_CONS.args[i])) {
        return true;
      }
    }
  }
  return false;
}

void print_codegen_location() { print_location(__current_ast); }
LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {

  __current_ast = ast;

  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    Ast *stmt = l->ast;
                    val = codegen(stmt, ctx, module, builder);
                  }));
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
      Type *t = item->type;

      if (t->kind == T_VAR) {
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
    Type *tuple_type = ast->type;
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

    Type *record_type = record->type;
    JITSymbol *constructor_sym = lookup_id_ast(record, ctx);

    if (constructor_sym) {
    }

    // printf("RECORD ACCESS cons: %p\n", record_type->constructor);
    // print_ast(ast);
    // print_type(record_type);

    if (record_type->kind == T_CONS &&
        strcmp(record_type->data.T_CONS.name, TYPE_NAME_MODULE) == 0) {

      LLVMValueRef val = codegen_module_access(
          record, record_type, ast->data.AST_RECORD_ACCESS.index,
          ast->data.AST_RECORD_ACCESS.member, ast->type, ctx, module, builder);
      return val;
    }

    LLVMValueRef rec = codegen(record, ctx, module, builder);
    // // print_ast(ast);
    // print_ast(record);
    // printf("rec %p\n", rec);

    const char *member_name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;

    int member_idx = ast->data.AST_RECORD_ACCESS.index;

    if (member_idx < 0) {
      fprintf(stderr, "Error: no member %s in obj\n", member_name);
      return NULL;
    }

    return codegen_tuple_access(
        member_idx, rec, type_to_llvm_type(record_type, ctx, module), builder);
  }

  case AST_YIELD: {
    return codegen_yield(ast, ctx, module, builder);
  }

  case AST_TYPE_DECL: {
    Type *t = ast->type;

    if (!is_generic(t) && is_sum_type(t) && is_recursive_datatype(t)) {

      LLVMTypeRef llvm_type = codegen_recursive_datatype(t, ast, ctx, module);

      JITSymbol *sym = new_symbol(STYPE_VARIANT_TYPE, t, NULL, llvm_type);
      Ast *binding = ast->data.AST_LET.binding;

      const char *id_chars = binding->data.AST_IDENTIFIER.value;
      int id_len = binding->data.AST_IDENTIFIER.length;

      ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                  sym);

      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        Ast *mem_ast = ast->data.AST_LET.expr->data.AST_LIST.items + i;
        const char *member_name;
        if (mem_ast->tag == AST_BINOP) {
          member_name = mem_ast->data.AST_BINOP.left->data.AST_IDENTIFIER.value;
        } else if (mem_ast->tag == AST_IDENTIFIER) {
          member_name = mem_ast->data.AST_IDENTIFIER.value;
        } else {
          fprintf(stderr, "Error: cannot create sum type member");
          print_location(mem_ast);
          return NULL;
        }

        int member_name_len = strlen(member_name);

        ht_set_hash(ctx->frame->table, member_name,
                    hash_string(member_name, member_name_len), sym);
      }

      return NULL;
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

    } else if (!is_sum_type(t) && t->kind != T_FN) {
      TypeClass cons = {.name = "Constructor"};
      if (!type_implements(t, &cons)) {
        fprintf(stderr,
                "Warning - constructor not implemented for type declaration ");
        print_ast_err(ast);
        return NULL;
      }
    }

    break;
  }

  case AST_IMPORT: {
    codegen_import(ast, NULL, ctx, module, builder);
    return LLVMConstInt(LLVMInt32Type(), 1, 0);
  }

  case AST_LOOP: {
    return codegen_loop(ast, ctx, module, builder);
  }

  case AST_BINOP: {
    return NULL;
  }

  case AST_GET_ARG: {
    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
    return LLVMGetParam(func, ast->data.AST_GET_ARG.i);
  }

  case AST_TRAIT_IMPL: {
    if (CHARS_EQ(ast->data.AST_TRAIT_IMPL.trait_name.chars, "Constructor")) {
      return create_constructor_module(ast, ctx, module, builder);
    }

    if (CHARS_EQ(ast->data.AST_TRAIT_IMPL.trait_name.chars, "Arithmetic")) {
      return create_arithmetic_typeclass_methods(ast, ctx, module, builder);
    }

    if (ast->type->kind == T_FN) {

      int name_len = ast->data.AST_TRAIT_IMPL.type.length +
                     ast->data.AST_TRAIT_IMPL.trait_name.length + 1;

      char sym_name[name_len + 1];

      sprintf(sym_name, "%s.Str", ast->data.AST_TRAIT_IMPL.type.chars);
      Ast fn_ast = *ast->data.AST_TRAIT_IMPL.impl;
      fn_ast.data.AST_LAMBDA.fn_name =
          (ObjString){.chars = sym_name, .length = name_len};

      Ast binding = (Ast){
          AST_IDENTIFIER,
          .data = {.AST_IDENTIFIER = {.value = sym_name, .length = name_len}}};
      if (!is_generic(ast->type)) {
        LLVMValueRef fn = codegen(&fn_ast, ctx, module, builder);
        create_fn_binding(&binding, ast->type, fn, ctx, module, builder);
      } else {
        create_generic_fn_binding(&binding, &fn_ast, ctx);
      }

      return NULL;
    }

    return NULL;
  }
  }

  return NULL;
}
