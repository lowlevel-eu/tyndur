int puts(const char* s) {
    asm("push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp" : : "r" (s));

    return 1;
}
