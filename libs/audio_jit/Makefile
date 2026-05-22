include ../../.env

CC ?= clang
CXX ?= clang++
LLVM        = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += -I../../lang/backend_llvm
CFLAGS += -I../../engine
CFLAGS += `$(LLVM_CONFIG) --cflags`

# Suppress warnings from unused Yalce include surface
CFLAGS += -Wno-unused-parameter -Wno-unused-function

CXXFLAGS  = $(CFLAGS)
CXXFLAGS += -std=c++17

LDFLAGS   = -shared
LDFLAGS  += `$(LLVM_CONFIG) --libs core linker passes` -lLLVM-21
LDFLAGS  += -lfftw3 -lm

ifeq ($(shell uname -s),Darwin)
LDFLAGS += -framework Accelerate
endif

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

C_SOURCES = audio_jit.c compile_synth.c dsp_fn_application.c dsp_build_expr.c pattern_coroutine.c dsp_array_proc.c dsp_fork.c dsp_runtime.c dsp_spectral.c play_handler.c
CPP_SOURCES =
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS += $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
LOCAL_HEADERS = $(wildcard *.h)
ENGINE_HEADERS = $(wildcard ../../engine/*.h)
ENGINE_LIB = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
