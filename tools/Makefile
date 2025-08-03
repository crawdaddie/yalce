# Simple Makefile for YLC Header Parser
# Reads LLVM_PATH from ../.env file

# Include environment variables from ../.env
include ../.env

# LLVM configuration - use LLVM_PATH from .env file, with fallback
LLVM_PATH := ${LLVM_PATH}
LLVM_CONFIG := $(LLVM_PATH)/bin/llvm-config

# Check if llvm-config exists
ifeq ($(shell test -x "$(LLVM_CONFIG)" && echo yes),yes)
    # Use llvm-config for flags
    CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
    LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --system-libs) -lclang
else
    # Fallback for manual LLVM installation
    CFLAGS := -I$(LLVM_PATH)/include
    LDFLAGS := -L$(LLVM_PATH)/lib -lclang
endif

# Add common flags
CFLAGS += -std=c99 -Wall -Wextra
CC := clang

# Source and target
SRC := ffi_gen.c
TARGET := ../build/tools/ffi_gen

# Build rule
$(TARGET): $(SRC)
	mkdir -p ../build/tools
	@echo "Building $(TARGET) with LLVM at $(LLVM_PATH)..."
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"

# Test with a simple header
test: $(TARGET)
	@echo "Testing with a simple header..."
	@echo '#include <stdint.h>' > test.h
	@echo 'void test_func(uint64_t arg);' >> test.h
	@echo 'typedef void (*TestCallback)(int x);' >> test.h
	./$(TARGET) test.h
	@rm -f test.h

# Clean
clean:
	rm -f $(TARGET) test.h

# Debug - show configuration
debug:
	@echo "LLVM_PATH: $(LLVM_PATH)"
	@echo "LLVM_CONFIG: $(LLVM_CONFIG)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "CC: $(CC)"

# Install to a directory (optional)
install: $(TARGET)
	@if [ -z "$(PREFIX)" ]; then \
		echo "Usage: make install PREFIX=/usr/local"; \
		exit 1; \
	fi
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/

# Help
help:
	@echo "YLC Header Parser Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all/$(TARGET)  Build the header parser"
	@echo "  test          Build and test with a simple header"
	@echo "  clean         Remove built files"
	@echo "  debug         Show build configuration"
	@echo "  install       Install to PREFIX (e.g., make install PREFIX=/usr/local)"
	@echo "  help          Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  LLVM_PATH     Path to LLVM installation (from ../.env or auto-detected)"
	@echo "  CC            C compiler (default: clang)"
	@echo ""
	@echo "Examples:"
	@echo "  make"
	@echo "  make test"
	@echo "  echo 'LLVM_PATH=/usr/local/llvm-20' > ../.env && make"

.PHONY: test clean debug install help

# Default target
all: $(TARGET)
