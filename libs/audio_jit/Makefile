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

SOURCES = audio_jit.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DYNAMIC_LIB)

.PHONY: all clean
