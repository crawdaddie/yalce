#include "orc.h"

#include <llvm-c/Error.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct ylc_orc_session {
  LLVMOrcLLJITRef jit;
  LLVMOrcJITDylibRef jd;
  LLVMTargetMachineRef target_machine;
};

static void ylc_orc_set_error(char *error, size_t error_size,
                              const char *format, ...) {
  if (!error || error_size == 0 || !format) {
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(error, error_size, format, args);
  va_end(args);
}

static bool ylc_orc_consume_error(LLVMErrorRef err, char *error,
                                  size_t error_size, const char *prefix) {
  if (!err) {
    return true;
  }

  char *message = LLVMGetErrorMessage(err);
  ylc_orc_set_error(error, error_size, "%s: %s", prefix,
                    message ? message : "unknown LLVM error");
  LLVMDisposeErrorMessage(message);
  return false;
}

static LLVMTargetMachineRef ylc_orc_create_target_machine(
    const ylc_orc_session_t *session, char *error, size_t error_size) {
  const char *triple = ylc_orc_session_triple(session);
  LLVMTargetRef target = NULL;
  char *message = NULL;

  if (LLVMGetTargetFromTriple(triple, &target, &message)) {
    ylc_orc_set_error(error, error_size, "failed getting target: %s",
                      message ? message : "unknown target error");
    LLVMDisposeMessage(message);
    return NULL;
  }

  return LLVMCreateTargetMachine(target, triple, "generic", "",
                                 LLVMCodeGenLevelDefault, LLVMRelocDefault,
                                 LLVMCodeModelDefault);
}

ylc_orc_session_t *ylc_orc_session_create(char *error, size_t error_size) {
  if (LLVMInitializeNativeTarget() != 0) {
    ylc_orc_set_error(error, error_size,
                      "failed initializing native LLVM target");
    return NULL;
  }
  if (LLVMInitializeNativeAsmPrinter() != 0) {
    ylc_orc_set_error(error, error_size,
                      "failed initializing native LLVM asm printer");
    return NULL;
  }
  if (LLVMInitializeNativeAsmParser() != 0) {
    ylc_orc_set_error(error, error_size,
                      "failed initializing native LLVM asm parser");
    return NULL;
  }

  ylc_orc_session_t *session = (ylc_orc_session_t *)calloc(1, sizeof(*session));
  if (!session) {
    ylc_orc_set_error(error, error_size, "failed allocating ORC session");
    return NULL;
  }

  LLVMErrorRef err = LLVMOrcCreateLLJIT(&session->jit, NULL);
  if (err) {
    ylc_orc_consume_error(err, error, error_size, "failed creating LLJIT");
    free(session);
    return NULL;
  }

  session->jd = LLVMOrcLLJITGetMainJITDylib(session->jit);

  LLVMOrcDefinitionGeneratorRef generator = NULL;
  err = LLVMOrcCreateDynamicLibrarySearchGeneratorForProcess(
      &generator, LLVMOrcLLJITGetGlobalPrefix(session->jit), NULL, NULL);
  if (err) {
    ylc_orc_consume_error(err, error, error_size,
                          "failed creating process symbol generator");
    ylc_orc_session_destroy(session);
    return NULL;
  }
  LLVMOrcJITDylibAddGenerator(session->jd, generator);

  session->target_machine =
      ylc_orc_create_target_machine(session, error, error_size);
  if (!session->target_machine) {
    ylc_orc_session_destroy(session);
    return NULL;
  }

  return session;
}

void ylc_orc_session_destroy(ylc_orc_session_t *session) {
  if (!session) {
    return;
  }

  if (session->target_machine) {
    LLVMDisposeTargetMachine(session->target_machine);
  }
  if (session->jit) {
    LLVMErrorRef err = LLVMOrcDisposeLLJIT(session->jit);
    if (err) {
      LLVMConsumeError(err);
    }
  }
  free(session);
}

const char *ylc_orc_session_triple(const ylc_orc_session_t *session) {
  return session && session->jit ? LLVMOrcLLJITGetTripleString(session->jit)
                                 : "";
}

const char *ylc_orc_session_data_layout(const ylc_orc_session_t *session) {
  return session && session->jit ? LLVMOrcLLJITGetDataLayoutStr(session->jit)
                                 : "";
}

LLVMTargetMachineRef
ylc_orc_session_target_machine(const ylc_orc_session_t *session) {
  return session ? session->target_machine : NULL;
}

static bool ylc_orc_session_define_absolute_symbol(
    ylc_orc_session_t *session, const char *name, void *address,
    LLVMJITSymbolGenericFlags flags, char *error, size_t error_size) {
  if (!session || !session->jit || !session->jd || !name || !address) {
    ylc_orc_set_error(error, error_size,
                      "invalid ORC absolute symbol definition request");
    return false;
  }

  LLVMOrcCSymbolMapPair symbol = {
      .Name = LLVMOrcLLJITMangleAndIntern(session->jit, name),
      .Sym = {.Address = (LLVMOrcExecutorAddress)(uintptr_t)address,
              .Flags = {.GenericFlags = flags,
                        .TargetFlags = 0}},
  };

  LLVMOrcMaterializationUnitRef mu = LLVMOrcAbsoluteSymbols(&symbol, 1);
  LLVMErrorRef err = LLVMOrcJITDylibDefine(session->jd, mu);
  if (err) {
    LLVMOrcDisposeMaterializationUnit(mu);
    return ylc_orc_consume_error(err, error, error_size,
                                 "failed defining host helper symbol");
  }

  return true;
}

bool ylc_orc_session_define_host_symbol(ylc_orc_session_t *session,
                                        const char *name, void *address,
                                        char *error, size_t error_size) {
  return ylc_orc_session_define_absolute_symbol(
      session, name, address,
      LLVMJITSymbolGenericFlagsExported | LLVMJITSymbolGenericFlagsCallable,
      error, error_size);
}

bool ylc_orc_session_define_host_data_symbol(ylc_orc_session_t *session,
                                             const char *name, void *address,
                                             char *error, size_t error_size) {
  return ylc_orc_session_define_absolute_symbol(
      session, name, address, LLVMJITSymbolGenericFlagsExported, error,
      error_size);
}

bool ylc_orc_session_add_module(ylc_orc_session_t *session,
                                LLVMContextRef context, LLVMModuleRef module,
                                char *error, size_t error_size) {
  if (!session || !session->jit || !session->jd || !context || !module) {
    ylc_orc_set_error(error, error_size, "invalid ORC module add request");
    return false;
  }

  LLVMOrcThreadSafeContextRef thread_safe_context =
      LLVMOrcCreateNewThreadSafeContextFromLLVMContext(context);
  LLVMOrcThreadSafeModuleRef thread_safe_module =
      LLVMOrcCreateNewThreadSafeModule(module, thread_safe_context);

  LLVMErrorRef err = LLVMOrcLLJITAddLLVMIRModule(session->jit, session->jd,
                                                 thread_safe_module);
  if (err) {
    LLVMOrcDisposeThreadSafeContext(thread_safe_context);
    return ylc_orc_consume_error(err, error, error_size,
                                 "failed adding LLVM IR module");
  }
  LLVMOrcDisposeThreadSafeContext(thread_safe_context);
  return true;
}

bool ylc_orc_session_lookup(ylc_orc_session_t *session, const char *name,
                            LLVMOrcExecutorAddress *address, char *error,
                            size_t error_size) {
  if (!session || !session->jit || !name || !address) {
    ylc_orc_set_error(error, error_size, "invalid ORC symbol lookup request");
    return false;
  }

  LLVMErrorRef err = LLVMOrcLLJITLookup(session->jit, address, name);
  return ylc_orc_consume_error(err, error, error_size,
                               "failed looking up JIT symbol");
}
