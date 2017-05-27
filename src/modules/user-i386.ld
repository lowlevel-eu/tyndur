LOAD_ADDR = 0x40000000;
LINK_ADDR = 0x40000000;

ENTRY(_start)
SECTIONS
{
	. = LINK_ADDR + SIZEOF_HEADERS;

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
		*(.dynbss)
	}

    .ctors : AT(ADDR(.ctors)) {
        __ctor_start__ = .;
        *(.ctors)
        __ctor_end__ = .;
    }

    .dtors : {
        __dtor_start__ = .;
        *(.dtors)
        __dtor_end__ = .;
    }
}

