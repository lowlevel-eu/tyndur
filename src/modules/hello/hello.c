void _start() {
    char *hw = "Hello world!";

    asm(
        "push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : : "r" (hw));

    while(1);
}
