#include "src/elf/elf_reader.h"

#include "src/elf/elf_types.h"

#include <fstream>
#include <iterator>
#include <sstream>

namespace decoder::elf {
namespace {

class Cursor {
public:
  Cursor(const uint8_t* data, size_t size, bool little_endian)
      : data_(data), size_(size), little_endian_(little_endian) {}

  size_t tell() const { return off_; }
  size_t size() const { return size_; }
  bool ok() const { return ok_; }
  void Seek(size_t off) {
    if (off > size_) {
      ok_ = false;
      return;
    }
    off_ = off;
  }

  uint8_t U8() {
    if (off_ >= size_) {
      ok_ = false;
      return 0;
    }
    return data_[off_++];
  }

  uint16_t U16() {
    const uint8_t a = U8();
    const uint8_t b = U8();
    if (!ok_) {
      return 0;
    }
    return little_endian_ ? static_cast<uint16_t>(a | (b << 8))
                          : static_cast<uint16_t>((a << 8) | b);
  }

  uint32_t U32() {
    const uint16_t a = U16();
    const uint16_t b = U16();
    if (!ok_) {
      return 0;
    }
    return little_endian_ ? (static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 16))
                          : ((static_cast<uint32_t>(a) << 16) | b);
  }

  uint64_t U64() {
    const uint32_t a = U32();
    const uint32_t b = U32();
    if (!ok_) {
      return 0;
    }
    return little_endian_ ? (static_cast<uint64_t>(a) | (static_cast<uint64_t>(b) << 32))
                          : ((static_cast<uint64_t>(a) << 32) | b);
  }

  bool ReadBytes(size_t offset, size_t length, std::vector<uint8_t>& out) {
    if (length == 0) {
      out.clear();
      return true;
    }
    if (offset > size_ || length > size_ - offset) {
      return false;
    }
    out.assign(data_ + offset, data_ + offset + length);
    return true;
  }

  std::string ReadCString(size_t table_off, size_t table_size, uint32_t name_off) {
    if (static_cast<uint64_t>(name_off) >= table_size) {
      return {};
    }
    const size_t start = table_off + name_off;
    if (start >= size_) {
      return {};
    }
    const size_t limit = table_off + table_size;
    const size_t end = limit < size_ ? limit : size_;
    size_t i = start;
    while (i < end && data_[i] != 0) {
      ++i;
    }
    return std::string(reinterpret_cast<const char*>(data_ + start), i - start);
  }

private:
  const uint8_t* data_;
  size_t size_;
  size_t off_ = 0;
  bool little_endian_;
  bool ok_ = true;
};

struct RawSection {
  uint32_t name_off = 0;
  uint32_t type = 0;
  uint64_t flags = 0;
  uint64_t addr = 0;
  uint64_t offset = 0;
  uint64_t size = 0;
  uint32_t link = 0;
  uint32_t info = 0;
  uint64_t addralign = 0;
  uint64_t entsize = 0;
};

bool ReadSectionHeader(Cursor& c, bool is64, RawSection& sh) {
  if (is64) {
    sh.name_off = c.U32();
    sh.type = c.U32();
    sh.flags = c.U64();
    sh.addr = c.U64();
    sh.offset = c.U64();
    sh.size = c.U64();
    sh.link = c.U32();
    sh.info = c.U32();
    sh.addralign = c.U64();
    sh.entsize = c.U64();
  } else {
    sh.name_off = c.U32();
    sh.type = c.U32();
    sh.flags = c.U32();
    sh.addr = c.U32();
    sh.offset = c.U32();
    sh.size = c.U32();
    sh.link = c.U32();
    sh.info = c.U32();
    sh.addralign = c.U32();
    sh.entsize = c.U32();
  }
  return c.ok();
}

}  // namespace

bool ElfReader::Load(const std::string& path) {
  error_.clear();
  sections_.clear();
  symbols_.clear();
  elf_class_ = ElfClass::kElf32;
  little_endian_ = true;
  entry_point_ = 0;

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error_ = "cannot open file: " + path;
    return false;
  }
  std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (file.size() < kEiNident) {
    error_ = "file too small to be ELF";
    return false;
  }

  if (file[kEiMag0] != kElfmago0 || file[kEiMag1] != kElfmago1 || file[kEiMag2] != kElfmago2 ||
      file[kEiMag3] != kElfmago3) {
    error_ = "bad ELF magic";
    return false;
  }

  const uint8_t ei_class = file[kEiClass];
  if (ei_class != kElfClass32 && ei_class != kElfClass64) {
    error_ = "unsupported EI_CLASS";
    return false;
  }
  const bool is64 = ei_class == kElfClass64;
  elf_class_ = is64 ? ElfClass::kElf64 : ElfClass::kElf32;

  const uint8_t ei_data = file[kEiData];
  if (ei_data != kElfData2Lsb && ei_data != kElfData2Msb) {
    error_ = "unsupported EI_DATA";
    return false;
  }
  little_endian_ = ei_data == kElfData2Lsb;

  Cursor c(file.data(), file.size(), little_endian_);
  c.Seek(kEiNident);
  const uint16_t e_type = c.U16();
  (void)e_type;
  const uint16_t e_machine = c.U16();
  const uint32_t e_version = c.U32();
  (void)e_version;
  entry_point_ = is64 ? c.U64() : c.U32();
  const uint64_t e_phoff = is64 ? c.U64() : c.U32();
  (void)e_phoff;
  const uint64_t e_shoff = is64 ? c.U64() : c.U32();
  const uint32_t e_flags = c.U32();
  (void)e_flags;
  const uint16_t e_ehsize = c.U16();
  const uint16_t e_phentsize = c.U16();
  (void)e_phentsize;
  const uint16_t e_phnum = c.U16();
  (void)e_phnum;
  const uint16_t e_shentsize = c.U16();
  const uint16_t e_shnum = c.U16();
  const uint16_t e_shstrndx = c.U16();

  if (!c.ok()) {
    error_ = "truncated ELF header";
    return false;
  }
  if (e_machine != kEmRiscv) {
    error_ = "unsupported e_machine (expected EM_RISCV)";
    return false;
  }

  const size_t expect_ehsize = is64 ? kElf64EhdrSize : kElf32EhdrSize;
  const size_t expect_shentsize = is64 ? kElf64ShdrSize : kElf32ShdrSize;
  if (e_ehsize != expect_ehsize) {
    error_ = "unexpected e_ehsize";
    return false;
  }
  if (e_shnum == 0) {
    error_ = "ELF has no section headers";
    return false;
  }
  if (e_shentsize != expect_shentsize) {
    error_ = "unexpected e_shentsize";
    return false;
  }
  if (e_shoff == 0) {
    error_ = "e_shoff is zero";
    return false;
  }
  if (e_shoff > file.size()) {
    error_ = "e_shoff past end of file";
    return false;
  }
  const uint64_t sh_bytes = static_cast<uint64_t>(e_shnum) * e_shentsize;
  if (sh_bytes / e_shentsize != e_shnum || e_shoff > file.size() ||
      sh_bytes > file.size() - e_shoff) {
    error_ = "section header table out of bounds";
    return false;
  }
  if (e_shstrndx >= e_shnum) {
    error_ = "e_shstrndx out of range";
    return false;
  }

  std::vector<RawSection> raw(e_shnum);
  for (uint16_t i = 0; i < e_shnum; ++i) {
    c.Seek(static_cast<size_t>(e_shoff + static_cast<uint64_t>(i) * e_shentsize));
    if (!ReadSectionHeader(c, is64, raw[i])) {
      error_ = "truncated section header";
      return false;
    }
  }

  const RawSection& shstr = raw[e_shstrndx];
  if (shstr.type != kShtStrtab) {
    error_ = "e_shstrndx does not point at SHT_STRTAB";
    return false;
  }
  if (shstr.offset > file.size() || shstr.size > file.size() - shstr.offset) {
    error_ = "section string table out of bounds";
    return false;
  }

  sections_.reserve(e_shnum);
  for (const RawSection& rs : raw) {
    Section sec;
    sec.name = c.ReadCString(static_cast<size_t>(shstr.offset), static_cast<size_t>(shstr.size),
                             rs.name_off);
    sec.address = rs.addr;
    sec.offset = rs.offset;
    sec.size = rs.size;
    sec.flags = static_cast<uint32_t>(rs.flags);
    sec.is_executable = (rs.flags & kShfExecinstr) != 0;
    if (rs.type != kShtNobits && rs.size != 0) {
      if (rs.offset > file.size() || rs.size > file.size() - rs.offset) {
        error_ = "section '" + sec.name + "' data out of bounds";
        return false;
      }
      if (!c.ReadBytes(static_cast<size_t>(rs.offset), static_cast<size_t>(rs.size), sec.bytes)) {
        error_ = "failed to read section '" + sec.name + "'";
        return false;
      }
    }
    sections_.push_back(std::move(sec));
  }

  for (size_t i = 0; i < raw.size(); ++i) {
    const RawSection& rs = raw[i];
    if (rs.type != kShtSymtab) {
      continue;
    }
    const size_t entsize = rs.entsize != 0 ? static_cast<size_t>(rs.entsize)
                                           : (is64 ? kElf64SymSize : kElf32SymSize);
    if (entsize == 0 || rs.size % entsize != 0) {
      error_ = "invalid symbol table entry size";
      return false;
    }
    if (rs.link >= sections_.size()) {
      error_ = "symbol table sh_link out of range";
      return false;
    }
    const Section& strtab = sections_[rs.link];
    const size_t count = static_cast<size_t>(rs.size / entsize);
    Cursor sc(file.data(), file.size(), little_endian_);
    for (size_t si = 0; si < count; ++si) {
      sc.Seek(static_cast<size_t>(rs.offset + si * entsize));
      Symbol sym;
      if (is64) {
        const uint32_t st_name = sc.U32();
        const uint8_t st_info = sc.U8();
        (void)st_info;
        (void)sc.U8();  // st_other
        (void)sc.U16(); // st_shndx
        sym.value = sc.U64();
        sym.size = sc.U64();
        if (!sc.ok()) {
          error_ = "truncated symbol table";
          return false;
        }
        sym.name = sc.ReadCString(static_cast<size_t>(strtab.offset), strtab.bytes.size(), st_name);
      } else {
        const uint32_t st_name = sc.U32();
        sym.value = sc.U32();
        sym.size = sc.U32();
        const uint8_t st_info = sc.U8();
        (void)st_info;
        (void)sc.U8();
        (void)sc.U16();
        if (!sc.ok()) {
          error_ = "truncated symbol table";
          return false;
        }
        sym.name = sc.ReadCString(static_cast<size_t>(strtab.offset), strtab.bytes.size(), st_name);
      }
      if (!sym.name.empty()) {
        symbols_.push_back(std::move(sym));
      }
    }
  }

  return true;
}

std::vector<const Section*> ElfReader::ExecutableSections() const {
  std::vector<const Section*> out;
  for (const Section& s : sections_) {
    if (s.is_executable) {
      out.push_back(&s);
    }
  }
  return out;
}

}  // namespace decoder::elf
