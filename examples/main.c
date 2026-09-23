/* Small real-world-ish RISC-V test program.
 * Deliberately avoids libc, floating point, and atomics so every
 * instruction the compiler emits stays within what the decoder
 * currently supports (RV32I/RV64I + M + C).
 *
 * Exercises: function calls (jal/jalr), loops and conditional
 * branches (beq/bne/blt), immediate loads (lui/addi), memory
 * access (lw/sw), and multiplication (M extension).
 */

static int add(int a, int b) {
    return a + b;
}

static int multiply(int a, int b) {
    return a * b;
}

static int fibonacci(int n) {
    if (n < 2) {
        return n;
    }
    int prev = 0, curr = 1;
    for (int i = 2; i <= n; i++) {
        int next = add(prev, curr);
        prev = curr;
        curr = next;
    }
    return curr;
}

int main(void) {
    volatile int result = 0;
    for (int i = 0; i < 10; i++) {
        result = add(result, multiply(i, 2));
    }
    result = add(result, fibonacci(10));
    return result;
}

/* Freestanding entry point: no libc, no crt0, no syscalls -
 * just call main and spin. Keeps the whole binary inside the
 * instruction set the decoder implements. */
void _start(void) {
    main();
    for (;;) { }
}
