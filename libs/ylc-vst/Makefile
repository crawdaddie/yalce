CC = clang
CXX = clang++
AR = ar
CXXFLAGS = -g -Wall -Wextra -std=c++17 -fPIC
INCLUDES = -I./libs/VST2_SDK/pluginterfaces/vst2.x -I./libs/VST3_SDK -I./libs/VST3_SDK/pluginterfaces

# Define source and object files
SRC_DIR = src
OBJ_DIR = build
LIB_DIR = build

# VST Plugin Library sources
VST_CPP_SOURCES = $(SRC_DIR)/VSTPlugin.cpp

# Create corresponding object file names
VST_CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(VST_CPP_SOURCES))

# All objects
VST_OBJECTS = $(VST_CPP_OBJECTS)

# Library name
LIB_NAME = libylcvst.a

# Default target
all: directories $(LIB_DIR)/$(LIB_NAME)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)

# Compile C++ source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create static library
$(LIB_DIR)/$(LIB_NAME): $(VST_OBJECTS)
	$(AR) rcs $@ $^

# Shared library version (optional)
shared: CXXFLAGS += -fPIC
shared: CFLAGS += -fPIC
shared: directories $(VST_OBJECTS)
	$(CXX) -shared -o $(LIB_DIR)/libylcvst.so $(VST_OBJECTS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)

# Install (optional, adjust paths as needed)
install: all
	@mkdir -p $(DESTDIR)/usr/local/lib
	@mkdir -p $(DESTDIR)/usr/local/include/ylcvst
	cp $(LIB_DIR)/$(LIB_NAME) $(DESTDIR)/usr/local/lib/
	cp $(SRC_DIR)/VSTPlugin.h $(DESTDIR)/usr/local/include/ylcvst/
	cp $(SRC_DIR)/vst_node.h $(DESTDIR)/usr/local/include/ylcvst/

# Test building (optional)
test: all
	$(CC) -o $(OBJ_DIR)/test_vst main.c -I$(SRC_DIR) -L$(LIB_DIR) -lylcvst -ldl -lstdc++

.PHONY: all directories clean install shared test
