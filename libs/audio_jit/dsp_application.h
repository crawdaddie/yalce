#include "../../lang/parse.h"
#include "./compile_synth.h"
#include <llvm-c/Types.h>
LLVMValueRef dsp_build_application(Ast *ast, DspBuildCtx *dsp_ctx,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder);

bool ast_is_const(Ast *ast, JITLangCtx *jit_ctx);
