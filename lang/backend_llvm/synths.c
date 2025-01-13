#include "backend_llvm/synths.h"
#include "list.h"
#include "tuple.h"
#include "util.h"
#include "llvm-c/Core.h"

LLVMValueRef node_of_val_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("node_of_double", *fn_type, module);
}

LLVMValueRef node_of_sig_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("node_of_sig", *fn_type, module);
}

LLVMValueRef sig_of_val_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);
  return get_extern_fn("signal_of_double", *fn_type, module);
}

LLVMValueRef sig_of_array_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(
      LLVMPointerType(LLVMInt8Type(), 0),
      (LLVMTypeRef[]){LLVMInt32Type(), LLVMPointerType(LLVMDoubleType(), 0)}, 2,
      0);
  return get_extern_fn("sig_of_array", *fn_type, module);
}

LLVMValueRef out_sig_of_node_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("out_sig", *fn_type, module);
}
LLVMValueRef const_node_of_val(LLVMValueRef val, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef node_of_double_func = node_of_val_fn(&fn_type, module);
  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  LLVMValueRef const_node = LLVMBuildCall2(
      builder, fn_type, node_of_double_func, &double_val, 1, "const_node");
  return const_node;
}

LLVMValueRef const_node_of_sig(LLVMValueRef val, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef node_of_sig_func = node_of_sig_fn(&fn_type, module);
  LLVMValueRef const_node =
      LLVMBuildCall2(builder, fn_type, node_of_sig_func, &val, 1, "const_node");
  return const_node;
}

LLVMValueRef const_sig_of_val_int(LLVMValueRef val, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef sig_of_val_func = sig_of_val_fn(&fn_type, module);
  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  LLVMValueRef const_sig = LLVMBuildCall2(builder, fn_type, sig_of_val_func,
                                          &double_val, 1, "sig_of_val");
  return const_sig;
}

LLVMValueRef const_sig_of_val(LLVMValueRef val, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef sig_of_val_func = sig_of_val_fn(&fn_type, module);

  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");

  LLVMValueRef const_sig = LLVMBuildCall2(builder, fn_type, sig_of_val_func,
                                          &double_val, 1, "sig_of_val");
  return const_sig;
}

LLVMValueRef const_sig_of_array(LLVMValueRef val, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef sig_of_array_func = sig_of_array_fn(&fn_type, module);
  LLVMTypeRef struct_type =
      array_struct_type(LLVMPointerType(LLVMDoubleType(), 0));

  LLVMValueRef array_ptr = codegen_tuple_access(1, val, struct_type, builder);
  LLVMValueRef const_sig =
      LLVMBuildCall2(builder, fn_type, sig_of_array_func,
                     (LLVMValueRef[]){
                         codegen_get_array_size(builder, val),
                         array_ptr,
                     },
                     2, "sig_of_array");
  return const_sig;
}

LLVMValueRef out_sig_of_node_val(LLVMValueRef val, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef out_sig_of_node_func = out_sig_of_node_fn(&fn_type, module);
  LLVMValueRef const_sig = LLVMBuildCall2(
      builder, fn_type, out_sig_of_node_func, &val, 1, "out_sig_of_node");
  return const_sig;
}

// Define the function pointer type
typedef LLVMValueRef (*SynthConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                        LLVMBuilderRef);
LLVMValueRef ConsSynth(LLVMValueRef value, Type *type_from,
                       LLVMModuleRef module, LLVMBuilderRef builder) {

  if (type_from->alias && strcmp(type_from->alias, "Signal") == 0) {
    return const_node_of_sig(value, module, builder);
  }

  return const_node_of_val(value, module, builder);
}

LLVMValueRef ConsSignal(LLVMValueRef value, Type *type_from,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  switch (type_from->kind) {
  case T_INT:
  case T_NUM: {

    return const_sig_of_val(value, module, builder);
  }
  case T_CONS: {
    if (type_from->alias && (strcmp(type_from->alias, "Synth") == 0)) {
      return out_sig_of_node_val(value, module, builder);
    }

    if (type_from->alias && (strcmp(type_from->alias, "Signal") == 0)) {
      return value;
    }
    if (is_array_type(type_from) &&
        (type_from->data.T_CONS.args[0]->kind == T_NUM)) {
      return const_sig_of_array(value, module, builder);
    }
  }
  default: {
    return NULL;
  }
  }
}
