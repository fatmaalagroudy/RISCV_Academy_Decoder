#include <gtest/gtest.h>

#include "encode.h"
#include "src/decode/decoder.h"

using decoder::decode::Decoder;

class DecoderCompressedTest : public ::testing::TestWithParam<DecodeCase> {};

TEST_P(DecoderCompressedTest, Decode) {
  Decoder d(/*is_rv64=*/false);
  auto bytes = Bytes2(static_cast<uint16_t>(GetParam().word));
  auto insn = d.Decode(bytes.data(), bytes.size(), 0, 0x1000);
  ASSERT_TRUE(insn.valid) << GetParam().mnemonic << " raw=" << GetParam().word;
  EXPECT_EQ(insn.mnemonic, GetParam().mnemonic);
  EXPECT_EQ(insn.operands, GetParam().operands);
  EXPECT_EQ(insn.length_bytes, 2);
}

INSTANTIATE_TEST_SUITE_P(
    CExt, DecoderCompressedTest,
    ::testing::Values(
        DecodeCase{0x4505, "c.li", "a0, 1", 2},
        DecodeCase{0x0001, "c.nop", "", 2},
        DecodeCase{0x8082, "c.jr", "ra", 2},
        DecodeCase{0x9002, "c.ebreak", "", 2},
        DecodeCase{0xa001, "c.j", "0", 2}));

TEST(DecoderCompressedTest, LengthDetectionDoesNotConsumeFourBytes) {
  Decoder d(false);
  uint8_t bytes[6] = {0x05, 0x45, 0x13, 0x05, 0x05, 0x00};  // c.li a0,1 ; addi a0,a0,0
  auto c = d.Decode(bytes, sizeof(bytes), 0, 0);
  ASSERT_TRUE(c.valid);
  EXPECT_EQ(c.length_bytes, 2);
  auto addi = d.Decode(bytes, sizeof(bytes), 2, 2);
  ASSERT_TRUE(addi.valid);
  EXPECT_EQ(addi.mnemonic, "addi");
}
