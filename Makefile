CXX      ?= g++
CXXSTD   := -std=c++17
WARN     := -Wall -Wextra -Werror -Wpedantic
INCLUDE  := -I.
CXXFLAGS ?= $(CXXSTD) $(WARN) $(INCLUDE) -O2
DEBUG_FLAGS := $(CXXSTD) $(WARN) $(INCLUDE) -O0 -g -fsanitize=address,undefined

BUILD_DIR := build
BIN       := $(BUILD_DIR)/decoder

SRCS := src/main.cpp \
        src/elf/elf_reader.cpp \
        src/decode/decoder.cpp \
        src/disasm/disassembler.cpp
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all debug test unit-test lit-test clean format

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

debug:
	$(MAKE) CXXFLAGS="$(DEBUG_FLAGS)" all

test: unit-test lit-test

unit-test:
	$(MAKE) -C tests/unit

lit-test: all
	lit -v tests/lit

format:
	clang-format -i $(shell find src tests -name '*.h' -o -name '*.cpp')

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C tests/unit clean
