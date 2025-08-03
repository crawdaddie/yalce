include ../../.env
LLVM := ${LLVM_PATH}
LLVM_CONFIG := $(LLVM)/bin/llvm-config

# Simple Makefile for cor.cpp
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -g

# Fix: Use $(shell ...) instead of backticks for command substitution
LLVM_CPPFLAGS := $(shell $(LLVM_CONFIG) --cppflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core support irreader transformutils analysis target mcparser bitwriter)

# Add ARM64 target support for macOS
LLVM_LIBS += $(shell $(LLVM_CONFIG) --libs aarch64asmprinter aarch64codegen aarch64desc aarch64disassembler aarch64info aarch64utils)

# Add system libraries
LLVM_SYSTEM_LIBS := $(shell $(LLVM_CONFIG) --system-libs)

TARGET = cor
SOURCE = cor.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(LLVM_CPPFLAGS) $(LLVM_LDFLAGS) $(SOURCE) -o $(TARGET) $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
