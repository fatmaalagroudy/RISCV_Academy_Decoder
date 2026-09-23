# examples/

`main.c` is a small, real (not hand-assembled) C program used to test the
decoder against genuine compiler output rather than only synthetic
instruction encodings.

It's deliberately freestanding (no libc, no syscalls, no floating point,
no atomics) so every instruction GCC emits stays inside what the decoder
currently implements (RV32I/RV64I + M + C). It still exercises real
function calls, loops, conditional branches, and multiplication.

## Toolchain

```
sudo apt-get install gcc-riscv64-linux-gnu
```

## Build

```
riscv64-linux-gnu-gcc -march=rv64imac -mabi=lp64 -nostdlib -static -no-pie \
    -O1 -e _start -o main_rv64.elf main.c
```

- `-nostdlib -static -no-pie -e _start`: no libc/crt0, so the binary
  contains only code the decoder is expected to support.
- `-march=rv64imac`: base RV64I plus M (multiply/divide) and C
  (compressed) — deliberately excludes F/D (float) and A (atomics),
  which aren't implemented yet.
- Built **without** `-s`/`strip`, so the ELF keeps its symbol table —
  required for the decoder's symbol-annotated branch/jump output
  (`<main+0x8>`, etc.) to have anything to resolve against.

## Verifying against a real disassembler

```
riscv64-linux-gnu-objdump -d --no-show-raw-insn main_rv64.elf
```

Compare this against `./build/decoder main_rv64.elf` — addresses, branch
targets, and symbol annotations should match; mnemonic spelling differs
in a few cosmetic spots (the decoder doesn't yet expand RISC-V's
pseudo-instructions, e.g. it prints `c.jr ra` where objdump prints the
pseudo-op `ret`, and keeps the `c.`-prefixed name for compressed forms
where objdump prints the expanded mnemonic).

`Inputs/real_main_rv64.elf` in the LIT test directory is a copy of the
binary built with the exact command above, used by `tests/lit/real-elf.test`.
