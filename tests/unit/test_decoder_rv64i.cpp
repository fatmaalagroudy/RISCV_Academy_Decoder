#include <gtest/gtest.h>

#include "encode.h"
#include "src/decode/decoder.h"

using decoder::decode::Decoder;

class DecoderRv64ITest : public ::testing::TestWithParam<DecodeCase> {};

TEST_P(DecoderRv64ITest, Decode) {
  Decoder d(/*is_rv64=*/true);
  auto bytes = Bytes4(GetParam().word);
  auto insn = d.Decode(bytes.data(), bytes.size(), 0, 0x1000);
  ASSERT_TRUE(insn.valid) << GetParam().mnemonic;
  EXPECT_EQ(insn.mnemonic, GetParam().mnemonic);
  EXPECT_EQ(insn.operands, GetParam().operands);
  EXPECT_EQ(insn.length_bytes, 4);
}

INSTANTIATE_TEST_SUITE_P(
    Rv64I, DecoderRv64ITest,
    ::testing::Values(
        DecodeCase{EncI(0x03, 10, 3, 2, 0), "ld", "a0, 0(sp)"},
        DecodeCase{EncI(0x03, 10, 6, 2, 4), "lwu", "a0, 4(sp)"},
        DecodeCase{EncS(0x23, 3, 2, 10, 8), "sd", "a0, 8(sp)"},
        DecodeCase{EncI(0x1B, 10, 0, 10, 0), "addiw", "a0, a0, 0"},
        DecodeCase{EncI(0x1B, 10, 1, 11, 3), "slliw", "a0, a1, 3"},
        DecodeCase{EncI(0x1B, 10, 5, 11, 3), "srliw", "a0, a1, 3"},
        DecodeCase{EncI(0x1B, 10, 5, 11, 3) | (0x20u << 25), "sraiw", "a0, a1, 3"},
        DecodeCase{EncR(0x3B, 10, 0, 11, 12, 0), "addw", "a0, a1, a2"},
        DecodeCase{EncR(0x3B, 10, 0, 11, 12, 0x20), "subw", "a0, a1, a2"},
        DecodeCase{EncR(0x3B, 10, 1, 11, 12, 0), "sllw", "a0, a1, a2"},
        DecodeCase{EncR(0x3B, 10, 5, 11, 12, 0), "srlw", "a0, a1, a2"},
        DecodeCase{EncR(0x3B, 10, 5, 11, 12, 0x20), "sraw", "a0, a1, a2"},
        DecodeCase{EncI(0x13, 10, 1, 11, 40), "slli", "a0, a1, 40"},
        DecodeCase{EncR(0x33, 10, 0, 11, 12, 1), "mul", "a0, a1, a2"}));
