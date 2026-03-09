include ../../.env

CXX         = clang++
LLVM        = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config

CXXFLAGS  = -fPIC -g -std=c++17
CXXFLAGS += -I../../lang
CXXFLAGS += -I../../lang/backend_llvm
CXXFLAGS += -I../../engine
CXXFLAGS += `$(LLVM_CONFIG) --cflags`

# MLIR headers live alongside LLVM headers
MLIR_INC  = $(LLVM)/include
CXXFLAGS += -I$(MLIR_INC)

# Suppress warnings from MLIR/LLVM headers and unused Yalce includes
CXXFLAGS += -Wno-unused-parameter -Wno-unused-function

LDFLAGS   = -shared
# Single MLIR dylib (built with MLIR_LINK_MLIR_DYLIB=ON)
LDFLAGS  += -lMLIR
LDFLAGS  += `$(LLVM_CONFIG) --libs core linker` -lLLVM-21

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

SOURCES = audio_jit.cpp \
	dialect.cpp \
	codegen_expr.cpp \
	codegen_osc.cpp \
	codegen_buf.cpp \
	codegen_filters.cpp \
	codegen_tables.cpp \
	codegen_noise.cpp \
	codegen_helpers.cpp
OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
ENGINE_HEADERS = $(wildcard ../../engine/*.h ../../engine/faust/*.h)
ENGINE_LIB = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
