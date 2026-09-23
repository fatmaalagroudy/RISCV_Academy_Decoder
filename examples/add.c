
int add(int a, int b) {
    return a + b;
}

int main(void) {
    int x = 5;
    int y = 7;
    int sum = add(x, y);
    return sum;
}

/* Freestanding entry point: no crt0, no syscalls, just call main and spin. */
void _start(void) {
    main();
    for (;;) { }
}