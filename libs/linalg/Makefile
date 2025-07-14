CC = clang
CFLAGS = -O3 -fPIC -std=c99 -Wall -Wextra
LDFLAGS = -shared

# Optional OpenMP support
ifdef USE_OPENMP
    CFLAGS += -fopenmp -D_USE_OPENMP
    LDFLAGS += -fopenmp
endif

# Optional BLAS support
ifdef USE_BLAS
    CFLAGS += -D_USE_BLAS
    LDFLAGS += -lopenblas
endif

TARGET = liblinalg.so
SOURCES = linalg.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(MAKE) -C . ylc_bindings

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

# Usage examples:
# make                     # Basic build
# make USE_OPENMP=1        # Build with OpenMP support
# make USE_BLAS=1          # Build with BLAS support
# make USE_OPENMP=1 USE_BLAS=1  # Build with both
ylc_bindings:
	$(MAKE) -C ../../tools
	@echo "let _ = dlopen \"./liblinalg.so\";" > LinAlg.ylc
	../../build/tools/ffi_gen linalg_export.h >> LinAlg.ylc
	cat _linalg_extra.ylc >> LinAlg.ylc
