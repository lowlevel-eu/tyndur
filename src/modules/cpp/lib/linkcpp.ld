ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

LINK_ADDR = 0x40000000;
SECTIONS
{
    . = LINK_ADDR;

    .text :
    {
        *(.text)
    }
    .rodata :
    {
        *(.data)
    }
    .data :
    {
        *(.data)


        __libcpp_constructors_start = .; *(.ctors) LONG(0) __libcpp_constructors_end = .;
        __libcpp_destructors_start = .; *(.dtors) LONG(0) __libcpp_destructors_end = .;

    }
    .bss :
    {
        *(.bss)
    }

    . = ALIGN(4096);
}
