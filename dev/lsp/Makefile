include .env

JSONC := ${JSONC}
CC = clang
CFLAGS = -Wall -Wextra -I./src -I/usr/local/include -I$(JSONC)/include -I../../lang
LDFLAGS = -L/usr/local/lib -L$(JSONC)/lib -ljson-c -lreadline

# YLC object files we need to link against
YLC_BUILD_DIR = ../../build
YLC_OBJECTS = \
	$(YLC_BUILD_DIR)/parse.o \
	$(YLC_BUILD_DIR)/common.o \
	$(YLC_BUILD_DIR)/lex.yy.o \
	$(YLC_BUILD_DIR)/y.tab.o \
	$(YLC_BUILD_DIR)/ht.o \
	$(YLC_BUILD_DIR)/string_proc.o \
	$(YLC_BUILD_DIR)/types/inference.o \
	$(YLC_BUILD_DIR)/types/type.o \
	$(YLC_BUILD_DIR)/types/common.o \
	$(YLC_BUILD_DIR)/types/builtins.o \
	$(YLC_BUILD_DIR)/types/unification.o \
	$(YLC_BUILD_DIR)/types/type_declaration.o \
	$(YLC_BUILD_DIR)/types/infer_application.o \
	$(YLC_BUILD_DIR)/types/closures.o \
	$(YLC_BUILD_DIR)/modules.o \
	$(YLC_BUILD_DIR)/serde.o

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/lsp-server

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS) $(YLC_OBJECTS)
	$(CC) $(OBJECTS) $(YLC_OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
