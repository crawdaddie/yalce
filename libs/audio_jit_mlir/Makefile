include ../../.env

CXX         = clang++
CC          = clang
LLVM        = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config
MLIR_TBLGEN = $(LLVM)/bin/mlir-tblgen

MLIR_INCLUDE = $(LLVM)/include
GEN_DIR = generated
OPS_TD = DspOps.td
OPS_HDR_INC = $(GEN_DIR)/DspOps.h.inc
OPS_CPP_INC = $(GEN_DIR)/DspOps.cpp.inc

CXXFLAGS  = -std=c++17 -Wall -Wextra -fPIC -g
CXXFLAGS += -I../../lang
CXXFLAGS += -I../../lang/backend_llvm
CXXFLAGS += -I../../engine
CXXFLAGS += -I$(MLIR_INCLUDE)
CXXFLAGS += `$(LLVM_CONFIG) --cflags`
CXXFLAGS += -Wno-unused-parameter -Wno-unused-function

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += -I../../lang/backend_llvm
CFLAGS += -I../../engine
CFLAGS += `$(LLVM_CONFIG) --cflags`
CFLAGS += -Wno-unused-parameter -Wno-unused-function

LDFLAGS   = -shared
LDFLAGS  += `$(LLVM_CONFIG) --libs core linker passes orcjit nativecodegen irreader` -lLLVM-21
LDFLAGS  += -lMLIR -lMLIRTargetLLVMIRExport -lMLIRTranslateLib

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

C_SOURCES   = lib.c runtime.c
CPP_SOURCES = compile_synth_mlir.cpp dsp_build_expr.cpp dsp_dialect.cpp state_plan.cpp

C_OBJECTS   = $(patsubst %.c,  $(BUILD_DIR)/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o, $(CPP_SOURCES))
OBJECTS     = $(C_OBJECTS) $(CPP_OBJECTS)

LOCAL_HEADERS  = $(wildcard *.h) $(wildcard *.hpp) $(OPS_HDR_INC) $(OPS_CPP_INC)
ENGINE_HEADERS = $(wildcard ../../engine/*.h)
ENGINE_LIB     = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GEN_DIR):
	mkdir -p $(GEN_DIR)

$(OPS_HDR_INC): $(OPS_TD) | $(GEN_DIR)
	$(MLIR_TBLGEN) --gen-op-decls -I$(MLIR_INCLUDE) $< -o $@

$(OPS_CPP_INC): $(OPS_TD) | $(GEN_DIR)
	$(MLIR_TBLGEN) --gen-op-defs -I$(MLIR_INCLUDE) $< -o $@


$(BUILD_DIR)/%.o: %.c $(LOCAL_HEADERS) $(ENGINE_HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(LOCAL_HEADERS) $(ENGINE_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(GEN_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
