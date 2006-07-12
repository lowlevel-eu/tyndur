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
