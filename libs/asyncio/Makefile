# Makefile for AsyncIO Library
# Platform-specific async I/O: kqueue (macOS/BSD) vs epoll (Linux)

CC = clang
CFLAGS = -Wall -Wextra -fPIC -g -I../../lang/

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS uses kqueue for async I/O
    PLATFORM_CFLAGS = -DUSE_KQUEUE
    DYNAMIC_FLAGS = -shared
else ifeq ($(UNAME_S),Linux)
    # Linux uses epoll for async I/O
    PLATFORM_CFLAGS = -DUSE_EPOLL
    DYNAMIC_FLAGS = -shared
    LDFLAGS += -lpthread
else
    $(error Unsupported platform: $(UNAME_S))
endif

DYNAMIC_LIB_EXT = .so

CFLAGS += $(PLATFORM_CFLAGS)

LIB_NAME = libasyncio
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME)$(DYNAMIC_LIB_EXT)

SOURCES = asyncio.c
OBJECTS = $(SOURCES:.c=.o)

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) $(DYNAMIC_FLAGS) -o $@ $^ $(LDFLAGS)

$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(STATIC_LIB) $(DYNAMIC_LIB)

# Static library build
static: $(STATIC_LIB)

# Show platform info
info:
	@echo "Platform: $(UNAME_S)"
	@echo "Platform flags: $(PLATFORM_CFLAGS)"
	@echo "Dynamic library: $(DYNAMIC_LIB)"
	@echo "Static library: $(STATIC_LIB)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build dynamic library (default)"
	@echo "  static   - Build static library"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized version"
	@echo "  info     - Show platform information"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Platform-specific defines:"
	@echo "  macOS:   USE_KQUEUE (uses kqueue for async I/O)"
	@echo "  Linux:   USE_EPOLL (uses epoll for async I/O)"

# Phony targets
.PHONY: all static clean debug release info help
