#pragma once

#include <cstddef>
#include <cstdint>

namespace decoder::elf {

constexpr uint8_t kElfmago0 = 0x7F;
constexpr uint8_t kElfmago1 = 'E';
constexpr uint8_t kElfmago2 = 'L';
constexpr uint8_t kElfmago3 = 'F';

constexpr uint8_t kEiMag0 = 0;
constexpr uint8_t kEiMag1 = 1;
constexpr uint8_t kEiMag2 = 2;
constexpr uint8_t kEiMag3 = 3;
constexpr uint8_t kEiClass = 4;
constexpr uint8_t kEiData = 5;
constexpr uint8_t kEiVersion = 6;
constexpr uint8_t kEiNident = 16;

constexpr uint8_t kElfClass32 = 1;
constexpr uint8_t kElfClass64 = 2;
constexpr uint8_t kElfData2Lsb = 1;
constexpr uint8_t kElfData2Msb = 2;
constexpr uint8_t kEvCurrent = 1;

constexpr uint16_t kEtNone = 0;
constexpr uint16_t kEtRel = 1;
constexpr uint16_t kEtExec = 2;
constexpr uint16_t kEtDyn = 3;
constexpr uint16_t kEtCore = 4;

constexpr uint16_t kEmRiscv = 243;

constexpr uint32_t kShtNull = 0;
constexpr uint32_t kShtProgbits = 1;
constexpr uint32_t kShtSymtab = 2;
constexpr uint32_t kShtStrtab = 3;
constexpr uint32_t kShtRela = 4;
constexpr uint32_t kShtHash = 5;
constexpr uint32_t kShtDynamic = 6;
constexpr uint32_t kShtNote = 7;
constexpr uint32_t kShtNobits = 8;

constexpr uint32_t kShfWrite = 0x1;
constexpr uint32_t kShfAlloc = 0x2;
constexpr uint32_t kShfExecinstr = 0x4;

constexpr uint32_t kShnUndef = 0;
constexpr uint32_t kShnAbs = 0xfff1;
constexpr uint32_t kShnCommon = 0xfff2;
constexpr uint16_t kShnXindex = 0xffff;

constexpr uint8_t kStbLocal = 0;
constexpr uint8_t kStbGlobal = 1;
constexpr uint8_t kStbWeak = 2;

constexpr uint8_t kSttNotype = 0;
constexpr uint8_t kSttObject = 1;
constexpr uint8_t kSttFunc = 2;
constexpr uint8_t kSttSection = 3;
constexpr uint8_t kSttFile = 4;

constexpr std::size_t kElf32EhdrSize = 52;
constexpr std::size_t kElf64EhdrSize = 64;
constexpr std::size_t kElf32ShdrSize = 40;
constexpr std::size_t kElf64ShdrSize = 64;
constexpr std::size_t kElf32SymSize = 16;
constexpr std::size_t kElf64SymSize = 24;

}  // namespace decoder::elf
