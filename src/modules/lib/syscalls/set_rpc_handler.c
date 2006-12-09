void set_rpc_handler(void* rpc_handler)
{
    asm(
        "pushl %0;"
        "mov $50, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : : "r" (rpc_handler));
}
