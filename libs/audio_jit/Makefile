include ../../.env

CC ?= clang
CXX ?= clang++
LLVM        = ${LLVM_PATH}
LLVM_VERSION = 21
LLVM_CONFIG = $(shell \
	for config in \
		"$(LLVM)/bin/llvm-config" \
		/usr/lib/llvm21/bin/llvm-config \
		/usr/lib/llvm-21/bin/llvm-config \
		"$$(command -v llvm-config-21 2>/dev/null)" \
		"$$(brew --prefix llvm@21 2>/dev/null)/bin/llvm-config" \
		"$$(command -v llvm-config 2>/dev/null)"; do \
		if [ -x "$$config" ] && "$$config" --version | grep -q '^$(LLVM_VERSION)[.]'; then \
			echo "$$config"; \
			break; \
		fi; \
	done)
CLANG       = $(dir $(LLVM_CONFIG))clang

CFLAGS  = -Wall -Wextra -fPIC -g

ifeq ($(ASAN),1)
    CFLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

ifeq ($(TSAN),1)
    CFLAGS += -fsanitize=thread -fno-omit-frame-pointer
endif
CFLAGS += -I../../lang
CFLAGS += -I../../lang/backend_llvm
CFLAGS += -I../../engine
CFLAGS += `$(LLVM_CONFIG) --cflags`

# Suppress warnings from unused Yalce include surface
CFLAGS += -Wno-unused-parameter -Wno-unused-function
BITCODE_CFLAGS = $(filter-out -g,$(CFLAGS)) -g0

CXXFLAGS  = $(CFLAGS)
CXXFLAGS += -std=c++17

LDFLAGS   = -shared

ifeq ($(ASAN),1)
LDFLAGS += -fsanitize=address
endif

ifeq ($(TSAN),1)
LDFLAGS += -fsanitize=thread
endif
LDFLAGS  += -L`$(LLVM_CONFIG) --libdir`
LDFLAGS  += `$(LLVM_CONFIG) --ldflags --libs core linker passes`
LDFLAGS  += `$(LLVM_CONFIG) --system-libs`
LDFLAGS  += -lm
LDFLAGS  += -L../../engine/build -Wl,-rpath,'$$ORIGIN/../../engine/build'

ifeq ($(shell uname -s),Darwin)
LDFLAGS += -framework Accelerate
endif

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

C_SOURCES = audio_jit.c
CPP_SOURCES =
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS += $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
KERNEL_BITCODE = $(BUILD_DIR)/osc_kernels.bc
LOCAL_HEADERS = $(wildcard *.h)
ENGINE_HEADERS = $(wildcard ../../engine/*.h)
ENGINE_LIB = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS) $(ENGINE_LIB) $(KERNEL_BITCODE)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) -lyalce_synth

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.bc: %.c $(LOCAL_HEADERS) $(ENGINE_HEADERS) | $(BUILD_DIR)
	$(CLANG) $(BITCODE_CFLAGS) -O3 -emit-llvm -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
