CC = clang
CFLAGS = -Wall -Wextra -fPIC -g -I../../lang/

UNAME_S := $(shell uname -s)

DYNAMIC_FLAGS = -shared
DYNAMIC_LIB_EXT = .so

CFLAGS += $(PLATFORM_CFLAGS)

LIB_NAME = libpatterns
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME)$(DYNAMIC_LIB_EXT)

SOURCES = patterns.c
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
