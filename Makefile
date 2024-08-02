BUILD_DIR := build
ENGINE_SRC_DIR := engine
ENGINE_SRCS := $(wildcard $(ENGINE_SRC_DIR)/*.c)
ENGINE_OBJS := $(ENGINE_SRCS:$(ENGINE_SRC_DIR)/%.c=$(BUILD_DIR)/_engine_%.o)

LLVM := /opt/homebrew/opt/llvm@16
LLVM_CONFIG := $(LLVM)/bin/llvm-config


TEST_DIR := lang_test
ENGINE_LDFLAGS = -lsoundio -lm -lsndfile -lraylib -lfftw3
ENGINE_FRAMEWORKS = -framework opengl -framework CoreMIDI -framework cocoa
# ENGINE_COMPILER_OPTIONS = -Werror -Wall -Wextra -g
ENGINE_COMPILER_OPTIONS =-g

ENGINE_CC = clang

# macOS-specific settings
READLINE_PREFIX := $(shell brew --prefix readline)

LANG_SRC_DIR := lang
LANG_SRCS := $(wildcard $(LANG_SRC_DIR)/*.c)
LANG_CC := clang -I./lang -I./engine
LANG_CC += -I$(READLINE_PREFIX)/include 
LANG_CC += -g


LANG_LD_FLAGS := -L./build -lyalce_synth -lm
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline


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

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean run runi audio_test

all: $(BUILD_DIR)/audio_lang

audio_test: $(ENGINE_OBJS)
	$(ENGINE_CC) $(ENGINE_LDFLAGS) $(ENGINE_FRAMEWORKS) $(ENGINE_OBJS) -o build/engine_test
	./build/engine_test



$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/backend_llvm
	mkdir -p $(BUILD_DIR)/types

# Build engine object files
$(BUILD_DIR)/_engine_%.o: $(ENGINE_SRC_DIR)/%.c | $(BUILD_DIR)
	$(ENGINE_CC) $(ENGINE_COMPILER_OPTIONS) -c -o $@ $<

# Build the shared library
$(SHARED_LIB_TARGET): $(filter-out $(BUILD_DIR)/_engine_main.o, $(ENGINE_OBJS))
	$(ENGINE_CC) -shared -o $@ $^ $(ENGINE_LDFLAGS) $(ENGINE_FRAMEWORKS)

# Build lex and yacc output files
$(YACC_OUTPUT): $(YACC_FILE)
	bison -yd $(YACC_FILE) -o $(LANG_SRC_DIR)/y.tab.c

$(LEX_OUTPUT): $(LEX_FILE)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)


# Build language object files
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILD_DIR)
	$(LANG_CC) -c -o $@ $<

# Build the final executable
$(BUILD_DIR)/audio_lang: $(LANG_OBJS) $(SHARED_LIB_TARGET)
	$(LANG_CC) -o $@ $^ $(LANG_LD_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)

LANG_TEST_LD_FLAGS := -L./build -lm -L$(READLINE_PREFIX)/lib -lreadline
runi: $(BUILD_DIR)/audio_lang
	./$(BUILD_DIR)/audio_lang $(input) -i

# List all test files
TEST_SOURCES := $(wildcard $(TEST_DIR)/test_*.c)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
_TEST_TARGETS := $(patsubst $(TEST_DIR)/test_%.c,%,$(TEST_SOURCES))
TEST_TARGETS := $(filter-out llvm_codegen,$(_TEST_TARGETS))

# Common objects for all tests
COMMON_OBJS := $(filter-out \
							 $(BUILD_DIR)/_lang_main.o \
							 $(BUILD_DIR)/main.o \
							 $(BUILD_DIR)/synth_functions.o \
							 $(BUILD_DIR)/arithmetic.o \
							 $(BUILD_DIR)/eval.o \
							 $(BUILD_DIR)/backend_vm.o \
							 $(BUILD_DIR)/backend.o \
							 $(BUILD_DIR)/eval_function.o \
							 $(BUILD_DIR)/eval_list.o, \
							 $(LANG_OBJS))

build/test_llvm_codegen.o: $(TEST_DIR)/test_llvm_codegen.c lang/backend_llvm/*.c
	$(LANG_CC) -I./lang/backend_llvm -DLLVM_BACKEND `$(LLVM_CONFIG) --cflags` -c -o $@ $<

test_llvm_codegen: build/test_llvm_codegen.o $(COMMON_OBJS) build/backend_llvm/*.o build/types/*.o
	$(LANG_CC) $(LANG_TEST_LD_FLAGS) -o $(BUILD_DIR)/$@ $^ `$(LLVM_CONFIG) --libs --ldflags core analysis executionengine mcjit interpreter native` -mmacosx-version-min=13.6
	-./$(BUILD_DIR)/$@

# Rule for building test objects
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/test_%.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) -c -o $@ $< $(LANG_TEST_LD_FLAGS)

# Generic rule for building and running tests
define make-test-rule
$(1): $(BUILD_DIR)/test_$(1).o $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o $(COMMON_OBJS)
	- $$(LANG_CC) $$(LANG_TEST_LD_FLAGS) -o $$(BUILD_DIR)/$$@ $$^ $$(LANG_TEST_LD_FLAGS)
	- ./$(BUILD_DIR)/$$@ 
endef

# Generate rules for all tests
$(foreach test,$(TEST_TARGETS),$(eval $(call make-test-rule,$(test))))

# Generic rule for any other tests
test_%: $(BUILD_DIR)/test_%.o $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o $(COMMON_OBJS)
	-$(LANG_CC) $(LANG_TEST_LD_FLAGS) -o $(BUILD_DIR)/$@ $^ $(LANG_TEST_LD_FLAGS)
	-./$(BUILD_DIR)/$@



# Phony target to run all tests
.PHONY: test
test: $(TEST_TARGETS) test_llvm_codegen $(BUILD_DIR)

# Phony target to clean test files
.PHONY: clean_tests
clean_tests:
	rm -f $(BUILD_DIR)/test_*
	rm -f $(TEST_OBJS)
