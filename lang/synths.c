#include "synths.h"
#include "ht.h"
#include "types/inference.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef ConsSynth(LLVMValueRef input, Type *input_type,
                       LLVMModuleRef module, LLVMBuilderRef builder);

bool is_synth_type(Type *t) {
  return t->alias && (strcmp(t->alias, "Synth") == 0);
}

Type t_synth = {
    T_CONS,
    {.T_CONS =
         {
             TYPE_NAME_PTR,
             (Type *[]){&t_char},
             1,
         }},
    .alias = "Synth",
};

void initialize_synth_types(JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  ht *stack = (ctx->frame->table);
  add_builtin("Synth", &t_synth);
  static TypeClass tc_synth[] = {{
                                     .name = TYPE_NAME_TYPECLASS_ORD,
                                     .rank = 5.0,
                                 },
                                 {
                                     .name = TYPE_NAME_TYPECLASS_EQ,
                                     .rank = 5.0,
                                 }};

  // typeclasses_extend(&t_synth, tc_synth);
  typeclasses_extend(&t_synth, tc_synth);
  typeclasses_extend(&t_synth, tc_synth + 1);
}
