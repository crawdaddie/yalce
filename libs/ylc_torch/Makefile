# Simple Makefile for LibTorch FFI
LIBTORCH := /Users/adam/projects/libtorch
LIBTORCH_INCLUDE = $(LIBTORCH)/torch/include
LIBTORCH_LIB = $(LIBTORCH)/lib

# Compiler settings
CC = clang
CXX = clang++
CFLAGS = -std=c99 -fPIC -Wall -Wextra -O2
CXXFLAGS = -std=c++17 -fPIC -Wall -Wextra -O2 -D_GLIBCXX_USE_CXX11_ABI=0

# Include directories
INCLUDES = -I./libtch \
					 -I/Users/adam/projects/libtorch/include \
					 -I/Users/adam/projects/libtorch/include/torch/csrc/api/include

# Library directories
LIBDIRS = -L$(LIBTORCH_LIB)

# Libraries to link
LIBS = -ltorch -ltorch_cpu -lc10

# Detect OS for library extension and flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIB_EXT = .dylib
    SHARED_FLAG = -dynamiclib
    LDFLAGS = -Wl,-rpath,$(LIBTORCH_LIB)
else
    LIB_EXT = .so
    SHARED_FLAG = -shared
    LDFLAGS = -Wl,-rpath,$(LIBTORCH_LIB)
endif

# Target library
TARGET = libtorch_c_api$(LIB_EXT)

# Source files from ./libtch
SOURCES = $(wildcard ./libtch/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean ylc_bindings

# Default target
all: $(TARGET)

# Build the shared library
$(TARGET): $(OBJECTS)
	$(CXX) $(SHARED_FLAG) -o $@ $^ $(LIBDIRS) $(LIBS) $(LDFLAGS)
	@echo "Built: $@"
	make -C . ylc_bindings

# Compile object files
./libtch/%.o: ./libtch/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned"

ylc_bindings:
	$(MAKE) -C ../../tools
	@echo "let _ = dlopen \"./libtorch_c_api.dylib\";" > Torch.ylc
	../../build/tools/ffi_gen -l type_lookups libtch/torch_api.h >> Torch.ylc
	../../build/tools/ffi_gen -l type_lookups libtch/torch_api_generated.hpp >> Torch.ylc

		
		

		

