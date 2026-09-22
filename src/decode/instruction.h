#pragma once
#include <cstdint>
#include <string>

namespace decoder::decode {

enum class Format { kR, kI, kS, kB, kU, kJ, kCR, kCI, kCSS, kCIW, kCL, kCS, kCB, kCJ, kUnknown };

struct DecodedInstruction {
    uint64_t address = 0;
    uint32_t raw = 0;          // raw bits, right-justified (16 or 32 valid bits)
    uint8_t length_bytes = 4;  // 2 for compressed, 4 otherwise
    Format format = Format::kUnknown;
    std::string mnemonic;      // e.g. "addi", "c.li", "beq"
    std::string operands;      // pre-formatted, e.g. "sp, sp, -16"
    bool valid = false;        // false => illegal/unrecognized encoding
};

}  // namespace decoder::decode
