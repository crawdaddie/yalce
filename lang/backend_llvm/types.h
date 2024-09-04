#ifndef _LANG_BACKEND_LLVM_CODEGEN_TYPES_H
#define _LANG_BACKEND_LLVM_CODEGEN_TYPES_H
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMTypeRef type_to_llvm_type(Type *type, TypeEnv *env);

LLVMValueRef attempt_value_conversion(LLVMValueRef value, Type *type_from,
                                      Type *type_to, LLVMModuleRef module,
                                      LLVMBuilderRef builder);
void initialize_ptr_constructor();

void initialize_double_constructor();

// Define the function pointer type
typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);
LLVMValueRef double_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef ptr_constructor(LLVMValueRef val, Type *from_type,
                             LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef uint64_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder);

void initialize_builtin_numeric_types(TypeEnv *env);

LLVMTypeRef llvm_type_of_identifier(Ast *id, TypeEnv *env);
#endif
