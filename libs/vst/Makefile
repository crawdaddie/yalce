# Makefile for VST Plugin Loader
# VST SDK path - UPDATE THIS to point to your VST SDK installation
# Compiler and flags
CXX = clang++
CXXFLAGS = -g -std=c++11 -Wall -Wextra -O2
LDFLAGS = -ldl
# Include VST SDK headers
VST_SDK_PATH = /Users/adam/projects/sound/ylc-vst/libs/VST2_SDK
ENGINE_PATH = /Users/adam/projects/sound/yalce/engine
CXXFLAGS += -I$(VST_SDK_PATH) -I$(ENGINE_PATH) -I../../lang/

# VST callback calling convention (needed for some systems)
CXXFLAGS += -DVSTCALLBACK=


# Source files
SOURCES = vst_loader.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)


# Library names
LIB_NAME = libvstloader
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).so
# DYLIB = $(LIB_NAME).dylib
# Target executable
TARGET = $(DYNAMIC_LIB)

# Detect OS for dynamic library extension
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # DYNAMIC_LIB = $(DYLIB)
    DYNAMIC_FLAGS = -dynamiclib -install_name @rpath/$(DYNAMIC_LIB)
else
    DYNAMIC_FLAGS = -shared
endif

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(DYNAMIC_FLAGS) -o $@ $^ $(LDFLAGS)

# Default target
all: $(DYNAMIC_LIB)
# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)


# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (default is already optimized)
release: $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the VST loader (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized version"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  help     - Show this help message"

# Phony targets
.PHONY: all clean debug release install uninstall help

# Dependencies (auto-generated)
vst_loader.o: vst_loader.cpp
