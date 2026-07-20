#include "backend_llvm/types.h"
#include "adt.h"
#include "backend_llvm/array.h"
#include "closures.h"
#include "codegen.h"
#include "common.h"
#include "list.h"
#include "types/inference.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <string.h>

LLVMTypeRef codegen_fn_type(LLVMTypeRef _ret_type, Type *fn_type, int fn_len,
                            JITLangCtx *ctx, LLVMModuleRef module);

static LLVMContextRef module_context(LLVMModuleRef module) {
  return module ? LLVMGetModuleContext(module) : LLVMGetGlobalContext();
}

static LLVMTypeRef generic_ptr_type(LLVMModuleRef module) {
  return LLVMPointerType(LLVMInt8TypeInContext(module_context(module)), 0);
}

typedef struct RecursiveStructBuildFrame {
  const char *name;
  LLVMModuleRef module;
  struct RecursiveStructBuildFrame *next;
} RecursiveStructBuildFrame;

static RecursiveStructBuildFrame *recursive_struct_build_stack = NULL;

static bool is_building_recursive_struct(const char *name,
                                         LLVMModuleRef module) {
  for (RecursiveStructBuildFrame *frame = recursive_struct_build_stack; frame;
       frame = frame->next) {
    if (frame->module == module && frame->name && name &&
        strcmp(frame->name, name) == 0) {
      return true;
    }
  }
  return false;
}

static const char *type_codegen_name(Type *type) {
  if (!type) {
    return NULL;
  }

  if (type->kind == T_RECURSIVE_REF) {
    return type->data.T_RECURSIVE_REF.name;
  }

  if (type->alias) {
    return type->alias;
  }

  if (type->kind == T_CONS || type->kind == T_SUM) {
    return type->data.T_CONS.name;
  }

  return NULL;
}

static bool type_contains_recursive_ref_name(Type *type, const char *name) {
  if (!type || !name) {
    return false;
  }

  switch (type->kind) {
  case T_RECURSIVE_REF:
    return type->data.T_RECURSIVE_REF.name &&
           strcmp(type->data.T_RECURSIVE_REF.name, name) == 0;

  case T_VAR:
    return type->is_recursive_type_ref && type->data.T_VAR.name &&
           strcmp(type->data.T_VAR.name, name) == 0;

  case T_CONS:
  case T_SUM:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (type_contains_recursive_ref_name(type->data.T_CONS.args[i], name)) {
        return true;
      }
    }
    return false;

  case T_FN:
    return type_contains_recursive_ref_name(type->data.T_FN.from, name) ||
           type_contains_recursive_ref_name(type->data.T_FN.to, name) ||
           type_contains_recursive_ref_name(type->closure_meta, name);

  default:
    return false;
  }
}

static bool type_contains_unwrapped_recursive_ref_name(Type *type,
                                                       const char *name) {
  if (!type || !name) {
    return false;
  }

  switch (type->kind) {
  case T_RECURSIVE_REF:
    return type->data.T_RECURSIVE_REF.name &&
           strcmp(type->data.T_RECURSIVE_REF.name, name) == 0;

  case T_VAR:
    return type->is_recursive_type_ref && type->data.T_VAR.name &&
           strcmp(type->data.T_VAR.name, name) == 0;

  case T_FN:
    return false;

  case T_CONS:
  case T_SUM:
    if (is_array_type(type) || is_list_type(type) || is_pointer_type(type) ||
        is_coroutine_type(type)) {
      return false;
    }

    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (type_contains_unwrapped_recursive_ref_name(
              type->data.T_CONS.args[i], name)) {
        return true;
      }
    }
    return false;

  default:
    return false;
  }
}

static bool type_uses_named_recursive_storage(Type *type) {
  if (!type || type->kind != T_CONS || !type->alias) {
    return false;
  }

  if (is_array_type(type) || is_list_type(type) || is_pointer_type(type) ||
      is_coroutine_type(type)) {
    return false;
  }

  return type_contains_recursive_ref_name(type, type->alias);
}

bool type_uses_boxed_recursive_storage(Type *type) {
  if (!type || type->kind != T_CONS || !type->alias) {
    return false;
  }

  return type_contains_unwrapped_recursive_ref_name(type, type->alias);
}

static LLVMTypeRef get_or_create_named_struct(const char *name,
                                              LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef existing = LLVMGetTypeByName2(llvm_ctx, name);
  return existing ? existing : LLVMStructCreateNamed(llvm_ctx, name);
}

static LLVMTypeRef recursive_ref_aggregate_type(Type *type, JITLangCtx *ctx,
                                                LLVMModuleRef module);

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type, JITLangCtx *ctx,
                       LLVMModuleRef module) {
  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef element_types[len];

  for (int i = 0; i < len; i++) {

    if (tuple_type->data.T_CONS.args[i]->kind == T_FN &&
        !is_closure(tuple_type->data.T_CONS.args[i])) {
      element_types[i] = generic_ptr_type(module);
    } else {
      element_types[i] =
          type_to_llvm_type(tuple_type->data.T_CONS.args[i], ctx, module);
    }
  }
  LLVMTypeRef llvm_tuple_type =
      LLVMStructTypeInContext(module_context(module), element_types, len, 0);
  // printf("llvm tuple type\n");
  // LLVMDumpType(llvm_tuple_type);
  // printf("\n");

  return llvm_tuple_type;
}

LLVMTypeRef named_struct_type(const char *name, Type *tuple_type,
                              JITLangCtx *ctx, LLVMModuleRef module) {
  if (type_uses_named_recursive_storage(tuple_type)) {
    return recursive_ref_aggregate_type(tuple_type, ctx, module);
  }

  int len = tuple_type->data.T_CONS.num_args;
  LLVMTypeRef element_types[len];
  for (int i = 0; i < len; i++) {

    if (tuple_type->data.T_CONS.args[i]->kind == T_FN &&
        !is_closure(tuple_type->data.T_CONS.args[i])) {
      element_types[i] = generic_ptr_type(module);
    } else {
      element_types[i] =
          type_to_llvm_type(tuple_type->data.T_CONS.args[i], ctx, module);
    }
  }
  LLVMTypeRef llvm_tuple_type =
      LLVMStructTypeInContext(module_context(module), element_types, len, 0);

  return llvm_tuple_type;
}

static LLVMTypeRef recursive_ref_aggregate_type(Type *type, JITLangCtx *ctx,
                                                LLVMModuleRef module) {
  const char *name = type_codegen_name(type);
  if (!name) {
    return NULL;
  }

  Type *decl_type = type;
  if (type->kind == T_RECURSIVE_REF) {
    TypeEnv *decl = type->data.T_RECURSIVE_REF.decl;
    decl_type = decl ? decl->type : NULL;
  }

  LLVMTypeRef llvm_struct = get_or_create_named_struct(name, module);
  if (!decl_type || decl_type->kind != T_CONS ||
      !LLVMIsOpaqueStruct(llvm_struct)) {
    return llvm_struct;
  }

  if (is_building_recursive_struct(name, module)) {
    return llvm_struct;
  }

  RecursiveStructBuildFrame build_frame = {
      .name = name,
      .module = module,
      .next = recursive_struct_build_stack,
  };
  recursive_struct_build_stack = &build_frame;

  int len = decl_type->data.T_CONS.num_args;
  LLVMTypeRef element_types[len];
  for (int i = 0; i < len; i++) {
    Type *field_type = decl_type->data.T_CONS.args[i];
    if (field_type->kind == T_FN && !is_closure(field_type)) {
      element_types[i] = generic_ptr_type(module);
    } else {
      element_types[i] = type_to_llvm_type(field_type, ctx, module);
    }
  }

  LLVMStructSetBody(llvm_struct, element_types, len, 0);
  recursive_struct_build_stack = build_frame.next;
  return llvm_struct;
}

LLVMTypeRef type_to_llvm_aggregate_type(Type *type, JITLangCtx *ctx,
                                        LLVMModuleRef module) {
  if (type_uses_named_recursive_storage(type)) {
    return recursive_ref_aggregate_type(type, ctx, module);
  }

  if (type && type->kind == T_RECURSIVE_REF) {
    return recursive_ref_aggregate_type(type, ctx, module);
  }

  return type_to_llvm_type(type, ctx, module);
}

LLVMTypeRef create_llvm_list_type(Type *list_el_type, JITLangCtx *ctx,
                                  LLVMModuleRef module);

Type *specialize_type_for_codegen(Type *type, JITLangCtx *ctx) {
  if (!type) {
    return NULL;
  }

  if (!is_generic(type) && !type->closure_meta) {
    return type;
  }

  Type *specialized = type;
  if (ctx->type_subst) {
    specialized = apply_subst_to_type(ctx->type_subst, deep_copy_type(type));
  }

  if (ctx->env) {
    specialized = resolve_type_in_env(specialized, ctx->env);
  }

  return specialized;
}

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
    return LLVMInt32TypeInContext(module_context(module));
  }

  case T_UINT64: {
    return LLVMInt64TypeInContext(module_context(module));
  }

  case T_NUM: {
    return LLVMDoubleTypeInContext(module_context(module));
  }

  case T_BOOL: {
    return LLVMInt1TypeInContext(module_context(module));
  }

  case T_CHAR: {
    return LLVMInt8TypeInContext(module_context(module));
  }

  case T_VAR: {
    if (ctx->type_subst) {
      Type *resolved = find_in_subst(ctx->type_subst, type->data.T_VAR.id);
      if (resolved &&
          !(resolved->kind == T_VAR && types_equal(resolved, type))) {
        return type_to_llvm_type(resolved, ctx, module);
      }
    }

    if (type->is_recursive_type_ref && ctx->env) {
      Type *lu = env_lookup(ctx->env, type->data.T_VAR.name);

      if (!lu) {
        fprintf(stderr,
                "Error recursive type var %s not found in environment! "
                "[compiler source : %s:%d]\n",
                type->data.T_VAR.name, __FILE__, __LINE__);
        print_location(__current_ast);
        return NULL;
      }

      if (lu->kind == T_VAR && types_equal(lu, type)) {
        fprintf(stderr,
                "Error: (circular ref??) type %s not found in env! [compiler "
                "source: [%s:%d]\n",
                type->data.T_VAR.name, __FILE__, __LINE__);
        print_location(__current_ast);
        return NULL;
      }
      return type_to_llvm_type(lu, ctx, module);
    }

    // Ordinary HM type variables are specialization-time placeholders, not
    // lexical env-bound names. If one survives to codegen, treat it as opaque.
    return generic_ptr_type(module);
  }

  case T_RECURSIVE_REF: {
    LLVMTypeRef aggregate = recursive_ref_aggregate_type(type, ctx, module);
    TypeEnv *decl = type->data.T_RECURSIVE_REF.decl;
    Type *decl_type = decl ? decl->type : NULL;
    if (aggregate && type_uses_boxed_recursive_storage(decl_type)) {
      return LLVMPointerType(aggregate, 0);
    }
    return aggregate ? aggregate : generic_ptr_type(module);
  }

  case T_SUM: {
    if (is_list_type(type)) {
      return create_llvm_list_type(type_of_list(type), ctx, module);
    }

    if (is_option_type(type)) {
      Type *opt_of = type_of_option(type);
      return codegen_option_struct_type(type_to_llvm_type(opt_of, ctx, module));
    }

    if (is_simple_enum(type)) {
      return LLVMInt8TypeInContext(module_context(module));
    }

    return codegen_adt_type(type, ctx, module);
  }

  case T_CONS: {
    if (type_uses_boxed_recursive_storage(type)) {
      LLVMTypeRef aggregate = recursive_ref_aggregate_type(type, ctx, module);
      return aggregate ? LLVMPointerType(aggregate, 0)
                       : generic_ptr_type(module);
    }

    if (type_uses_named_recursive_storage(type)) {
      LLVMTypeRef aggregate = recursive_ref_aggregate_type(type, ctx, module);
      return aggregate ? aggregate : generic_ptr_type(module);
    }

    if (is_coroutine_type(type)) {
      return generic_ptr_type(module);
    }

    if (is_string_type(type)) {
      return codegen_string_type(LLVMInt8TypeInContext(module_context(module)));
    }

    if (is_array_type(type)) {
      if (type->data.T_CONS.args[0]->kind == T_VAR &&
          !type->data.T_CONS.args[0]->is_recursive_type_ref) {
        return codegen_array_type(generic_ptr_type(module));
      }
      LLVMTypeRef el_type;
      el_type = type_to_llvm_type(type->data.T_CONS.args[0], ctx, module);

      return el_type ? codegen_array_type(el_type) : NULL;
    }
    if (is_tuple_type(type)) {
      return tuple_type(type, ctx, module);
    }

    if (is_pointer_type(type)) {
      return generic_ptr_type(module);
    }

    // if (type->data.T_CONS.num_args == 1) {
    //   // this is maybe not legit???
    //   return type_to_llvm_type(type->data.T_CONS.args[0], ctx, module);
    // }

    // if (type->data.T_CONS.num_args == 0) {
    //   return NULL;
    // }

    return tuple_type(type, ctx, module);
  }

  case T_FN: {
    if (is_closure(type)) {
      return get_named_closure_type(module);
    }

    int fn_len = 0;

    for (Type *t = type; t->kind == T_FN && !(is_closure(t));
         t = t->data.T_FN.to, fn_len++) {
    }

    return codegen_fn_type(NULL, type, fn_len, ctx, module);
  }

  default: {
    return LLVMVoidTypeInContext(module_context(module));
  }
  }

  if (is_generic(type)) {
    return NULL;
  }
}
