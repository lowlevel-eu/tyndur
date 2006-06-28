/*
 * Die funktionen memcpy und memset
 *
 * Aenderungen:
 *
 * Wann???: Wer??
 *   + Erste Version erstellt
 * 2006-06-27: Damian
 *   + Funktion memset hinzugefügt
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
         /// Addresse    wie oft   was?

void * memset(void * addr, int len, char value)
{
	char *d = (char*)addr;
	while(len--)
	{
		// Code ausführen
		/// Würde dies funktionieren????
		*(d++) = value;
	}
	
	return addr;
}
