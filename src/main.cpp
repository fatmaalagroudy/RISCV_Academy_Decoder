#include "src/disasm/disassembler.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: decoder <elf-file>\n";
    return 2;
  }
  decoder::disasm::Disassembler disasm;
  if (!disasm.Run(argv[1], std::cout)) {
    std::cerr << "decoder: " << disasm.error() << "\n";
    return 1;
  }
  return 0;
}
