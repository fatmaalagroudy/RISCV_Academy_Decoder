CMAKE ?= cmake
BUILD_DIR := build
CXX ?= g++

.PHONY: all clean

all:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(CXX)
	$(CMAKE) --build $(BUILD_DIR) --config Release

clean:
	rm -rf $(BUILD_DIR)
