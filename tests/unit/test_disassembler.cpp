#include <gtest/gtest.h>

#include "src/disasm/disassembler.h"
#include "test_paths.h"

#include <sstream>
#include <string>

using decoder::disasm::Disassembler;

TEST(DisassemblerTest, HelloRv32GoldenFormat) {
  Disassembler d;
  std::ostringstream oss;
  ASSERT_TRUE(d.Run(TestInputPath("hello_rv32.elf"), oss)) << d.error();
  const std::string expected =
      "Disassembly of section .text:\n"
      "\n"
      "00010094:  ff010113  addi  sp, sp, -16\n"
      "00010098:  00112623  sw    ra, 12(sp)\n";
  EXPECT_EQ(oss.str(), expected);
}

TEST(DisassemblerTest, CompressedStaysInSync) {
  Disassembler d;
  std::ostringstream oss;
  ASSERT_TRUE(d.Run(TestInputPath("compressed_rv32.elf"), oss)) << d.error();
  EXPECT_NE(oss.str().find("c.li  a0, 1"), std::string::npos);
  EXPECT_NE(oss.str().find("addi  a0, a0, 1"), std::string::npos);
}
