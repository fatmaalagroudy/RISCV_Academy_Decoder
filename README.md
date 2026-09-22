# RISC-V ELF Instruction Decoder

Command-line tool that reads a RISC-V ELF (RV32 or RV64), walks executable sections, and prints a stable disassembly listing.

## Build

```
make
```

Produces `build/decoder`. Requires a C++17 compiler (`g++` or `clang++`).

Debug (ASan/UBSan):

```
make debug
```

## Usage

```
./build/decoder path/to/binary.elf
```

Output format (two spaces between columns, lowercase hex, no `0x` on the address):

```
Disassembly of section .text:

00010094:  ff010113  addi  sp, sp, -16
00010098:  00112623  sw    ra, 12(sp)
```

## Coverage

- ELF32/ELF64, little- or big-endian, `e_machine = EM_RISCV`
- RV32I and RV64I base opcodes (plus `*W` / `ld` / `sd` / `lwu` on RV64)
- RVC (compressed) encodings needed to walk real GCC/Clang binaries without desync
- M extension (`mul`/`div`/`rem` family)

No RISC-V hardware is required: the decoder runs natively and only interprets instruction bytes.

## Tests

Unit tests (GoogleTest, built with CMake from `tests/unit`):

```
make unit-test
./tests/unit/build/unit_tests
```

LIT regression tests (`pip install lit`; `FileCheck` from LLVM):

```
make lit-test
```

Test ELFs under `tests/lit/Inputs/` are generated once with `python3 tests/lit/Inputs/gen_elfs.py` (see that directory's README). They are committed so CI does not need a RISC-V toolchain.
