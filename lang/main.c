#ifdef VM_BACKEND
#include "backend_vm.h"
#elif defined(LLVM_BACKEND)
#include "backend_llvm/backend.h"
#else
#include "backend.h"
#endif

int main(int argc, char **argv) {

#ifdef VM_BACKEND
  return interpreter_vm(argc, argv);
#elif defined(LLVM_BACKEND)
  return jit(argc, argv);
#else
  return interpreter(argc, argv);
#endif
}
