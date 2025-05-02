# Makefile for VST3 C Wrapper

# Define the compiler and flags
CXX = clang++
CC = clang
CFLAGS = -Wall -g
CXXFLAGS = -Wall -g -std=c++14

# VST3 SDK Path - Replace with your VST3 SDK location
VST3_SDK_PATH = /Users/adam/projects/sound/vst3sdk

# Include paths
INCLUDES = -I$(VST3_SDK_PATH) -I.

# Library paths and libraries
VST3_LIBS_PATH = $(VST3_SDK_PATH)/build/lib
VST3_LIBS = -L$(VST3_LIBS_PATH) -lbase -lpluginterfaces -lsdk -lsdk_common -lsdk_hosting -lvstgui -lvstgui_standalone -lvstgui_support -lvstgui_uidescription -lobjc

# Mac OS specific frameworks
MAC_FRAMEWORKS = -framework CoreFoundation -framework CoreServices -framework CoreAudio -framework AudioUnit -framework AudioToolbox -framework CoreMIDI -framework Cocoa

# Source files
WRAPPER_SRC = vst3.cpp
EXAMPLE_SRC = main.c

# Object files
WRAPPER_OBJ = $(WRAPPER_SRC:.cpp=.o)
EXAMPLE_OBJ = $(EXAMPLE_SRC:.c=.o)

# Targets
all: vst3_wrapper.a vst3_example_host

# Build the wrapper library
vst3_wrapper.a: $(WRAPPER_OBJ)
	ar rcs $@ $^

# Build the example host
vst3_example_host: $(EXAMPLE_OBJ) vst3_wrapper.a
	$(CC) $(CFLAGS) -o $@ $^ $(VST3_LIBS) $(MAC_FRAMEWORKS) -lstdc++ -ldl -lm

# Compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean up
clean:
	rm -f *.o *.a vst3_example_host

.PHONY: all clean
