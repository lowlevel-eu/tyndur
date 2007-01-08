/*
 * Die funktionen memcpy und memset
 *
 * Aenderungen:
 *
 * Wann???: Jidder
 *   + Erste Version erstellt
 * 2006-06-27: Damian
 *   + Funktion memset hinzugefügt
 * 2006-07-12: Jidder
 *   ! Signatur von memset an den Standard angepasst
 */

#include "string.h"

void * memcpy(void * dest, const void * src, size_t num)
{
    char *d = (char*)dest, *s = (char*)src;

    while(num--)
    {
        *d++ = *s++;
    }

    return dest;
}
         /// Addresse   was    wie oft?

void * memset(void * addr, int value, size_t len)
{
	char *d = (char*)addr;
	while(len--)
	{
		// Code ausführen
		*d++ = value;
	}

	return addr;
}

void * memmove(void * dest, const void * src, size_t num)
{
    const char * s = src;
    char * d = dest;

    if(s > d)
    {
        while(num--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        d += num;
        s += num;
        while(num--)
        {
            *--d = *--s;
        }
    }
    
    return dest;
}

char * strcpy(char * dest, const char * src)
{
    char *d = (char*) dest, *s = (char*) src;

    while((*d++ = *s++));

    return dest;
}

char * strncpy(char * dest, const char * src, size_t n)
{
    char *d = (char*) dest, *s = (char*) src;

    while((*d++ = *s++) && n) {
        n--;
    }

    while (n--) {
        *d++ = '\0';
    }

    return dest;
}


int strcmp(const char *s1, const char *s2) 
{
    while ((*s1 == *s2) && *s1) {
        s1++; s2++;
    }

    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while ((*s1 == *s2) && *s1 && n--) {
        s1++; s2++;
    }

    return n ? (*s1 - *s2) : 0;
}
