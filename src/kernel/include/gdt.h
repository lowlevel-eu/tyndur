#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_SIZE 6

#define GDT_CODESEG 0x0A
#define GDT_DATASEG 0x02
#define GDT_TSS 0x09
#define GDT_PRESENT 0x80
#define GDT_SEGMENT 0x10

#define SYS_CODE_SEL 0x08
#define SYS_DATA_SEL 0x10
#define USER_CODE_SEL 0x18
#define USER_DATA_SEL 0x20
#define TSS_SEL 0x28

void init_gdt(void);
void gdt_set_descriptor(int segment, uint32_t size, uint32_t base,
    uint8_t access, int dpl);
void gdt_set_descriptor_byte_granularity(int segment, uint32_t size,
    uint32_t base, uint8_t access, int dpl);

#endif
