include .env

JSONC := ${JSONC}
CC = clang
CFLAGS = -Wall -Wextra -I./src -I/usr/local/include -I$(JSONC)/include
LDFLAGS = -L/usr/local/lib -L$(JSONC)/lib -ljson-c

SRC_DIR = src
BUILD_DIR = ../../build/dev/lsp
INCLUDE_DIR = include

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/ylc-lsp-server

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
