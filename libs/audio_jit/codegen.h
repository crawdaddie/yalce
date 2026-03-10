#pragma once

#include "codegen_context.h"
#include "codegen_utils.h"

// Consolidated entrypoint header for split audio_jit codegen units.
// As handlers are moved out of audio_jit.cpp, their declarations should be
// added here so units can share them without circular includes.

#ifndef SQ_TABSIZE
#define SQ_TABSIZE (1 << 11)
#endif
#ifndef SIN_TABSIZE
#define SIN_TABSIZE (1 << 11)
#endif
#ifndef SAW_TABSIZE
#define SAW_TABSIZE (1 << 11)
#endif
extern const double sin_table[SIN_TABSIZE];
extern const double sq_table[SQ_TABSIZE];
extern const double saw_table[SAW_TABSIZE];

mlir::Value SinOscHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                          JITLangCtx *jit_ctx);
mlir::Value SqOscHandler(Ast *ast, mlir::DspBuildCtx &ctx, JITLangCtx *jit_ctx);
mlir::Value SawOscHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                          JITLangCtx *jit_ctx);
mlir::Value PhasorHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                          JITLangCtx *jit_ctx);
mlir::Value ImpulseHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                           JITLangCtx *jit_ctx);
mlir::Value PhasorTrigHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                              JITLangCtx *jit_ctx);

mlir::Value LinscaleHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                            JITLangCtx *jit_ctx);
mlir::Value BipolarLinscaleHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                                   JITLangCtx *jit_ctx);
mlir::Value UnipolarLinscaleHandler(Ast *ast, mlir::DspBuildCtx &ctx,
                                    JITLangCtx *jit_ctx);
mlir::Value FoldHandler(Ast *ast, mlir::DspBuildCtx &ctx, JITLangCtx *jit_ctx);
mlir::Value WrapHandler(Ast *ast, mlir::DspBuildCtx &ctx, JITLangCtx *jit_ctx);
