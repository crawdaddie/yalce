#include "backend_llvm/codegen_match_values.h"
#include "codegen_binop.h"
#include "codegen_list.h"
#include "codegen_tuple.h"
#include "codegen_types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_match_condition(LLVMValueRef expr_val, Ast *pattern,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef codegen_equality(LLVMValueRef left, Type *left_type,
                              LLVMValueRef right, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  if (left_type->kind == T_INT) {
    return codegen_int_binop(builder, TOKEN_EQUALITY, left, right);
  }

  if (left_type->kind == T_NUM) {
    return codegen_float_binop(builder, TOKEN_EQUALITY, left, right);
  }
  return _FALSE;
}

LLVMValueRef match_values(Ast *left, LLVMValueRef right, Type *right_type,
                          LLVMValueRef *res, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (left->tag) {
  case AST_IDENTIFIER: {
    if (*(left->data.AST_IDENTIFIER.value) == '_') {
      return *res;
    }

    LLVMTypeRef llvm_type = LLVMTypeOf(right);
    const char *id_chars = left->data.AST_IDENTIFIER.value;
    int id_len = left->data.AST_IDENTIFIER.length;

    JITSymbol *sym = malloc(sizeof(JITSymbol));
    Type *val_type = right_type;

    if (val_type->kind == T_FN) {
      *sym = (JITSymbol){.type = STYPE_FUNCTION,
                         .llvm_type = llvm_type,
                         .val = right,
                         .symbol_type = val_type};

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

      return right;
    }

    if (ctx->stack_ptr == 0) {

      // top-level
      LLVMValueRef alloca_val =
          LLVMAddGlobalInAddressSpace(module, llvm_type, id_chars, 0);

      LLVMSetInitializer(alloca_val, LLVMConstNull(llvm_type));
      LLVMBuildStore(builder, right, alloca_val);

      // LLVMSetInitializer(global_j, LLVMConstInt(llvm_type, 0, 0));
      /*
    // Check if the value is a pointer type
    if (LLVMGetTypeKind(llvm_type) == LLVMPointerTypeKind) {
      // create a const null pointer because global vars must be initialized
      // with a constant
      // printf("store pointer to global (ptr)\n");
      // LLVMDumpValue(right);
      // printf("\n");
      LLVMSetInitializer(alloca_val, LLVMConstNull(llvm_type));
      LLVMBuildStore(builder, right, alloca_val);

    } else {
      LLVMSetInitializer(alloca_val, right);
    }
    */

      *sym = (JITSymbol){STYPE_TOP_LEVEL_VAR, llvm_type, alloca_val,
                         .symbol_type = left->md};

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

      return *res;
    }

    *sym =
        (JITSymbol){STYPE_LOCAL_VAR, llvm_type, right, .symbol_type = left->md};
    ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                hash_string(id_chars, id_len), sym);
    return *res;
  }
  case AST_BINOP: {
    if (left->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {

      Type *right_list_el_type = right_type->data.T_CONS.args[0];

      Ast *pattern_left = left->data.AST_BINOP.left;
      Ast *pattern_right = left->data.AST_BINOP.right;

      LLVMTypeRef list_el_type = type_to_llvm_type(pattern_left->md, ctx->env);

      *res =
          and_vals(*res, ll_is_not_null(right, list_el_type, builder), builder);

      LLVMValueRef list_head_val =
          ll_get_head_val(right, list_el_type, builder);

      *res =
          and_vals(*res,
                   match_values(pattern_left, list_head_val, right_list_el_type,
                                res, ctx, module, builder),
                   builder);

      LLVMValueRef list_next = ll_get_next(right, list_el_type, builder);

      *res = and_vals(*res,
                      match_values(pattern_right, list_next, right_type, res,
                                   ctx, module, builder),
                      builder);
    }
    return *res;
  }
  case AST_LIST: {
    Type *t = ((Type *)left->md)->data.T_CONS.args[0];
    LLVMTypeRef list_el_type = type_to_llvm_type(t, ctx->env);
    if (left->data.AST_LIST.len == 0) {
      *res = ll_is_null(right, list_el_type, builder);
    } else {
      // TODO: crudely computes all values & comparisons and ands them together
      // would be better to implement this using short-circuiting logic
      int len = left->data.AST_LIST.len;
      LLVMValueRef list = right;
      LLVMValueRef and = _TRUE;
      Type *right_list_el_type = right_type->data.T_CONS.args[0];
      for (int i = 0; i < len; i++) {
        Ast *list_item_ast = left->data.AST_LIST.items + i;
        and =
            and_vals(and, ll_is_not_null(list, list_el_type, builder), builder);

        LLVMValueRef list_head = ll_get_head_val(list, list_el_type, builder);
        and =
            and_vals(and,
                     match_values(list_item_ast, list_head, right_list_el_type,
                                  &and, ctx, module, builder),
                     builder);
        list = ll_get_next(list, list_el_type, builder);
      }

      and = and_vals(and, ll_is_null(list, list_el_type, builder), builder);
      *res = and;
    }
    return *res;
  }

  case AST_TUPLE: {
    int len = left->data.AST_LIST.len;
    LLVMValueRef and = _TRUE;
    for (int i = 0; i < len; i++) {
      Ast *tuple_item_ast = left->data.AST_LIST.items + i;

      LLVMValueRef tuple_item_val =
          codegen_tuple_access(i, right, LLVMTypeOf(right), builder);
      Type *tuple_item_type = right_type->data.T_CONS.args[i];
      and = and_vals(and,
                     match_values(tuple_item_ast, tuple_item_val,
                                  tuple_item_type, &and, ctx, module, builder),
                     builder);
    }
    return *res;
  }

  default: {
    LLVMValueRef left_val = codegen(left, ctx, module, builder);

    *res = and_vals(
        *res, codegen_equality(left_val, left->md, right, ctx, module, builder),
        builder);
    return *res;
  }
  }
  return *res;
}
