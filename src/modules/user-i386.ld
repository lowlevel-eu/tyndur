LOAD_ADDR = 0x40000000;
LINK_ADDR = 0x40000000;

ENTRY(_start)
SECTIONS
{
	. = LINK_ADDR;

	.text : AT(ADDR(.text))
	{
		*(.text)
	}
	.rodata ALIGN(4096) : AT(ADDR(.rodata))
	{
		*(.rodata)
	}
	.data ALIGN(4096) : AT(ADDR(.data))
	{
		*(.data)
	}
	.bss ALIGN(4096) : AT(ADDR(.bss))
	{
		*(.bss)
	}

    __ctor_start__ = .;
    .ctors : AT(ADDR(.ctors)) {
        *(.ctors)
    }
    __ctor_end__ = .;

    __dtor_start__ = .;
    .dtors : {
        *(.dtors)
    }
    __dtor_end__ = .;
}

