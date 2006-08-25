#include "ports.h"
static short * video = (short*)0xb8000;
char color = 0xC0;

void clrscr()
{
	unsigned char *vidmem = (unsigned char *)0xB8000;
	const long size = 80*25;
	long loop;

	for (loop=0; loop<size; loop++)
	{
		*vidmem++ = 0;
		*vidmem++ = color;
	}
}

void kcolor(char newcolor)
{
	color = newcolor;
}

/* Gibt ein Zeichen auf dem Bildschirm aus. */
static void kputc(char c)
{
    int i;

    if(c == '\r')
    {
        video -= (((int)video - 0xb8000) % 160) / 2;
    }
    else if(c == '\n')
    {
        for(i = (((int)video - 0xb8000) % 160) / 2; i < 80; i++)
        {
            kputc(' ');
        }
    }
    else if(c == '\t')
    {
        for(i = (((int)video - 0xb8000) % 16) / 2; i < 8; i++)
        {
            kputc(' ');
        }
    }
    else
    {
        *video++ = c | (color << 8);
    }

    if((int)video - 0xb8000 >= 160 * 24)
    {
        short * src = (short*)0xb8000;
        while((int)src - 0xb8000 <= 160 * 24)
        {
            *src++ = *(src + 80);
        }

        video -= 80;
    }
}

/* Gibt eine Zeichenkette aus */
static void kputs(char * s)
{
    while(*s)
    {
        kputc(*s++);
    }
}

/* Dividiert ein uint64 durch einen uint32 und gibt das Ergebnis zurck */
unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
    unsigned int highword = dividend >> 32;
    unsigned int lowword = dividend & 0xffffffff;
    unsigned long long quotient;
    unsigned int rem;

    __asm__("div %%ecx\n\t"
        "xchg %%ebx, %%eax\n\t"
        "div %%ecx\n\t"
        "xchg %%edx, %%ebx"
        : "=A"(quotient), "=b"(rem)
        : "a"(highword), "b"(lowword), "c"(divisor), "d"(0)
        );

    if(remainder)
    {
        *remainder = rem;
    }

    return quotient;
}

/* Gibt eine unsigned 64-bit Zahl mit beliebiger Basis zwischen 2 und 36 aus */
static void kputn(unsigned long long x, int radix, int pad, char padchar)
{
    char b[65];
    char * r = b + 64; // r zeig auf das letzte Zeichen von b[]
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned int remainder;

    if(radix < 2 || radix > 36)
    {
        return;
    }

    *r-- = 0;

    do
    {
        x = divmod(x, radix, &remainder);
        *r-- = digits[remainder];
        pad--;
    }
    while(x > 0);

    while(pad-- > 0)
    {
        kputc(padchar);
    }

    kputs(r + 1);
}

/* Hilfsfunktion für kprintf() */
void kaprintf(char * format, int ** args)
{
    char * format2;
    int pad;
    signed long long value;
    char padfill;

    while(*format)
    {
        if(*format == '%')
        {
            format++;

            pad = 0;
            if(*format == '0')
            {
                padfill = '0';
                format++;
            }
            else
            {
                padfill = ' ';
            }

            while(*format >= '0' && *format <= '9')
            {
                pad = pad * 10 + *format++ - '0';
            }

            if(format[0] == 'l' && format[1] == 'l' && (format[2] == 'd' || format[2] == 'i' || format[2] == 'o' || format[2] == 'u' || format[2] == 'x'))
            {
                value = *(unsigned long long*)(*args);
                *args += 2;

                format += 2;
            }
            else if(format[0] == 'd' || format[0] == 'u')
            {
                value = *(*args)++;
                if(value < 0)
                {
                    kputc('-');
                    pad--;
                    value = -value;
                }
            }
            else if(format[0] == 'i' || format[0] == 'o' || format[0] == 'p' || format[0] == 'x')
            {
                value = *(*args)++;
                value = value & 0xffffffff;
            }

            switch(*format)
            {
                case 0:
                    return;
                case '%':
                    kputc('%');
                    break;
                case 'c':
                    kputc(*(*args)++);
                    break;
                case 'd':
                case 'i':
                case 'u':
                    kputn(value, 10, pad, padfill);
                    break;
                case 'o':
                    kputn(value, 8, pad, padfill);
                    break;
                case 'p':
                case 'x':
                    kputn(value, 16, pad, padfill);
                    break;
                /* Folgendes wieder einkommentieren um das extrem coole
                   Feature "rekursives printf" auszuprobieren */
                /*case 'r':
                    format2 = (char*)*(*args)++;
                    kaprintf(format2, args);
                    break;*/
                case 's':
                    kputs((char*)*(*args)++);
                    break;
                default:
                    kputc('%');
                    kputc(*format);
                    break;
            }
            format++;
        }
        else
        {
            kputc(*format++);
        }
    }
}

/* printf für den Kernel. Nur für Testzwecke gedacht.
 Unterstützt %c, %d, %i, %o, %p, %u, %s, %x und %lld, %lli, %llo, %llu, %llx. */
void kprintf(char * format, ...)
{
    int * args = ((int*)&format) + 1;
    kaprintf(format, &args);
}

