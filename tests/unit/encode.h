#pragma once

#include <cstdint>
#include <string>
#include <vector>

inline uint32_t EncI(uint32_t opcode, uint32_t rd, uint32_t f3, uint32_t rs1, int32_t imm) {
  return ((static_cast<uint32_t>(imm) & 0xFFFu) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) |
         opcode;
}

inline uint32_t EncR(uint32_t opcode, uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t rs2,
                     uint32_t f7) {
  return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | opcode;
}

inline uint32_t EncS(uint32_t opcode, uint32_t f3, uint32_t rs1, uint32_t rs2, int32_t imm) {
  const uint32_t u = static_cast<uint32_t>(imm) & 0xFFFu;
  return ((u >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((u & 0x1Fu) << 7) | opcode;
}

inline uint32_t EncB(uint32_t rs1, uint32_t rs2, uint32_t f3, int32_t imm) {
  const uint32_t u = static_cast<uint32_t>(imm) & 0x1FFFu;
  return ((u >> 12) << 31) | (((u >> 5) & 0x3Fu) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) |
         (((u >> 1) & 0xFu) << 8) | (((u >> 11) & 1u) << 7) | 0x63u;
}

inline uint32_t EncU(uint32_t opcode, uint32_t rd, uint32_t imm20) {
  return (imm20 << 12) | (rd << 7) | opcode;
}

inline uint32_t EncJ(uint32_t rd, int32_t imm) {
  const uint32_t u = static_cast<uint32_t>(imm) & 0x1FFFFFu;
  return ((u >> 20) << 31) | (((u >> 1) & 0x3FFu) << 21) | (((u >> 11) & 1u) << 20) |
         (((u >> 12) & 0xFFu) << 12) | (rd << 7) | 0x6Fu;
}

inline std::vector<uint8_t> Bytes4(uint32_t w) {
  return {static_cast<uint8_t>(w), static_cast<uint8_t>(w >> 8), static_cast<uint8_t>(w >> 16),
          static_cast<uint8_t>(w >> 24)};
}

inline std::vector<uint8_t> Bytes2(uint16_t w) {
  return {static_cast<uint8_t>(w), static_cast<uint8_t>(w >> 8)};
}

struct DecodeCase {
  uint32_t word;
  const char* mnemonic;
  const char* operands;
  uint8_t length = 4;
};
