#include "gdt.h"
#include "tss.h"
#include "cdefs.h"

tss_t tss = { 
    .ss0 = SYS_DATA_SEL, 
    .io_bit_map_offset = offsetof(tss_t, io_bit_map),
    .io_bit_map_end = 0xFF 
};
