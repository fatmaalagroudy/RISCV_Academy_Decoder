#!/usr/bin/env python3
"""Generate tiny RISC-V ELF test binaries (no toolchain required).

Re-run this script after changing instruction sequences:
    python3 tests/lit/Inputs/gen_elfs.py
"""

from __future__ import annotations

import struct
from pathlib import Path

EM_RISCV = 243
ET_EXEC = 2
SHT_NULL = 0
SHT_PROGBITS = 1
SHT_STRTAB = 3
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
TEXT_ADDR = 0x10094


def u32le(word: int) -> bytes:
    return struct.pack("<I", word & 0xFFFFFFFF)


def u16le(word: int) -> bytes:
    return struct.pack("<H", word & 0xFFFF)


def i_type(opcode: int, rd: int, funct3: int, rs1: int, imm: int) -> int:
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode


def s_type(opcode: int, funct3: int, rs1: int, rs2: int, imm: int) -> int:
    imm &= 0xFFF
    imm11_5 = (imm >> 5) & 0x7F
    imm4_0 = imm & 0x1F
    return (imm11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm4_0 << 7) | opcode


def b_type(rs1: int, rs2: int, funct3: int, imm: int) -> int:
    imm &= 0x1FFF
    bit12 = (imm >> 12) & 1
    bit11 = (imm >> 11) & 1
    bits10_5 = (imm >> 5) & 0x3F
    bits4_1 = (imm >> 1) & 0xF
    return (
        (bit12 << 31)
        | (bits10_5 << 25)
        | (rs2 << 20)
        | (rs1 << 15)
        | (funct3 << 12)
        | (bits4_1 << 8)
        | (bit11 << 7)
        | 0x63
    )


def j_type(rd: int, imm: int) -> int:
    imm &= 0x1FFFFF
    bit20 = (imm >> 20) & 1
    bits10_1 = (imm >> 1) & 0x3FF
    bit11 = (imm >> 11) & 1
    bits19_12 = (imm >> 12) & 0xFF
    return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) | (rd << 7) | 0x6F


def build_elf(is64: bool, text: bytes, addr: int = TEXT_ADDR) -> bytes:
    shstr = b"\x00.text\x00.shstrtab\x00"
    ehdr_size = 64 if is64 else 52
    shentsize = 64 if is64 else 40
    text_off = ehdr_size
    shstr_off = text_off + len(text)
    shoff = shstr_off + len(shstr)
    ei_class = 2 if is64 else 1
    e_ident = bytes([0x7F, ord("E"), ord("L"), ord("F"), ei_class, 1, 1, 0]) + bytes(8)

    if is64:
        ehdr = e_ident + struct.pack(
            "<HHIQQQIHHHHHH",
            ET_EXEC,
            EM_RISCV,
            1,
            addr,
            0,
            shoff,
            0,
            ehdr_size,
            0,
            0,
            shentsize,
            3,
            2,
        )
        assert len(ehdr) == 64
        sh0 = bytes(64)
        sh1 = struct.pack(
            "<IIQQQQIIQQ",
            1,
            SHT_PROGBITS,
            SHF_ALLOC | SHF_EXECINSTR,
            addr,
            text_off,
            len(text),
            0,
            0,
            4,
            0,
        )
        sh2 = struct.pack(
            "<IIQQQQIIQQ",
            7,
            SHT_STRTAB,
            0,
            0,
            shstr_off,
            len(shstr),
            0,
            0,
            1,
            0,
        )
    else:
        ehdr = e_ident + struct.pack(
            "<HHIIIIIHHHHHH",
            ET_EXEC,
            EM_RISCV,
            1,
            addr,
            0,
            shoff,
            0,
            ehdr_size,
            0,
            0,
            shentsize,
            3,
            2,
        )
        assert len(ehdr) == 52
        sh0 = bytes(40)
        sh1 = struct.pack(
            "<IIIIIIIIII",
            1,
            SHT_PROGBITS,
            SHF_ALLOC | SHF_EXECINSTR,
            addr,
            text_off,
            len(text),
            0,
            0,
            4,
            0,
        )
        sh2 = struct.pack(
            "<IIIIIIIIII",
            7,
            SHT_STRTAB,
            0,
            0,
            shstr_off,
            len(shstr),
            0,
            0,
            1,
            0,
        )

    return ehdr + text + shstr + sh0 + sh1 + sh2


def main() -> None:
    out_dir = Path(__file__).resolve().parent

    hello32 = u32le(i_type(0x13, 2, 0, 2, -16)) + u32le(s_type(0x23, 2, 2, 1, 12))
    (out_dir / "hello_rv32.elf").write_bytes(build_elf(False, hello32))

    hello64 = (
        u32le(i_type(0x13, 2, 0, 2, -16))
        + u32le(s_type(0x23, 2, 2, 1, 12))
        + u32le(i_type(0x1B, 10, 0, 10, 0))
        + u32le(i_type(0x03, 10, 3, 2, 0))  # ld a0, 0(sp)
    )
    (out_dir / "hello_rv64.elf").write_bytes(build_elf(True, hello64))

    branches = (
        u32le(b_type(10, 11, 0, 8))  # beq a0, a1, 8
        + u32le(b_type(10, 0, 1, 4))  # bne a0, zero, 4
        + u32le(j_type(1, 16))  # jal ra, 16
        + u32le(i_type(0x67, 0, 0, 1, 0))  # jalr zero, 0(ra)
    )
    (out_dir / "branches_rv64.elf").write_bytes(build_elf(True, branches))

    compressed = u16le(0x4505) + u32le(i_type(0x13, 10, 0, 10, 1))  # c.li a0, 1; addi a0, a0, 1
    (out_dir / "compressed_rv32.elf").write_bytes(build_elf(False, compressed))

    print("wrote", out_dir / "hello_rv32.elf")
    print("wrote", out_dir / "hello_rv64.elf")
    print("wrote", out_dir / "branches_rv64.elf")
    print("wrote", out_dir / "compressed_rv32.elf")


if __name__ == "__main__":
    main()
