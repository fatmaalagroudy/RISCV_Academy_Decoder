#include "src/disasm/disassembler.h"

#include "src/decode/decoder.h"

#include <iomanip>
#include <sstream>

namespace decoder::disasm {
namespace {

std::string FormatLine(const decoder::decode::DecodedInstruction& insn) {
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
      out << FormatLine(insn) << "\n";
      offset += insn.length_bytes;
    }
    if (i + 1 != exec.size()) {
      out << "\n";
    }
  }
  return true;
}

}  // namespace decoder::disasm
