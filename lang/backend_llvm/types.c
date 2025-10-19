#include "backend_llvm/types.h"
#include "adt.h"
#include "backend_llvm/array.h"
#include "closures.h"
#include "codegen.h"
#include "common.h"
#include "list.h"
#include "types/inference.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <string.h>

#define LLVM_TYPE_int LLVMInt32Type()
#define LLVM_TYPE_uint64 LLVMInt64Type()
#define LLVM_TYPE_bool LLVMInt1Type()
#define LLVM_TYPE_float LLVMFloatType()
#define LLVM_TYPE_double LLVMDoubleType()
#define LLVM_TYPE_void LLVMVoidType()
#define LLVM_TYPE_str LLVMPointerType(LLVMInt8Type(), 0)
#define LLVM_TYPE_char LLVMInt8Type()
#define LLVM_TYPE_ptr(type) LLVMPointerType(LLVM_TYPE_##type, 0)

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, JITLangCtx *ctx,
                            LLVMModuleRef module);

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type, JITLangCtx *ctx,
                       LLVMModuleRef module) {
  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef element_types[len];

  for (int i = 0; i < len; i++) {

    if (tuple_type->data.T_CONS.args[i]->kind == T_FN) {
      element_types[i] = GENERIC_PTR;
    } else {
      element_types[i] =
          type_to_llvm_type(tuple_type->data.T_CONS.args[i], ctx, module);
    }
  }
  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);
  // printf("llvm tuple type\n");
  // LLVMDumpType(llvm_tuple_type);
  // printf("\n");

  return llvm_tuple_type;
}

LLVMTypeRef named_struct_type(const char *name, Type *tuple_type,
                              JITLangCtx *ctx, LLVMModuleRef module) {
  int len = tuple_type->data.T_CONS.num_args;
  LLVMTypeRef element_types[len];
  for (int i = 0; i < len; i++) {

    if (tuple_type->data.T_CONS.args[i]->kind == T_FN) {
      element_types[i] = GENERIC_PTR;
    } else {
      element_types[i] =
          type_to_llvm_type(tuple_type->data.T_CONS.args[i], ctx, module);
    }
  }
  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);

  return llvm_tuple_type;
}

LLVMTypeRef create_llvm_list_type(Type *list_el_type, JITLangCtx *ctx,
                                  LLVMModuleRef module);

LLVMTypeRef type_to_llvm_type(Type *type, JITLangCtx *ctx,
                              LLVMModuleRef module) {
  if (!type) {
    return NULL;
  }

  // LLVMTypeRef variant = variant_member_to_llvm_type(type, env, module);
  // if (variant) {
  //   return variant;
  // }

  switch (type->kind) {

  case T_INT: {
    return LLVM_TYPE_int;
  }

  case T_UINT64: {
    return LLVMInt64Type();
  }

  case T_NUM: {
    return LLVM_TYPE_double;
  }

  case T_BOOL: {
    return LLVM_TYPE_bool;
  }

  case T_CHAR: {
    return LLVM_TYPE_char;
  }

  case T_VAR: {
    if (ctx->env) {
      Type *lu = env_lookup(ctx->env, type->data.T_VAR);

      if (!lu) {

        // print_type_env(ctx->env);
        fprintf(stderr,
                "Error type var %s not found in environment! [compiler source "
                ": %s:%d]\n",
                type->data.T_VAR, __FILE__, __LINE__);
        print_location(__current_ast);
        return NULL;
        // return type_to_llvm_type(&t_string, ctx, module);
      }

      if (lu->kind == T_VAR && types_equal(lu, type)) {
        fprintf(stderr,
                "Error: (circular ref??) type %s not found in env! [compiler "
                "source: [%s:%d]\n",
                type->data.T_VAR, __FILE__, __LINE__);

        print_location(__current_ast);
        return NULL;
      }
      return type_to_llvm_type(lu, ctx, module);
    }
    return LLVMInt32Type();
  }

  case T_TYPECLASS_RESOLVE: {
    type = resolve_tc_rank_in_env(type, ctx->env);
    return type_to_llvm_type(type, ctx, module);
  }

  case T_CONS: {

    if (is_array_type(type)) {
      if (type->data.T_CONS.args[0]->kind == T_VAR) {
        return tmp_generic_codegen_array_type();
      }
      LLVMTypeRef el_type;
      el_type = type_to_llvm_type(type->data.T_CONS.args[0], ctx, module);

      return el_type ? codegen_array_type(el_type) : NULL;
    }
    if (is_tuple_type(type)) {
      return tuple_type(type, ctx, module);
    }

    if (is_list_type(type)) {
      // if (type->data.T_CONS.args[0]->kind == T_CHAR) {
      //   return LLVMPointerType(LLVMInt8Type(), 0);
      // }

      return create_llvm_list_type(type->data.T_CONS.args[0], ctx, module);
    }

    if (is_pointer_type(type)) {
      return LLVM_TYPE_ptr(char);
    }

    // if (type->data.T_CONS.num_args == 1) {
    //   // this is maybe not legit???
    //   return type_to_llvm_type(type->data.T_CONS.args[0], ctx, module);
    // }

    if (strcmp(type->data.T_CONS.name, TYPE_NAME_VARIANT) == 0) {
      if (is_simple_enum(type)) {
        return LLVMInt8Type();
      } else {
        Type *member_type = type->data.T_CONS.args[0];
        const char *member_type_name = member_type->data.T_CONS.name;
        // printf("member %s\n", member_type_name);
        // print_type(type);
        JITSymbol *sym =
            find_in_ctx(member_type_name, strlen(member_type_name), ctx);
        if (sym && sym->type == STYPE_VARIANT_TYPE) {
          return sym->llvm_type;
        }
        return codegen_adt_type(type, ctx, module);
      }
    }

    // if (type->data.T_CONS.num_args == 0) {
    //   return NULL;
    // }

    return tuple_type(type, ctx, module);
  }

  case T_FN: {
    if (is_closure(type)) {

      LLVMTypeRef clos_env_type = closure_record_type(type, ctx, module);
      LLVMTypeRef clos_impl_type =
          closure_fn_type(type, clos_env_type, ctx, module);

      return LLVMStructType(
          (LLVMTypeRef[]){clos_impl_type, LLVMPointerType(clos_env_type, 0)}, 2,
          0);
    }
    Type *t = type;
    int fn_len = 0;

    for (Type *t = type; t->kind == T_FN && !(is_closure(t));
         t = t->data.T_FN.to, fn_len++) {
    }
    return codegen_fn_type(type, fn_len, ctx, module);
  }

  default: {
    return LLVMVoidType();
  }
  }

  if (is_generic(type)) {
    return NULL;
  }
}
