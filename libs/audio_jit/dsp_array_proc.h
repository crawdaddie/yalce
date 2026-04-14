#ifndef _AUDIO_JIT_ARRAY_PROC_H
#define _AUDIO_JIT_ARRAY_PROC_H

#include "../../lang/parse.h"
#include "./compile_synth.h"
#include "./dsp_build_expr.h"
#include <llvm-c/Types.h>
Ast *get_collection_proc_func(Ast *fn_ast);

DspValue call_dsp_list_proc(Ast *fn_ast, Ast *ast, DspBuildCtx *dsp_ctx,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

DspValue dsp_delay_proc(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder);
#endif
