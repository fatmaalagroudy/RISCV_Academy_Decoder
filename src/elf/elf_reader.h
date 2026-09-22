#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace decoder::elf {

enum class ElfClass { kElf32, kElf64 };

struct Section {
    std::string name;
    uint64_t address = 0;      // virtual address (sh_addr)
    uint64_t offset = 0;       // file offset (sh_offset)
    uint64_t size = 0;         // sh_size
    uint32_t flags = 0;        // sh_flags
    bool is_executable = false;
    std::vector<uint8_t> bytes;
};

struct Symbol {
    std::string name;
    uint64_t value = 0;
    uint64_t size = 0;
};

class ElfReader {
public:
    // Loads and validates the file. Returns false (and sets error()) on
    // malformed/non-ELF/unsupported-machine input. Never throws on bad input.
    bool Load(const std::string& path);

    ElfClass elf_class() const { return elf_class_; }
    bool is_little_endian() const { return little_endian_; }
    uint64_t entry_point() const { return entry_point_; }

    const std::vector<Section>& sections() const { return sections_; }
    const std::vector<Symbol>& symbols() const { return symbols_; }

    // Convenience: all sections with SHF_EXECINSTR set, in file order.
    std::vector<const Section*> ExecutableSections() const;

    const std::string& error() const { return error_; }

private:
    ElfClass elf_class_ = ElfClass::kElf32;
    bool little_endian_ = true;
    uint64_t entry_point_ = 0;
    std::vector<Section> sections_;
    std::vector<Symbol> symbols_;
    std::string error_;
};

}  // namespace decoder::elf
