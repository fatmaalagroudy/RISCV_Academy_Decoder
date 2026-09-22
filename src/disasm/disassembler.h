#pragma once
#include <ostream>
#include <string>
#include "src/elf/elf_reader.h"

namespace decoder::disasm {

class Disassembler {
public:
    // Loads `elf_path`, decodes every executable section, and writes the
    // listing to `out`. Returns false on load/parse failure.
    bool Run(const std::string& elf_path, std::ostream& out);

    const std::string& error() const { return error_; }

private:
    std::string error_;
};

}  // namespace decoder::disasm
