#ifndef YLC_CLAP_ORC_H
#define YLC_CLAP_ORC_H

#include <llvm-c/Core.h>
#include <llvm-c/Orc.h>
#include <llvm-c/TargetMachine.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ylc_orc_session ylc_orc_session_t;

ylc_orc_session_t *ylc_orc_session_create(char *error, size_t error_size);
void ylc_orc_session_destroy(ylc_orc_session_t *session);

const char *ylc_orc_session_triple(const ylc_orc_session_t *session);
const char *ylc_orc_session_data_layout(const ylc_orc_session_t *session);
LLVMTargetMachineRef
ylc_orc_session_target_machine(const ylc_orc_session_t *session);

bool ylc_orc_session_define_host_symbol(ylc_orc_session_t *session,
                                        const char *name, void *address,
                                        char *error, size_t error_size);
bool ylc_orc_session_define_host_data_symbol(ylc_orc_session_t *session,
                                             const char *name, void *address,
                                             char *error, size_t error_size);
bool ylc_orc_session_add_module(ylc_orc_session_t *session,
                                LLVMContextRef context, LLVMModuleRef module,
                                char *error, size_t error_size);
bool ylc_orc_session_lookup(ylc_orc_session_t *session, const char *name,
                            LLVMOrcExecutorAddress *address, char *error,
                            size_t error_size);

#ifdef __cplusplus
}
#endif

#endif
