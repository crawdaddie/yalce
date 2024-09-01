BUILD_DIR := build

LLVM := /opt/homebrew/opt/llvm@16
LLVM_CONFIG := $(LLVM)/bin/llvm-config


# macOS-specific settings
READLINE_PREFIX := $(shell brew --prefix readline)

LANG_SRC_DIR := lang
LANG_SRCS := $(filter-out $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/lex.yy.c, $(wildcard $(LANG_SRC_DIR)/*.c))
LANG_CC := clang -I./lang -I./engine
LANG_CC += -I$(READLINE_PREFIX)/include 
LANG_CC += -g


LANG_LD_FLAGS := -L$(BUILD_DIR)/engine -lyalce_synth -lm
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline
LANG_LD_FLAGS += -Wl,-rpath,@executable_path/engine

LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/types/*.c)

ifdef DUMP_AST 
LANG_CC += -DDUMP_AST
endif

LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/backend_llvm/*.c)
LANG_CC += -I./lang/backend_llvm -DLLVM_BACKEND `$(LLVM_CONFIG) --cflags`
LANG_LD_FLAGS += `$(LLVM_CONFIG) --libs --cflags --ldflags core analysis executionengine mcjit interpreter native` -mmacosx-version-min=13.6

LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/y.tab.h

# Ensure y.tab.c and lex.yy.c are built before any object files that depend on them
$(LANG_OBJS): $(YACC_OUTPUT) $(LEX_OUTPUT)

LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Explicitly add y.tab.o and lex.yy.o to LANG_OBJS
LANG_OBJS += $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o

.PHONY: all clean engine test wasm serve_docs

all: $(BUILD_DIR)/audio_lang

engine:
	$(MAKE) -C engine

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/backend_llvm
	mkdir -p $(BUILD_DIR)/types

# Build lex and yacc output files
$(YACC_OUTPUT): $(YACC_FILE)
	bison -yd $(YACC_FILE) -o $(LANG_SRC_DIR)/y.tab.c

$(LEX_OUTPUT): $(LEX_FILE)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)


# Build language object files
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILD_DIR)
	$(LANG_CC) -c -o $@ $<

# Build the final executable
$(BUILD_DIR)/audio_lang: $(LANG_OBJS) | engine
	$(LANG_CC) -o $@ $(LANG_OBJS) $(LANG_LD_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)

test:
	$(MAKE) -C lang_test

test_typecheck:
	$(MAKE) -C lang_test test_typecheck

wasm:
	./build_wasm.sh

serve_docs:
	python -m http.server -d docs
