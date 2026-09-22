#include <gtest/gtest.h>

#include "encode.h"
#include "src/decode/decoder.h"

using decoder::decode::Decoder;

namespace {

void Check(bool is_rv64, const DecodeCase& c) {
  Decoder d(is_rv64);
  auto bytes = Bytes4(c.word);
  auto insn = d.Decode(bytes.data(), bytes.size(), 0, 0x10094);
  ASSERT_TRUE(insn.valid) << c.mnemonic << " " << c.operands;
  EXPECT_EQ(insn.mnemonic, c.mnemonic);
  EXPECT_EQ(insn.operands, c.operands);
  EXPECT_EQ(insn.length_bytes, c.length);
}

}  // namespace

class DecoderRv32ITest : public ::testing::TestWithParam<DecodeCase> {};

TEST_P(DecoderRv32ITest, Decode) { Check(false, GetParam()); }

INSTANTIATE_TEST_SUITE_P(
    Rv32I, DecoderRv32ITest,
    ::testing::Values(
        DecodeCase{EncU(0x37, 10, 1), "lui", "a0, 0x1"},
        DecodeCase{EncU(0x17, 5, 0), "auipc", "t0, 0x0"},
        DecodeCase{EncJ(1, 8), "jal", "ra, 8"},
        DecodeCase{EncI(0x67, 0, 0, 1, 0), "jalr", "zero, 0(ra)"},
        DecodeCase{EncB(10, 11, 0, 8), "beq", "a0, a1, 8"},
        DecodeCase{EncB(10, 0, 1, 4), "bne", "a0, zero, 4"},
        DecodeCase{EncB(10, 11, 4, 12), "blt", "a0, a1, 12"},
        DecodeCase{EncB(10, 11, 5, 12), "bge", "a0, a1, 12"},
        DecodeCase{EncB(10, 11, 6, 12), "bltu", "a0, a1, 12"},
        DecodeCase{EncB(10, 11, 7, 12), "bgeu", "a0, a1, 12"},
        DecodeCase{EncI(0x03, 10, 0, 2, 0), "lb", "a0, 0(sp)"},
        DecodeCase{EncI(0x03, 10, 1, 2, 2), "lh", "a0, 2(sp)"},
        DecodeCase{EncI(0x03, 10, 2, 2, 4), "lw", "a0, 4(sp)"},
        DecodeCase{EncI(0x03, 10, 4, 2, 0), "lbu", "a0, 0(sp)"},
        DecodeCase{EncI(0x03, 10, 5, 2, 2), "lhu", "a0, 2(sp)"},
        DecodeCase{EncS(0x23, 0, 2, 10, 0), "sb", "a0, 0(sp)"},
        DecodeCase{EncS(0x23, 1, 2, 10, 2), "sh", "a0, 2(sp)"},
        DecodeCase{EncS(0x23, 2, 2, 1, 12), "sw", "ra, 12(sp)"},
        DecodeCase{EncI(0x13, 2, 0, 2, 0), "addi", "sp, sp, 0"},
        DecodeCase{EncI(0x13, 2, 0, 2, -16), "addi", "sp, sp, -16"},
        DecodeCase{EncI(0x13, 10, 2, 11, 1), "slti", "a0, a1, 1"},
        DecodeCase{EncI(0x13, 10, 3, 11, 1), "sltiu", "a0, a1, 1"},
        DecodeCase{EncI(0x13, 10, 4, 11, 1), "xori", "a0, a1, 1"},
        DecodeCase{EncI(0x13, 10, 6, 11, 1), "ori", "a0, a1, 1"},
        DecodeCase{EncI(0x13, 10, 7, 11, 1), "andi", "a0, a1, 1"},
        DecodeCase{EncI(0x13, 10, 1, 11, 3), "slli", "a0, a1, 3"},
        DecodeCase{EncI(0x13, 10, 5, 11, 3), "srli", "a0, a1, 3"},
        DecodeCase{EncI(0x13, 10, 5, 11, 3) | (0x20u << 25), "srai", "a0, a1, 3"},
        DecodeCase{EncR(0x33, 10, 0, 11, 12, 0), "add", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 0, 11, 12, 0x20), "sub", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 1, 11, 12, 0), "sll", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 2, 11, 12, 0), "slt", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 3, 11, 12, 0), "sltu", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 4, 11, 12, 0), "xor", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 5, 11, 12, 0), "srl", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 5, 11, 12, 0x20), "sra", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 6, 11, 12, 0), "or", "a0, a1, a2"},
        DecodeCase{EncR(0x33, 10, 7, 11, 12, 0), "and", "a0, a1, a2"},
        DecodeCase{0x0ff0000f, "fence", "iorw, iorw"},
        DecodeCase{0x00000073, "ecall", ""},
        DecodeCase{0x00100073, "ebreak", ""}));

TEST(DecoderRv32ITest, AddiSpSpZero) {
  Decoder d(/*is_rv64=*/false);
  uint8_t bytes[4] = {0x13, 0x01, 0x01, 0x00};
  auto insn = d.Decode(bytes, sizeof(bytes), 0, 0x10094);
  ASSERT_TRUE(insn.valid);
  EXPECT_EQ(insn.mnemonic, "addi");
  EXPECT_EQ(insn.operands, "sp, sp, 0");
  EXPECT_EQ(insn.length_bytes, 4);
}

TEST(DecoderRv32ITest, LdRejectedOnRv32) {
  Decoder d(false);
  auto bytes = Bytes4(EncI(0x03, 10, 3, 2, 0));
  auto insn = d.Decode(bytes.data(), bytes.size(), 0, 0);
  EXPECT_FALSE(insn.valid);
}
