# Directories
BUILD_DIR := ../build/test
LANG_SRC_DIR := ../lang
READLINE_PREFIX := $(shell brew --prefix readline)

# Compiler and flags
CC := clang
CFLAGS := -g -I$(LANG_SRC_DIR) -I$(READLINE_PREFIX)/include
LDFLAGS := -L$(READLINE_PREFIX)/lib -lreadline

# Lex and Yacc files
LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c
YACC_HEADER := $(LANG_SRC_DIR)/y.tab.h

# Source files
LANG_SRCS := $(LANG_SRC_DIR)/parse.c \
             $(LANG_SRC_DIR)/ht.c \
             $(LANG_SRC_DIR)/common.c \
             $(LANG_SRC_DIR)/input.c \
             $(LANG_SRC_DIR)/string_proc.c \
             $(LANG_SRC_DIR)/serde.c \
             $(LEX_OUTPUT) \
             $(YACC_OUTPUT)

# Phony targets
.PHONY: all clean test_parse test_typecheck test_scripts

all:
	-$(MAKE) test_parse
	-$(MAKE) test_typecheck
	-$(MAKE) test_scripts

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build yacc output files
$(YACC_OUTPUT) $(YACC_HEADER): $(YACC_FILE)
	bison --locations -yd $(YACC_FILE) -o $(YACC_OUTPUT)

# Build lex output file
$(LEX_OUTPUT): $(LEX_FILE) $(YACC_HEADER)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)

test_parse: $(BUILD_DIR) $(LEX_OUTPUT) $(YACC_OUTPUT)
	$(CC) $(CFLAGS) test_parse.c $(LANG_SRCS) \
		$(LANG_SRC_DIR)/types/common.c \
		-o $(BUILD_DIR)/test_parse $(LDFLAGS)
	$(BUILD_DIR)/test_parse

test_typecheck: $(BUILD_DIR) $(LEX_OUTPUT) $(YACC_OUTPUT)
	$(CC) $(CFLAGS) test_typecheck.c \
		$(LANG_SRCS) \
		$(LANG_SRC_DIR)/modules.c \
		$(LANG_SRC_DIR)/types/*.c \
		-o $(BUILD_DIR)/test_typecheck \
		$(LDFLAGS)
	$(BUILD_DIR)/test_typecheck


test_scripts: 
	./run_test_scripts.sh ./test_scripts


clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT) $(YACC_HEADER)

