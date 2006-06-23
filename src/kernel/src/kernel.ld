ENTRY(_start)
SECTIONS
{
	. = 0xc0100000;

	kernel_start = .;

	.text : AT(ADDR(.text) - 0xc0000000)
	{
		*(.text)
	}
	.data ALIGN(4096) : AT(ADDR(.data) - 0xc0000000)
	{
		*(.data)
	}
	.bss ALIGN(4096) : AT(ADDR(.bss) - 0xc0000000)
	{
		*(.bss)
	}

	. = ALIGN(4096);

	kernel_end = .;
	kernel_size = kernel_end - kernel_start;
}