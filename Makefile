BUILD_DIR := build
ENGINE_SRC_DIR := engine
ENGINE_SRCS := $(wildcard $(ENGINE_SRC_DIR)/*.c)
ENGINE_OBJS := $(ENGINE_SRCS:$(ENGINE_SRC_DIR)/%.c=$(BUILD_DIR)/_engine_%.o)


TEST_DIR := lang_test
ENGINE_LDFLAGS = -lsoundio -lm -lsndfile -lraylib -lfftw3
ENGINE_FRAMEWORKS = -framework opengl -framework CoreMIDI -framework cocoa
# ENGINE_COMPILER_OPTIONS = -Werror -Wall -Wextra -g
ENGINE_COMPILER_OPTIONS =-g

ENGINE_CC = clang

LANG_SRC_DIR := lang
LANG_SRCS := $(wildcard $(LANG_SRC_DIR)/*.c)
LANG_CC := clang -I./lang -I./engine -g
LANG_LD_FLAGS := -L./build -lyalce_synth -lm

# type inference
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/type_inference/*.c)
LANG_CC += -I./lang/type_inference

# Check for VM flag
ifdef VM_BACKEND
LANG_CC += -DVM_BACKEND
endif

# Check for LLVM flag
ifdef LLVM_BACKEND
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/backend_llvm/*.c)
LANG_CC += -I./lang/backend_llvm -DLLVM_BACKEND `llvm-config --cflags`
LANG_LD_FLAGS += `llvm-config --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`
endif


LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/y.tab.h


SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean run runi audio_test

all: $(BUILD_DIR)/audio_lang

audio_test: $(ENGINE_OBJS)
	$(ENGINE_CC) $(ENGINE_LDFLAGS) $(ENGINE_FRAMEWORKS) $(ENGINE_OBJS) -o build/engine_test
	./build/engine_test



$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/backend_llvm
	mkdir -p $(BUILD_DIR)/type_inference

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
	$(LANG_CC) $(CFLAGS) -c -o $@ $<

# Build the final executable
$(BUILD_DIR)/audio_lang: $(LANG_OBJS) $(SHARED_LIB_TARGET)
	$(LANG_CC) -o $@ $^ $(LANG_LD_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)

test_parse: $(TEST_DIR)/test_parse.o \
	$(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o \
	$(filter-out \
		$(BUILD_DIR)/_lang_main.o \
		$(BUILD_DIR)/main.o $(BUILD_DIR)/test_eval.o \
		$(BUILD_DIR)/test_typecheck.o, \
	$(LANG_OBJS))

	$(LANG_CC) -o $(BUILD_DIR)/$@ $^ $(LANG_LD_FLAGS)
	./$(BUILD_DIR)/test_parse

$(TEST_DIR)/test_parse.o: $(TEST_DIR)/test_parse.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) $(CFLAGS) -c -o $@ $< $(LANG_LD_FLAGS)

test_eval: $(TEST_DIR)/test_eval.o $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o $(filter-out $(BUILD_DIR)/_lang_main.o $(BUILD_DIR)/test_parse.o $(BUILD_DIR)/test_typecheck.o, $(LANG_OBJS))
	$(LANG_CC) -o $(BUILD_DIR)/$@ $^ $(LANG_LD_FLAGS)
	./$(BUILD_DIR)/test_eval

$(TEST_DIR)/test_eval.o: $(TEST_DIR)/test_eval.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) $(CFLAGS) -c -o $@ $< $(LANG_LD_FLAGS)

test_typecheck: $(TEST_DIR)/test_typecheck.o \
	$(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o \
	$(filter-out \
		$(BUILD_DIR)/_lang_main.o \
		$(BUILD_DIR)/main.o \
		$(BUILD_DIR)/test_eval.o \
		$(BUILD_DIR)/test_parse.o, \
	$(LANG_OBJS))

	$(LANG_CC) -o $(BUILD_DIR)/$@ $^ $(LANG_LD_FLAGS)
	./$(BUILD_DIR)/test_typecheck

$(TEST_DIR)/test_typecheck.o: $(TEST_DIR)/test_typecheck.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) $(CFLAGS) -c -o $@ $< $(LANG_LD_FLAGS)

runi: $(BUILD_DIR)/audio_lang
	./$(BUILD_DIR)/audio_lang $(input) -i
