#ifndef _LANG_MIR_H
#define _LANG_MIR_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "parse.h"
#include "types/type.h"

typedef struct MirProgram MirProgram;
typedef struct MirArena MirArena;

MirArena *mir_arena_create(void);
void mir_arena_destroy(MirArena *arena);

MirProgram *mir_build_program(MirArena *arena, Ast *prog, TypeEnv *type_env);
void mir_run_passes(MirProgram *program);
void mir_dump_program(MirProgram *program, FILE *stream);

int mir(Ast *prog, TypeEnv *type_env);
#ifdef __cplusplus
}
#endif

#endif
