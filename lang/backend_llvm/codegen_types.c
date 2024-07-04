#include "backend_llvm/codegen_types.h"
#include "llvm-c/Core.h"

LLVMTypeRef type_to_llvm_type(Type *type) {
  switch (type->kind) {

  case T_INT: {
    return LLVMInt32Type();
  }

  case T_NUM: {
    return LLVMDoubleType();
  }

  case T_BOOL: {
    return LLVMInt1Type();
  }

  case T_STRING: {
    return LLVMPointerType(LLVMInt8Type(), 0);
  }

  default: {
    return LLVMInt32Type();
  }
  }
}
