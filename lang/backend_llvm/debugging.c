#include "./debugging.h"
#include "common.h"
#include "config.h"
#include "input.h"
#include "llvm-c/DebugInfo.h"
#include <llvm-c/Core.h>
#include <string.h>

void init_debugging(const char *src_file, JITLangCtx *ctx,
                    LLVMModuleRef module) {

  printf("init debugging %s\n", src_file);
  // Create debug info builder
  LLVMDIBuilderRef di_builder = LLVMCreateDIBuilder(module);
  ctx->debug_ctx.di_builder = di_builder;

  // Get source filename (you'll need to pass this in or determine it)
  printf("debug src file: %s", src_file);
  ctx->debug_ctx.source_filename = src_file;
  const char *source_dir = get_dirname(src_file);
  printf("src dir: %s\n", source_dir);

  LLVMMetadataRef di_file =
      LLVMDIBuilderCreateFile(ctx->debug_ctx.di_builder, src_file,
                              strlen(src_file), source_dir, strlen(source_dir));

  ctx->debug_ctx.di_file = di_file;

  // Create compile unit
  LLVMMetadataRef di_compile_unit = LLVMDIBuilderCreateCompileUnit(
      di_builder,
      LLVMDWARFSourceLanguageC, // or define your own language
      di_file, "ylc", 3,        // producer
      config.opt_level != NULL, // isOptimized (set based on your opt level)
      "", 0,                    // flags
      0,                        // runtime version
      "", 0,                    // split name
      LLVMDWARFEmissionFull, 0, 0, 0, 0, 0, "", 0);
  ctx->debug_ctx.di_compile_unit = di_compile_unit;
}

void apply_debug_metadata(LLVMValueRef val, Ast *ast, JITLangCtx *ctx,
                          LLVMBuilderRef builder) {
  // Set debug location if available
  if (ctx->debug_ctx.di_builder == NULL) {
    return;
  }
  if (!val) {
    return;
  }

  if (!ast->loc_info) {
    return;
  }

  // const char *src_file;
  // const char *src_content;
  // const char *src_ptr;
  // int line;
  // int col;
  // int col_end;
  // long long absolute_offset;
  LLVMMetadataRef di_loc = LLVMDIBuilderCreateDebugLocation(
      LLVMGetGlobalContext(), ast->loc_info->line, ast->loc_info->col,
      ctx->debug_ctx.di_compile_unit, NULL);

  LLVMSetCurrentDebugLocation2(builder, di_loc);
  printf("\nset debug %s\n\n", ctx->debug_ctx.source_filename);
  LLVMDumpValue(val);
}
