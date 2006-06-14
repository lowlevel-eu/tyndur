#include "ports.h"
static short * video = (short*)0xb8000;

/* Gibt ein Zeichen auf dem Bildschirm aus. */
static void kputc(char c)
{
    if(c == '\r')
    {
        video -= (((int)video - 0xb8000) % 160) / 2;
    }
    else if(c == '\n')
    {
        video += 80 - (((int)video - 0xb8000) % 160) / 2;
    }
    else
    {
        *video++ = c | 0x0700;
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



/* säubert den screen */
void clrscr()
{
	unsigned char *vidmem = (unsigned char *)0xB8000;
	const long size = 80*25;
	long loop;
	
	for (loop=0; loop<size; loop++)
	{
		*vidmem++ = 0;
		*vidmem++ = 0xF;
	}
	
	//outb(0x3D4, 14);
	outb(0x3D5, 0);
	outb(0x3D4, 15);
	outb(0x3D5, 0);
}


/* Gibt eine Zeichenkette aus */
static void kputs(char * s)
{
    while(*s)
    {
        kputc(*s++);
    }
}

/* Gibt eine Zahl mit beliebiger Basis zwischen 2 und 32 aus */
static void kputn(int x, int radix, int pad, char padchar)
{
    char b[33], * r = b + 33;
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    int minus = 0;

    if(radix < 2 || radix > 36)
    {
        return;
    }

    if(radix == 10 && x < 0)
    {
        minus = 1;
        x = -x;
        pad--;
    }

    *r-- = 0;

    if(x == 0)
    {
        *r-- = '0';
        pad--;
    }

    while(x > 0)
    {
        *r-- = digits[x % radix];
        x /= radix;
        pad--;
    }

    if(minus)
    {
        kputc('-');
    }

    while(pad-- > 0)
    {
        kputc(padchar);
    }

    kputs(r + 1);
}

/* Hilfsfunktion für kprintf() */
static void kaprintf(char * format, int ** args)
{
    char * format2;
    int pad;
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
                    kputn(*(*args)++, 10, pad, padfill);
                    break;
                case 'p':
                case 'x':
                    kputn(*(*args)++, 16, pad, padfill);
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

/* printf für den Kernel. Nur für test-Zwecke gedacht.
 Unterstützt %d, %i, %x, %s */
void kprintf(char * format, ...)
{
    int * args = ((int*)&format) + 1;
    kaprintf(format, &args);
}

