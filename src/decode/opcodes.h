#pragma once

#include <cstdint>

namespace decoder::decode {

constexpr uint32_t kOpLoad = 0x03;
constexpr uint32_t kOpMiscMem = 0x0f;
constexpr uint32_t kOpOpImm = 0x13;
constexpr uint32_t kOpAuipc = 0x17;
constexpr uint32_t kOpOpImm32 = 0x1b;
constexpr uint32_t kOpStore = 0x23;
constexpr uint32_t kOpOp = 0x33;
constexpr uint32_t kOpLui = 0x37;
constexpr uint32_t kOpOp32 = 0x3b;
constexpr uint32_t kOpBranch = 0x63;
constexpr uint32_t kOpJalr = 0x67;
constexpr uint32_t kOpJal = 0x6f;
constexpr uint32_t kOpSystem = 0x73;

constexpr uint32_t kFunct7SubSra = 0x20;
constexpr uint32_t kFunct7MulDiv = 0x01;

}  // namespace decoder::decode
