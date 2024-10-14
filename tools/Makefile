# tools/Makefile
ENGINE_DIR := ../engine
CC := clang
CFLAGS := $(shell llvm-config --cflags)
LDFLAGS := $(shell llvm-config --ldflags --system-libs) -lclang

BUILD_DIR := ../build/tools

SRCS := $(wildcard *.c)
EXES := $(SRCS:%.c=$(BUILD_DIR)/%)

.PHONY: all clean engine_bindings

all: $(EXES)

$(BUILD_DIR)/%: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

# Add .PHONY target for each tool
.PHONY: $(EXES:$(BUILD_DIR)/%=%)

# Define targets for each tool
$(EXES:$(BUILD_DIR)/%=%): %: $(BUILD_DIR)/%

engine_bindings: all
	$(BUILD_DIR)/ffi_gen $(ENGINE_DIR)/oscillators.h $(ENGINE_DIR)/lib.h $(ENGINE_DIR)/filters.h > ../examples/Synth.ylc
	cat engine_lib_extra.ylc >> ../examples/Synth.ylc
	$(BUILD_DIR)/ffi_gen $(ENGINE_DIR)/scheduling.h > ../examples/Sched.ylc
