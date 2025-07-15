# tools/Makefile
include ../.env

BUILD_DIR := ../build/tools

# LLVM configuration
LLVM_PATH ?= $(shell check_library "llvm")
LLVM_CONFIG := $(LLVM_PATH)/bin/llvm-config
CC := clang
CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs) -lclang

# Source and target configuration
SRCS := $(wildcard *.c)
EXES := $(SRCS:%.c=$(BUILD_DIR)/%)

# Default target
.PHONY: all clean
all: $(EXES)

# Build rules
$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

# Add .PHONY target for each tool
.PHONY: $(EXES:$(BUILD_DIR)/%=%)

# Define targets for each tool
$(EXES:$(BUILD_DIR)/%=%): %: $(BUILD_DIR)/%
