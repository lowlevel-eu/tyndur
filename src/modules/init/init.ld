LINK_ADDR = 0x40000000;

ENTRY(_start)
SECTIONS
{
    . = LINK_ADDR;

    .text :
{
        *(.text)
}
    .rodata ALIGN(4096) :
{
        *(.data)
}
    .data ALIGN(4096) :
{
        *(.data)
}
    .bss ALIGN(4096) :
{
        *(.bss)
}
}
