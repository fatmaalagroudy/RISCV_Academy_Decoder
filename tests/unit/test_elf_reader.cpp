#include <gtest/gtest.h>

#include "src/elf/elf_reader.h"
#include "test_paths.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using decoder::elf::ElfClass;
using decoder::elf::ElfReader;

namespace {

std::string WriteTemp(const std::string& name, const std::vector<uint8_t>& bytes) {
  const std::string path = testing::TempDir() + name;
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return path;
}

std::vector<uint8_t> MinimalIdent(uint8_t ei_class = 2, uint8_t ei_data = 1) {
  std::vector<uint8_t> b(16, 0);
  b[0] = 0x7F;
  b[1] = 'E';
  b[2] = 'L';
  b[3] = 'F';
  b[4] = ei_class;
  b[5] = ei_data;
  b[6] = 1;
  return b;
}

}  // namespace

TEST(ElfReaderTest, ValidRv32ElfLoads) {
  ElfReader r;
  ASSERT_TRUE(r.Load(TestInputPath("hello_rv32.elf"))) << r.error();
  EXPECT_EQ(r.elf_class(), ElfClass::kElf32);
  EXPECT_TRUE(r.is_little_endian());
  EXPECT_EQ(r.entry_point(), 0x10094u);
  auto exec = r.ExecutableSections();
  ASSERT_EQ(exec.size(), 1u);
  EXPECT_EQ(exec[0]->name, ".text");
  EXPECT_TRUE(exec[0]->is_executable);
  EXPECT_EQ(exec[0]->bytes.size(), 8u);
}

TEST(ElfReaderTest, ValidRv64ElfLoads) {
  ElfReader r;
  ASSERT_TRUE(r.Load(TestInputPath("hello_rv64.elf"))) << r.error();
  EXPECT_EQ(r.elf_class(), ElfClass::kElf64);
  auto exec = r.ExecutableSections();
  ASSERT_EQ(exec.size(), 1u);
  EXPECT_EQ(exec[0]->name, ".text");
  EXPECT_EQ(exec[0]->address, 0x10094u);
}

TEST(ElfReaderTest, TruncatedFileRejected) {
  auto ident = MinimalIdent();
  const std::string path = WriteTemp("trunc.elf", ident);
  ElfReader r;
  EXPECT_FALSE(r.Load(path));
  EXPECT_FALSE(r.error().empty());
}

TEST(ElfReaderTest, BadMagicRejected) {
  auto bytes = MinimalIdent();
  bytes[1] = 'X';
  const std::string path = WriteTemp("badmagic.elf", bytes);
  ElfReader r;
  EXPECT_FALSE(r.Load(path));
  EXPECT_NE(r.error().find("magic"), std::string::npos);
}

TEST(ElfReaderTest, WrongMachineRejected) {
  ElfReader good;
  ASSERT_TRUE(good.Load(TestInputPath("hello_rv32.elf")));
  std::ifstream in(TestInputPath("hello_rv32.elf"), std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
  // e_machine is at offset 18 for ELF32 LE
  bytes[18] = 3;  // EM_386
  bytes[19] = 0;
  const std::string path = WriteTemp("x86.elf", bytes);
  ElfReader r;
  EXPECT_FALSE(r.Load(path));
  EXPECT_NE(r.error().find("e_machine"), std::string::npos);
}

TEST(ElfReaderTest, ExecutableSectionsIdentified) {
  ElfReader r;
  ASSERT_TRUE(r.Load(TestInputPath("branches_rv64.elf"))) << r.error();
  bool saw_text = false;
  for (const auto& s : r.sections()) {
    if (s.name == ".text") {
      saw_text = true;
      EXPECT_TRUE(s.is_executable);
    } else if (s.name == ".shstrtab") {
      EXPECT_FALSE(s.is_executable);
    }
  }
  EXPECT_TRUE(saw_text);
}
