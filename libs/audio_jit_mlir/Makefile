include ../../.env

CXX ?= clang++
LLVM        = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config

CXXFLAGS  = -Wall -Wextra -fPIC -g -std=c++17
CXXFLAGS += -I../../lang
CXXFLAGS += -I../../lang/backend_llvm
CXXFLAGS += -I../../engine
CXXFLAGS += `$(LLVM_CONFIG) --cflags`
CXXFLAGS += -I$(LLVM)/include
CXXFLAGS += -Wno-unused-parameter -Wno-unused-function -Wno-extern-c-compat

LDFLAGS   = -shared
LDFLAGS  += `$(LLVM_CONFIG) --ldflags`
LDFLAGS  += -lMLIR
LDFLAGS  += `$(LLVM_CONFIG) --libs core linker` -lLLVM-21
LDFLAGS  += -lm

LIB_NAME    = libaudio_jit_mlir
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

CPP_SOURCES = audio_jit_mlir.cpp mlir_synth_compiler.cpp
OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
LOCAL_HEADERS = $(wildcard *.h)
ENGINE_HEADERS = $(wildcard ../../engine/*.h)
ENGINE_LIB = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
