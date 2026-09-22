#pragma once
#include <cstddef>
#include <cstdint>
#include "src/decode/instruction.h"

namespace decoder::decode {

class Decoder {
public:
    explicit Decoder(bool is_rv64) : is_rv64_(is_rv64) {}

    // Reads 2 or 4 bytes starting at `data[offset]` (little-endian), decodes
    // one instruction, and returns it. Sets .valid=false for illegal/
    // not-yet-implemented encodings rather than throwing.
    DecodedInstruction Decode(const uint8_t* data, size_t size, size_t offset,
                               uint64_t vaddr) const;

private:
    bool is_rv64_;
    DecodedInstruction DecodeCompressed(uint16_t word, uint64_t vaddr) const;
    DecodedInstruction Decode32(uint32_t word, uint64_t vaddr) const;
};

}  // namespace decoder::decode
