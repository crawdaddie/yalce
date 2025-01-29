include ../.env

BUILD_DIR := ../build/cor
INSTALL_DIR := ../build/cor
SRCS := cor.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

COMPILER_OPTIONS = -g -fPIC
CC = clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libcor.so

.PHONY: all clean example

all: $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build cor object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<



$(SHARED_LIB_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	$(CC) -shared -o $@ $^ $(LDFLAGS)
	install_name_tool -id @rpath/libcor.so $@

clean:
	rm -rf $(BUILD_DIR)


example:
	clang example.c cor.c -I./ -o cor && ./cor
