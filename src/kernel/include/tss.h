#ifndef TSS_H
#define TSS_H

#define IO_BITMAP_LENGTH 0x1000

#include "types.h"

typedef struct tss_s
{
	dword backlink;
	dword esp0;
	dword ss0;
	dword esp1;
	dword ss1;
	dword esp2;
	dword ss2;
	dword cr3;
	dword eip;
	dword eflags;
	dword eax;
	dword ecx;
	dword edx;
	dword ebx;
	dword esp;
	dword ebp;
	dword esi;
	dword edi;
	dword es;
	dword cs;
	dword ss;
	dword ds;
	dword fs;
	dword gs;
	dword ldt;
	word trace_trap;
	word io_bit_map_offset;
    byte io_bit_map[IO_BITMAP_LENGTH / 8];
    byte io_bit_map_end;
} __attribute__((packed)) tss_t;

extern tss_t tss;

#endif /* ndef TSS_H */
