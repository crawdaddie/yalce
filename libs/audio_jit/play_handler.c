#include "./play_handler.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/serde.h"
#include "../../lang/types/type_ser.h"
#include <string.h>
LLVMValueRef play_module_handler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  print_ast(ast);
  // if (ast->tag != AST_APPLICATION) {
  //
  //
  //   return NULL;
  // }
  //
  // Ast *expr = ast->data.AST_APPLICATION.args;
  // if (expr->tag == AST_BODY) {
  //   printf("play body\n");
  //   print_ast(ast);
  //   return NULL;
  // }
  //
  // if (expr->tag == AST_APPLICATION && expr->type->kind == T_NUM) {
  //   Ast f = (Ast){.tag = AST_IDENTIFIER,
  //                 .data = {.AST_IDENTIFIER = {"play_node", 9}},
  //                 .type = ast->data.AST_APPLICATION.function->type};
  //   ast->data.AST_APPLICATION.function = &f;
  //
  //   return codegen(ast, ctx, module, builder);
  // }
}
