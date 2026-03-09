#pragma once

#include "codegen_context.h"

// Shared state allocation helpers for AST visitor handlers.
int reserve_state(mlir::DspBuildCtx &ctx, int bytes);
int reserve_state_aligned(mlir::DspBuildCtx &ctx, int bytes, int align);

// Core AST -> DSP/LLVM expression builder used by all visitor handlers.
mlir::Value build_dsp_expr(Ast *ast, mlir::DspBuildCtx &ctx,
                           JITLangCtx *jit_ctx);
