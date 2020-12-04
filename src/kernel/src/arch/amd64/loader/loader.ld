ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0x100000 + SIZEOF_HEADERS;
	.text :
	{
		*(.text*)
	}
	.data :
	{
		*(.data*)
		*(.rodata*)
	}
	.bss :
	{
		*(.bss*)
	}

}

