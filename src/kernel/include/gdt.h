#ifndef GDT_H
#define GDT_H

#define GDT_SIZE 4

// FIXME: Aus kernel_memory.h beziehen
#define GDT_BASE 0x500

#define CODESEG 0x0A
#define DATASEG 0x02
#define PRESENT 0x80
#define SEGMENT 0x10

void init_gdt();
void gdt_set_descriptor(int segment, dword size, dword base, byte access);

#endif
