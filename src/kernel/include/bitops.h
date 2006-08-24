#ifndef BITOPS_H
#define BITOPS_H

/* Voraussetzung: Mindestens ein Bit von i muss gesetzt sein. */
static inline unsigned int bit_scan_forward(unsigned int i)
{
    unsigned int j;
#if 0
    for(j = 0; j < 32; j++)
    {
        if(i & (1 << j))
        {
            return j;
        }
    }
#else
    __asm__("bsfl %1, %0\n\t" : "=r"(j) : "g"(i));
    return j;
#endif
}

#endif /* ndef BITOPS_H */
