# Makefile for Tidal C Project
CC = clang
CFLAGS = -Wall -Wextra -fPIC -O3

# Paths
JSON_C_DIR = /opt/homebrew/opt/json-c


# Include directories
INCLUDES = -I$(JSON_C_DIR)/include \
					 -I../../lang

# Link flags for dynamic linking
LDFLAGS_DYN = -L$(JSON_C_DIR)/lib \
              -rpath $(JSON_C_DIR)/lib \
              -ljson-c \
              -ldl

LDFLAGS_STATIC_HS = -L$(JSON_C_DIR)/lib \
                    -rpath $(JSON_C_DIR)/lib \
                    -ljson-c \
                    -ldl \
                    -lpthread

# Target names
TARGET_DYN = libtidal.dylib
TARGET_STATIC = libtidal_static.dylib
EXECUTABLE = tidal_test

# Default target - build dynamic library
all: $(TARGET_DYN)

# Build shared library with dynamic linking
$(TARGET_DYN): tidal.c
	@echo "Building dynamic library..."
	@echo "Checking for required libraries..."
	$(CC) $(CFLAGS) $(INCLUDES) -shared -o $@ $< $(LDFLAGS_DYN)
	@echo "Built $@ successfully"

# Build self-contained library that doesn't need DYLD_INSERT_LIBRARIES
self-contained: $(TARGET_DYN)
	@echo "Creating self-contained library..."
	# First, let's see what we currently depend on
	@echo "Current dependencies:"
	@otool -L $(TARGET_DYN)
	@echo "Updated dependencies:"
	@otool -L $(TARGET_DYN)

# Build shared library with static linking (alternative)
static: $(TARGET_STATIC)

$(TARGET_STATIC): tidal.c
	@echo "Building static-linked library..."
	@test -f $(TIDAL_STATIC) || (echo "Error: $(TIDAL_STATIC) not found" && exit 1)
	$(CC) $(CFLAGS) $(INCLUDES) -shared -o $@ $< $(LDFLAGS_STATIC)
	@echo "Built $@ successfully"

# Build executable for testing
test: $(EXECUTABLE)


# Clean build artifacts
clean:
	rm -f $(TARGET_DYN) $(TARGET_STATIC) $(EXECUTABLE)
	@echo "Cleaned build artifacts"

# Check if all dependencies exist
check-deps:
	@echo "Checking dependencies..."
	@echo -n "json-c headers: "; test -d $(JSON_C_DIR)/include && echo "✓ Found" || echo "✗ Missing: $(JSON_C_DIR)/include"
	@echo -n "json-c lib: "; test -f $(JSON_C_DIR)/lib/libjson-c.dylib && echo "✓ Found" || echo "✗ Missing: $(JSON_C_DIR)/lib/libjson-c.dylib"

# Show usage information
help:
	@echo "Available targets:"
	@echo "  all (default) - Build dynamic library (libtidal.dylib)"
	@echo "  static        - Build library with static Haskell linking"
	@echo "  test          - Build test executable"
	@echo "  check-deps    - Check if all dependencies exist"
	@echo "  clean         - Remove build artifacts"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Environment variables needed for runtime:"
	@echo "  DYLD_INSERT_LIBRARIES=\"$(HSrts_LIB):$(HSexceptions_LIB)\""

# Declare phony targets
.PHONY: all static test clean check-deps help fix-rpath self-contained
