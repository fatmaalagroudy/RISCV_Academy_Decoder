#include "src/decode/decoder.h"

#include "src/decode/opcodes.h"

#include <sstream>
#include <string>

namespace decoder::decode {
namespace {

const char* kAbiNames[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

const char* Reg(unsigned r) { return kAbiNames[r & 31]; }

const char* CReg(unsigned r) { return kAbiNames[(r & 7) + 8]; }

int64_t SignExtend(uint64_t value, int bits) {
  const uint64_t sign = 1ull << (bits - 1);
  return static_cast<int64_t>((value ^ sign) - sign);
}

uint32_t Rd(uint32_t w) { return (w >> 7) & 0x1f; }
uint32_t Funct3(uint32_t w) { return (w >> 12) & 0x7; }
uint32_t Rs1(uint32_t w) { return (w >> 15) & 0x1f; }
uint32_t Rs2(uint32_t w) { return (w >> 20) & 0x1f; }
uint32_t Funct7(uint32_t w) { return (w >> 25) & 0x7f; }

int32_t ImmI(uint32_t w) { return static_cast<int32_t>(SignExtend(w >> 20, 12)); }

int32_t ImmS(uint32_t w) {
  const uint32_t imm = ((w >> 25) << 5) | ((w >> 7) & 0x1f);
  return static_cast<int32_t>(SignExtend(imm, 12));
}

int32_t ImmB(uint32_t w) {
  const uint32_t imm = ((w >> 31) << 12) | (((w >> 25) & 0x3f) << 5) | (((w >> 8) & 0xf) << 1) |
                       (((w >> 7) & 1) << 11);
  return static_cast<int32_t>(SignExtend(imm, 13));
}

uint32_t ImmU(uint32_t w) { return w >> 12; }

int32_t ImmJ(uint32_t w) {
  const uint32_t imm = ((w >> 31) << 20) | (((w >> 12) & 0xff) << 12) | (((w >> 20) & 1) << 11) |
                       (((w >> 21) & 0x3ff) << 1);
  return static_cast<int32_t>(SignExtend(imm, 21));
}

std::string Dec(int64_t v) { return std::to_string(v); }

std::string HexU(uint32_t v) {
  std::ostringstream oss;
  oss << "0x" << std::hex << v;
  return oss.str();
}

std::string FenceBits(unsigned bits) {
  std::string s;
  if (bits & 0x8) {
    s += 'i';
  }
  if (bits & 0x4) {
    s += 'o';
  }
  if (bits & 0x2) {
    s += 'r';
  }
  if (bits & 0x1) {
    s += 'w';
  }
  if (s.empty()) {
    s = "0";
  }
  return s;
}

DecodedInstruction Make(uint64_t addr, uint32_t raw, uint8_t len, Format fmt,
                        const std::string& mnem, const std::string& ops) {
  DecodedInstruction insn;
  insn.address = addr;
  insn.raw = raw;
  insn.length_bytes = len;
  insn.format = fmt;
  insn.mnemonic = mnem;
  insn.operands = ops;
  insn.valid = true;
  return insn;
}

// Same as Make(), but also records the statically-known absolute target
// address of a jal/branch/compressed jump-branch instruction so the
// disassembler can annotate it with a symbol name.
DecodedInstruction MakeBranch(uint64_t addr, uint32_t raw, uint8_t len, Format fmt,
                               const std::string& mnem, const std::string& ops,
                               uint64_t target) {
  DecodedInstruction insn = Make(addr, raw, len, fmt, mnem, ops);
  insn.has_target = true;
  insn.target_address = target;
  return insn;
}

// Bare lowercase hex, no "0x" prefix and no zero-padding (e.g. "1014c"),
// matching how objdump prints branch/jump targets.
std::string HexAddr(uint64_t v) {
  std::ostringstream oss;
  oss << std::hex << v;
  return oss.str();
}

DecodedInstruction Invalid(uint64_t addr, uint32_t raw, uint8_t len) {
  DecodedInstruction insn;
  insn.address = addr;
  insn.raw = raw;
  insn.length_bytes = len;
  insn.format = Format::kUnknown;
  insn.mnemonic = "unknown";
  insn.valid = false;
  return insn;
}

std::string Rops(unsigned rd, unsigned rs1, unsigned rs2) {
  return std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Reg(rs2);
}

std::string Iops(unsigned rd, unsigned rs1, int32_t imm) {
  return std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Dec(imm);
}

std::string LoadOps(unsigned rd, unsigned rs1, int32_t imm) {
  return std::string(Reg(rd)) + ", " + Dec(imm) + "(" + Reg(rs1) + ")";
}

std::string StoreOps(unsigned rs2, unsigned rs1, int32_t imm) {
  return std::string(Reg(rs2)) + ", " + Dec(imm) + "(" + Reg(rs1) + ")";
}

}  // namespace

DecodedInstruction Decoder::Decode(const uint8_t* data, size_t size, size_t offset,
                                   uint64_t vaddr) const {
  if (data == nullptr || offset >= size || size - offset < 2) {
    return Invalid(vaddr, 0, 2);
  }
  const uint16_t half = static_cast<uint16_t>(data[offset] | (data[offset + 1] << 8));
  if ((half & 0x3) != 0x3) {
    return DecodeCompressed(half, vaddr);
  }
  if (size - offset < 4) {
    return Invalid(vaddr, half, 2);
  }
  const uint32_t word = half | (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
  return Decode32(word, vaddr);
}

DecodedInstruction Decoder::Decode32(uint32_t word, uint64_t vaddr) const {
  const uint32_t opcode = word & 0x7f;
  const uint32_t rd = Rd(word);
  const uint32_t f3 = Funct3(word);
  const uint32_t rs1 = Rs1(word);
  const uint32_t rs2 = Rs2(word);
  const uint32_t f7 = Funct7(word);

  switch (opcode) {
  case kOpLui:
    return Make(vaddr, word, 4, Format::kU, "lui", std::string(Reg(rd)) + ", " + HexU(ImmU(word)));
  case kOpAuipc:
    return Make(vaddr, word, 4, Format::kU, "auipc", std::string(Reg(rd)) + ", " + HexU(ImmU(word)));
  case kOpJal: {
    const uint64_t target = vaddr + static_cast<uint64_t>(static_cast<int64_t>(ImmJ(word)));
    return MakeBranch(vaddr, word, 4, Format::kJ, "jal", std::string(Reg(rd)) + ", " + HexAddr(target),
                       target);
  }
  case kOpJalr:
    if (f3 != 0) {
      return Invalid(vaddr, word, 4);
    }
    return Make(vaddr, word, 4, Format::kI, "jalr", LoadOps(rd, rs1, ImmI(word)));
  case kOpBranch: {
    const char* mnem = nullptr;
    switch (f3) {
    case 0:
      mnem = "beq";
      break;
    case 1:
      mnem = "bne";
      break;
    case 4:
      mnem = "blt";
      break;
    case 5:
      mnem = "bge";
      break;
    case 6:
      mnem = "bltu";
      break;
    case 7:
      mnem = "bgeu";
      break;
    default:
      return Invalid(vaddr, word, 4);
    }
    const uint64_t target = vaddr + static_cast<uint64_t>(static_cast<int64_t>(ImmB(word)));
    return MakeBranch(vaddr, word, 4, Format::kB, mnem,
                       std::string(Reg(rs1)) + ", " + Reg(rs2) + ", " + HexAddr(target), target);
  }
  case kOpLoad: {
    const char* mnem = nullptr;
    switch (f3) {
    case 0:
      mnem = "lb";
      break;
    case 1:
      mnem = "lh";
      break;
    case 2:
      mnem = "lw";
      break;
    case 3:
      if (!is_rv64_) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "ld";
      break;
    case 4:
      mnem = "lbu";
      break;
    case 5:
      mnem = "lhu";
      break;
    case 6:
      if (!is_rv64_) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "lwu";
      break;
    default:
      return Invalid(vaddr, word, 4);
    }
    return Make(vaddr, word, 4, Format::kI, mnem, LoadOps(rd, rs1, ImmI(word)));
  }
  case kOpStore: {
    const char* mnem = nullptr;
    switch (f3) {
    case 0:
      mnem = "sb";
      break;
    case 1:
      mnem = "sh";
      break;
    case 2:
      mnem = "sw";
      break;
    case 3:
      if (!is_rv64_) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "sd";
      break;
    default:
      return Invalid(vaddr, word, 4);
    }
    return Make(vaddr, word, 4, Format::kS, mnem, StoreOps(rs2, rs1, ImmS(word)));
  }
  case kOpOpImm: {
    const int32_t imm = ImmI(word);
    switch (f3) {
    case 0:
      return Make(vaddr, word, 4, Format::kI, "addi", Iops(rd, rs1, imm));
    case 2:
      return Make(vaddr, word, 4, Format::kI, "slti", Iops(rd, rs1, imm));
    case 3:
      return Make(vaddr, word, 4, Format::kI, "sltiu", Iops(rd, rs1, imm));
    case 4:
      return Make(vaddr, word, 4, Format::kI, "xori", Iops(rd, rs1, imm));
    case 6:
      return Make(vaddr, word, 4, Format::kI, "ori", Iops(rd, rs1, imm));
    case 7:
      return Make(vaddr, word, 4, Format::kI, "andi", Iops(rd, rs1, imm));
    case 1: {
      const uint32_t shamt = is_rv64_ ? ((word >> 20) & 0x3f) : ((word >> 20) & 0x1f);
      if (is_rv64_) {
        if (((word >> 26) & 0x3f) != 0) {
          return Invalid(vaddr, word, 4);
        }
      } else if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      return Make(vaddr, word, 4, Format::kI, "slli",
                  std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Dec(shamt));
    }
    case 5: {
      const uint32_t shamt = is_rv64_ ? ((word >> 20) & 0x3f) : ((word >> 20) & 0x1f);
      const char* mnem = nullptr;
      if (is_rv64_) {
        const uint32_t top = (word >> 26) & 0x3f;
        if (top == 0) {
          mnem = "srli";
        } else if (top == 0x10) {
          mnem = "srai";
        } else {
          return Invalid(vaddr, word, 4);
        }
      } else {
        if (f7 == 0) {
          mnem = "srli";
        } else if (f7 == kFunct7SubSra) {
          mnem = "srai";
        } else {
          return Invalid(vaddr, word, 4);
        }
      }
      return Make(vaddr, word, 4, Format::kI, mnem,
                  std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Dec(shamt));
    }
    default:
      return Invalid(vaddr, word, 4);
    }
  }
  case kOpOp: {
    if (f7 == kFunct7MulDiv) {
      const char* mnem = nullptr;
      switch (f3) {
      case 0:
        mnem = "mul";
        break;
      case 1:
        mnem = "mulh";
        break;
      case 2:
        mnem = "mulhsu";
        break;
      case 3:
        mnem = "mulhu";
        break;
      case 4:
        mnem = "div";
        break;
      case 5:
        mnem = "divu";
        break;
      case 6:
        mnem = "rem";
        break;
      case 7:
        mnem = "remu";
        break;
      }
      return Make(vaddr, word, 4, Format::kR, mnem, Rops(rd, rs1, rs2));
    }
    if (f7 != 0 && f7 != kFunct7SubSra) {
      return Invalid(vaddr, word, 4);
    }
    const char* mnem = nullptr;
    switch (f3) {
    case 0:
      mnem = (f7 == kFunct7SubSra) ? "sub" : "add";
      break;
    case 1:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "sll";
      break;
    case 2:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "slt";
      break;
    case 3:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "sltu";
      break;
    case 4:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "xor";
      break;
    case 5:
      mnem = (f7 == kFunct7SubSra) ? "sra" : "srl";
      break;
    case 6:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "or";
      break;
    case 7:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "and";
      break;
    default:
      return Invalid(vaddr, word, 4);
    }
    return Make(vaddr, word, 4, Format::kR, mnem, Rops(rd, rs1, rs2));
  }
  case kOpOpImm32: {
    if (!is_rv64_) {
      return Invalid(vaddr, word, 4);
    }
    switch (f3) {
    case 0:
      return Make(vaddr, word, 4, Format::kI, "addiw", Iops(rd, rs1, ImmI(word)));
    case 1:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      return Make(vaddr, word, 4, Format::kI, "slliw",
                  std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Dec((word >> 20) & 0x1f));
    case 5: {
      const char* mnem = nullptr;
      if (f7 == 0) {
        mnem = "srliw";
      } else if (f7 == kFunct7SubSra) {
        mnem = "sraiw";
      } else {
        return Invalid(vaddr, word, 4);
      }
      return Make(vaddr, word, 4, Format::kI, mnem,
                  std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Dec((word >> 20) & 0x1f));
    }
    default:
      return Invalid(vaddr, word, 4);
    }
  }
  case kOpOp32: {
    if (!is_rv64_) {
      return Invalid(vaddr, word, 4);
    }
    if (f7 == kFunct7MulDiv) {
      const char* mnem = nullptr;
      switch (f3) {
      case 0:
        mnem = "mulw";
        break;
      case 4:
        mnem = "divw";
        break;
      case 5:
        mnem = "divuw";
        break;
      case 6:
        mnem = "remw";
        break;
      case 7:
        mnem = "remuw";
        break;
      default:
        return Invalid(vaddr, word, 4);
      }
      return Make(vaddr, word, 4, Format::kR, mnem, Rops(rd, rs1, rs2));
    }
    if (f7 != 0 && f7 != kFunct7SubSra) {
      return Invalid(vaddr, word, 4);
    }
    const char* mnem = nullptr;
    switch (f3) {
    case 0:
      mnem = (f7 == kFunct7SubSra) ? "subw" : "addw";
      break;
    case 1:
      if (f7 != 0) {
        return Invalid(vaddr, word, 4);
      }
      mnem = "sllw";
      break;
    case 5:
      mnem = (f7 == kFunct7SubSra) ? "sraw" : "srlw";
      break;
    default:
      return Invalid(vaddr, word, 4);
    }
    return Make(vaddr, word, 4, Format::kR, mnem, Rops(rd, rs1, rs2));
  }
  case kOpMiscMem:
    if (f3 == 0) {
      const unsigned pred = (word >> 24) & 0xf;
      const unsigned succ = (word >> 20) & 0xf;
      return Make(vaddr, word, 4, Format::kI, "fence", FenceBits(pred) + ", " + FenceBits(succ));
    }
    if (f3 == 1) {
      return Make(vaddr, word, 4, Format::kI, "fence.i", "");
    }
    return Invalid(vaddr, word, 4);
  case kOpSystem:
    if (f3 == 0 && rs1 == 0 && rd == 0) {
      if (ImmI(word) == 0) {
        return Make(vaddr, word, 4, Format::kI, "ecall", "");
      }
      if (ImmI(word) == 1) {
        return Make(vaddr, word, 4, Format::kI, "ebreak", "");
      }
    }
    return Invalid(vaddr, word, 4);
  default:
    return Invalid(vaddr, word, 4);
  }
}

DecodedInstruction Decoder::DecodeCompressed(uint16_t word, uint64_t vaddr) const {
  if (word == 0) {
    return Invalid(vaddr, word, 2);
  }
  const unsigned op = word & 0x3;
  const unsigned f3 = (word >> 13) & 0x7;
  const unsigned rd = (word >> 7) & 0x1f;
  const unsigned rs2 = (word >> 2) & 0x1f;
  const unsigned rdp = (word >> 2) & 0x7;
  const unsigned rs1p = (word >> 7) & 0x7;

  auto cimm5 = [&]() -> int32_t {
    const uint32_t u = (((word >> 12) & 1) << 5) | ((word >> 2) & 0x1f);
    return static_cast<int32_t>(SignExtend(u, 6));
  };

  if (op == 0) {
    switch (f3) {
    case 0: {  // c.addi4spn
      // nzuimm[5:4]=word[12:11], [9:6]=word[10:7], [2]=word[6], [3]=word[5]
      const unsigned uimm = (((word >> 11) & 0x3) << 4) | (((word >> 7) & 0xf) << 6) |
                            (((word >> 6) & 1) << 2) | (((word >> 5) & 1) << 3);
      if (uimm == 0) {
        return Invalid(vaddr, word, 2);
      }
      return Make(vaddr, word, 2, Format::kCIW, "c.addi4spn",
                  std::string(CReg(rdp)) + ", sp, " + Dec(uimm));
    }
    case 2: {  // c.lw
      const unsigned uimm = (((word >> 10) & 0x7) << 3) | (((word >> 6) & 1) << 2) |
                            (((word >> 5) & 1) << 6);
      return Make(vaddr, word, 2, Format::kCL, "c.lw",
                  std::string(CReg(rdp)) + ", " + Dec(uimm) + "(" + CReg(rs1p) + ")");
    }
    case 3: {  // c.ld (rv64) / c.flw (rv32, skip)
      if (!is_rv64_) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned uimm = (((word >> 10) & 0x7) << 3) | (((word >> 5) & 0x3) << 6);
      return Make(vaddr, word, 2, Format::kCL, "c.ld",
                  std::string(CReg(rdp)) + ", " + Dec(uimm) + "(" + CReg(rs1p) + ")");
    }
    case 6: {  // c.sw
      const unsigned uimm = (((word >> 10) & 0x7) << 3) | (((word >> 6) & 1) << 2) |
                            (((word >> 5) & 1) << 6);
      return Make(vaddr, word, 2, Format::kCS, "c.sw",
                  std::string(CReg(rdp)) + ", " + Dec(uimm) + "(" + CReg(rs1p) + ")");
    }
    case 7: {
      if (!is_rv64_) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned uimm = (((word >> 10) & 0x7) << 3) | (((word >> 5) & 0x3) << 6);
      return Make(vaddr, word, 2, Format::kCS, "c.sd",
                  std::string(CReg(rdp)) + ", " + Dec(uimm) + "(" + CReg(rs1p) + ")");
    }
    default:
      return Invalid(vaddr, word, 2);
    }
  }

  if (op == 1) {
    switch (f3) {
    case 0: {  // c.addi / c.nop
      const int32_t imm = cimm5();
      if (rd == 0 && imm == 0) {
        return Make(vaddr, word, 2, Format::kCI, "c.nop", "");
      }
      if (rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      if (imm == 0) {
        return Invalid(vaddr, word, 2);
      }
      return Make(vaddr, word, 2, Format::kCI, "c.addi", std::string(Reg(rd)) + ", " + Dec(imm));
    }
    case 1:
      if (is_rv64_) {  // c.addiw
        if (rd == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCI, "c.addiw", std::string(Reg(rd)) + ", " + Dec(cimm5()));
      } else {  // c.jal
        const uint32_t off = (((word >> 12) & 1) << 11) | (((word >> 8) & 1) << 10) |
                             (((word >> 9) & 3) << 8) | (((word >> 6) & 1) << 7) |
                             (((word >> 7) & 1) << 6) | (((word >> 2) & 1) << 5) |
                             (((word >> 11) & 1) << 4) | (((word >> 3) & 7) << 1);
        const uint64_t target =
            vaddr + static_cast<uint64_t>(static_cast<int64_t>(SignExtend(off, 12)));
        return MakeBranch(vaddr, word, 2, Format::kCJ, "c.jal", HexAddr(target), target);
      }
    case 2: {  // c.li
      if (rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      return Make(vaddr, word, 2, Format::kCI, "c.li", std::string(Reg(rd)) + ", " + Dec(cimm5()));
    }
    case 3:
      if (rd == 2) {  // c.addi16sp
        const uint32_t u = (((word >> 12) & 1) << 9) | (((word >> 3) & 1) << 4) |
                           (((word >> 2) & 1) << 6) | (((word >> 5) & 3) << 7) |
                           (((word >> 6) & 1) << 5);
        if (u == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCI, "c.addi16sp",
                    std::string("sp, ") + Dec(static_cast<int32_t>(SignExtend(u, 10))));
      }
      if (rd != 0) {  // c.lui
        const uint32_t u = (((word >> 12) & 1) << 17) | (((word >> 2) & 0x1f) << 12);
        if ((u >> 12) == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCI, "c.lui",
                    std::string(Reg(rd)) + ", " + HexU(static_cast<uint32_t>(SignExtend(u, 18)) >> 12));
      }
      return Invalid(vaddr, word, 2);
    case 4: {
      const unsigned f2 = (word >> 10) & 0x3;
      if (f2 == 0 || f2 == 1) {  // c.srli / c.srai
        const unsigned shamt = (((word >> 12) & 1) << 5) | ((word >> 2) & 0x1f);
        if (!is_rv64_ && (shamt & 0x20)) {
          return Invalid(vaddr, word, 2);
        }
        if (shamt == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCB, f2 == 0 ? "c.srli" : "c.srai",
                    std::string(CReg(rs1p)) + ", " + Dec(shamt));
      }
      if (f2 == 2) {  // c.andi
        return Make(vaddr, word, 2, Format::kCB, "c.andi",
                    std::string(CReg(rs1p)) + ", " + Dec(cimm5()));
      }
      // f2 == 3
      const unsigned f12 = (word >> 12) & 1;
      const unsigned f6 = (word >> 5) & 0x3;
      if (f12 == 0) {
        const char* m = nullptr;
        switch (f6) {
        case 0:
          m = "c.sub";
          break;
        case 1:
          m = "c.xor";
          break;
        case 2:
          m = "c.or";
          break;
        case 3:
          m = "c.and";
          break;
        }
        return Make(vaddr, word, 2, Format::kCS, m, std::string(CReg(rs1p)) + ", " + CReg(rdp));
      }
      if (!is_rv64_) {
        return Invalid(vaddr, word, 2);
      }
      if (f6 == 0) {
        return Make(vaddr, word, 2, Format::kCS, "c.subw", std::string(CReg(rs1p)) + ", " + CReg(rdp));
      }
      if (f6 == 1) {
        return Make(vaddr, word, 2, Format::kCS, "c.addw", std::string(CReg(rs1p)) + ", " + CReg(rdp));
      }
      return Invalid(vaddr, word, 2);
    }
    case 5: {  // c.j
      const uint32_t off = (((word >> 12) & 1) << 11) | (((word >> 8) & 1) << 10) |
                           (((word >> 9) & 3) << 8) | (((word >> 6) & 1) << 7) |
                           (((word >> 7) & 1) << 6) | (((word >> 2) & 1) << 5) |
                           (((word >> 11) & 1) << 4) | (((word >> 3) & 7) << 1);
      const uint64_t target = vaddr + static_cast<uint64_t>(static_cast<int64_t>(SignExtend(off, 12)));
      return MakeBranch(vaddr, word, 2, Format::kCJ, "c.j", HexAddr(target), target);
    }
    case 6:
    case 7: {  // c.beqz / c.bnez
      const uint32_t off = (((word >> 12) & 1) << 8) | (((word >> 10) & 3) << 3) |
                           (((word >> 5) & 3) << 6) | (((word >> 3) & 3) << 1) |
                           (((word >> 2) & 1) << 5);
      const uint64_t target = vaddr + static_cast<uint64_t>(static_cast<int64_t>(SignExtend(off, 9)));
      return MakeBranch(vaddr, word, 2, Format::kCB, f3 == 6 ? "c.beqz" : "c.bnez",
                         std::string(CReg(rs1p)) + ", " + HexAddr(target), target);
    }
    default:
      return Invalid(vaddr, word, 2);
    }
  }

  if (op == 2) {
    switch (f3) {
    case 0: {  // c.slli
      if (rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned shamt = (((word >> 12) & 1) << 5) | ((word >> 2) & 0x1f);
      if (shamt == 0 || (!is_rv64_ && (shamt & 0x20))) {
        return Invalid(vaddr, word, 2);
      }
      return Make(vaddr, word, 2, Format::kCI, "c.slli", std::string(Reg(rd)) + ", " + Dec(shamt));
    }
    case 2: {  // c.lwsp
      if (rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned uimm = (((word >> 12) & 1) << 5) | (((word >> 4) & 0x7) << 2) |
                            (((word >> 2) & 0x3) << 6);
      return Make(vaddr, word, 2, Format::kCI, "c.lwsp",
                  std::string(Reg(rd)) + ", " + Dec(uimm) + "(sp)");
    }
    case 3: {  // c.ldsp
      if (!is_rv64_ || rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned uimm = (((word >> 12) & 1) << 5) | (((word >> 5) & 0x3) << 3) |
                            (((word >> 2) & 0x7) << 6);
      return Make(vaddr, word, 2, Format::kCI, "c.ldsp",
                  std::string(Reg(rd)) + ", " + Dec(uimm) + "(sp)");
    }
    case 4: {
      const unsigned f12 = (word >> 12) & 1;
      if (f12 == 0) {
        if (rs2 == 0) {  // c.jr
          if (rd == 0) {
            return Invalid(vaddr, word, 2);
          }
          return Make(vaddr, word, 2, Format::kCR, "c.jr", Reg(rd));
        }
        if (rd == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCR, "c.mv", std::string(Reg(rd)) + ", " + Reg(rs2));
      }
      if (rd == 0 && rs2 == 0) {
        return Make(vaddr, word, 2, Format::kCR, "c.ebreak", "");
      }
      if (rs2 == 0) {
        if (rd == 0) {
          return Invalid(vaddr, word, 2);
        }
        return Make(vaddr, word, 2, Format::kCR, "c.jalr", Reg(rd));
      }
      if (rd == 0) {
        return Invalid(vaddr, word, 2);
      }
      return Make(vaddr, word, 2, Format::kCR, "c.add", std::string(Reg(rd)) + ", " + Reg(rs2));
    }
    case 6: {  // c.swsp
      const unsigned uimm = (((word >> 9) & 0xf) << 2) | (((word >> 7) & 0x3) << 6);
      return Make(vaddr, word, 2, Format::kCSS, "c.swsp",
                  std::string(Reg(rs2)) + ", " + Dec(uimm) + "(sp)");
    }
    case 7: {  // c.sdsp
      if (!is_rv64_) {
        return Invalid(vaddr, word, 2);
      }
      const unsigned uimm = (((word >> 10) & 0x7) << 3) | (((word >> 7) & 0x7) << 6);
      return Make(vaddr, word, 2, Format::kCSS, "c.sdsp",
                  std::string(Reg(rs2)) + ", " + Dec(uimm) + "(sp)");
    }
    default:
      return Invalid(vaddr, word, 2);
    }
  }

  return Invalid(vaddr, word, 2);
}

}  // namespace decoder::decode
