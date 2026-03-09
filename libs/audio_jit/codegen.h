#pragma once

#include "codegen_context.h"
#include "codegen_utils.h"

// Consolidated entrypoint header for split audio_jit codegen units.
// As handlers are moved out of audio_jit.cpp, their declarations should be
// added here so units can share them without circular includes.

