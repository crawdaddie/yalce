#include "./play_handler.h"
#include "../../lang/serde.h"
LLVMValueRef play_module_handler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {

  printf("play whatever this is?????\n");
  print_ast(ast);
  return NULL;
}
