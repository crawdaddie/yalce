BUILDDIR := build
ENGINE_SRC_DIR := engine
ENGINE_SRCS := $(wildcard $(ENGINE_SRC_DIR)/*.c)
ENGINE_OBJS := $(ENGINE_SRCS:$(ENGINE_SRC_DIR)/%.c=$(BUILDDIR)/_engine_%.o)

ENGINE_LDFLAGS = -lsoundio -lm -lsndfile -lraylib -lfftw3
ENGINE_FRAMEWORKS = -framework opengl -framework CoreMIDI -framework cocoa
# ENGINE_COMPILER_OPTIONS = -Werror -Wall -Wextra -g
ENGINE_COMPILER_OPTIONS =-g

ENGINE_CC = clang

LANG_SRC_DIR := lang
LANG_SRCS := $(wildcard $(LANG_SRC_DIR)/*.c)
# LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILDDIR)/%.o)
LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILDDIR)/%.o)

LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/y.tab.h

TESTDIR := test
LANG_CC := clang -I./lang -I./engine -g
LANG_LD_FLAGS := -L./build -lyalce_synth -lm

SHARED_LIB_TARGET := $(BUILDDIR)/libyalce_synth.so

.PHONY: all clean run runi audio_test

all: $(BUILDDIR)/audio_lang

audio_test: $(ENGINE_OBJS)
	$(ENGINE_CC) $(ENGINE_LDFLAGS) $(ENGINE_FRAMEWORKS) $(ENGINE_OBJS) -o build/engine_test
	./build/engine_test



$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build engine object files
$(BUILDDIR)/_engine_%.o: $(ENGINE_SRC_DIR)/%.c | $(BUILDDIR)
	$(ENGINE_CC) $(ENGINE_COMPILER_OPTIONS) -c -o $@ $<

# Build the shared library
$(SHARED_LIB_TARGET): $(filter-out $(BUILDDIR)/_engine_main.o, $(ENGINE_OBJS))
	$(ENGINE_CC) -shared -o $@ $^ $(ENGINE_LDFLAGS) $(ENGINE_FRAMEWORKS)

# Build lex and yacc output files
$(YACC_OUTPUT): $(YACC_FILE)
	bison -yd $(YACC_FILE) -o $(LANG_SRC_DIR)/y.tab.c

$(LEX_OUTPUT): $(LEX_FILE)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)

# Build language object files
$(BUILDDIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILDDIR)
	$(LANG_CC) $(CFLAGS) -c -o $@ $<

# Build the final executable
$(BUILDDIR)/audio_lang: $(LANG_OBJS) $(SHARED_LIB_TARGET)
	$(LANG_CC) -o $@ $^ $(LANG_LD_FLAGS)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)

test_parse: $(TESTDIR)/test_parse.o $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o $(filter-out $(BUILDDIR)/_lang_main.o $(BUILDDIR)/test_eval.o, $(LANG_OBJS))
	$(LANG_CC) -o $(BUILDDIR)/$@ $^ $(LANG_LD_FLAGS)
	./$(BUILDDIR)/test_parse

$(TESTDIR)/test_parse.o: $(TESTDIR)/test_parse.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) $(CFLAGS) -c -o $@ $< $(LANG_LD_FLAGS)

test_eval: $(TESTDIR)/test_eval.o $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o $(filter-out $(BUILDDIR)/_lang_main.o $(BUILDDIR)/test_parse.o, $(LANG_OBJS))
	$(LANG_CC) -o $(BUILDDIR)/$@ $^ $(LANG_LD_FLAGS)
	./$(BUILDDIR)/test_eval

$(TESTDIR)/test_eval.o: $(TESTDIR)/test_eval.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(LANG_CC) $(CFLAGS) -c -o $@ $< $(LANG_LD_FLAGS)

run: $(BUILDDIR)/audio_lang
	./$(BUILDDIR)/audio_lang $(input)

runi: $(BUILDDIR)/audio_lang
	./$(BUILDDIR)/audio_lang $(input) -i
