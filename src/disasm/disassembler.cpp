#include "src/disasm/disassembler.h"

#include "src/decode/decoder.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

namespace decoder::disasm {
namespace {

// Resolves a branch/jump target address to "<name>" or "<name+0xN>" the way
// objdump does, using the ELF's named symbols. Targets that don't fall
// inside any named symbol are left unannotated, same as objdump.
class SymbolResolver {
 public:
  explicit SymbolResolver(const std::vector<decoder::elf::Symbol>& symbols) {
    for (const auto& sym : symbols) {
      if (!sym.name.empty()) {
        entries_.push_back(&sym);
      }
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const decoder::elf::Symbol* a, const decoder::elf::Symbol* b) {
                return a->value < b->value;
              });
  }

  std::string Annotate(uint64_t addr) const {
    if (entries_.empty()) {
      return "";
    }
    auto it = std::upper_bound(
        entries_.begin(), entries_.end(), addr,
        [](uint64_t value, const decoder::elf::Symbol* sym) { return value < sym->value; });
    if (it == entries_.begin()) {
      return "";
    }
    --it;
    const decoder::elf::Symbol* sym = *it;
    const uint64_t end = sym->size > 0 ? sym->value + sym->size : sym->value;
    if (addr != sym->value && (sym->size == 0 || addr >= end)) {
      return "";
    }
    std::ostringstream oss;
    oss << " <" << sym->name;
    if (addr != sym->value) {
      oss << "+0x" << std::hex << (addr - sym->value);
    }
    oss << ">";
    return oss.str();
  }

 private:
  std::vector<const decoder::elf::Symbol*> entries_;
};

std::string FormatLine(const decoder::decode::DecodedInstruction& insn,
                        const SymbolResolver& resolver) {
  std::ostringstream oss;
  oss << std::hex << std::nouppercase << std::setfill('0') << std::setw(8) << insn.address << ":  ";
  if (insn.length_bytes == 2) {
    oss << std::setw(4) << (insn.raw & 0xffff);
  } else {
    oss << std::setw(8) << insn.raw;
  }
  oss << "  ";
  const std::string& mnem = insn.mnemonic.empty() ? "unknown" : insn.mnemonic;
  oss << mnem;
  const int pad = static_cast<int>(mnem.size()) >= 4 ? 0 : 4 - static_cast<int>(mnem.size());
  oss << std::string(static_cast<size_t>(pad), ' ') << "  ";
  if (insn.valid) {
    oss << insn.operands;
    if (insn.has_target) {
      oss << resolver.Annotate(insn.target_address);
    }
  }
  return oss.str();
}

}  // namespace

bool Disassembler::Run(const std::string& elf_path, std::ostream& out) {
  error_.clear();
  decoder::elf::ElfReader reader;
  if (!reader.Load(elf_path)) {
    error_ = reader.error();
    return false;
  }

  const bool is_rv64 = reader.elf_class() == decoder::elf::ElfClass::kElf64;
  decoder::decode::Decoder dec(is_rv64);

  const SymbolResolver resolver(reader.symbols());

  const auto exec = reader.ExecutableSections();
  for (size_t i = 0; i < exec.size(); ++i) {
    const decoder::elf::Section* sec = exec[i];
    out << "Disassembly of section " << sec->name << ":\n\n";
    size_t offset = 0;
    while (offset + 2 <= sec->bytes.size()) {
      const uint64_t vaddr = sec->address + offset;
      auto insn = dec.Decode(sec->bytes.data(), sec->bytes.size(), offset, vaddr);
      if (insn.length_bytes < 2) {
        insn.length_bytes = 2;
      }
      if (offset + insn.length_bytes > sec->bytes.size()) {
        break;
      }
      out << FormatLine(insn, resolver) << "\n";
      offset += insn.length_bytes;
    }
    if (i + 1 != exec.size()) {
      out << "\n";
    }
  }
  return true;
}

}  // namespace decoder::disasm
