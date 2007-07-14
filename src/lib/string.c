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

/*
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

    while(n && (*d++ = *s++)) {
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

size_t strlen(const char *s)
{
    size_t len = 0;
    
    while(*s++) {
        len++;
    }
    
    return len;
}
*/
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    
    while (*s++ && maxlen--) {
        len++;
    }

    return len;
}

/*
//Aus der PDCLIB
char* strstr(const char* s1, const char* s2)
{
    const char* p1 = s1;
    const char* p2;
    while(*s1)
    {
        p2 = s2;
        while(*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }
        if(!*p2)
        {
            return (char*) s1;
        }
        ++s1;
        p1 = s1;
    }
    return NULL;
}
*/

//Mit Erlaubnis aus lightOS kopiert
void itoa(unsigned int n, char *s, unsigned int base)
{
	char st[33];
	unsigned int i = 0;
	
	do
	{
		unsigned int t = n % base;
		if (t < 10)st[i++] = t + '0';
		else st[i++] = (t - 10) + 'A';
	} while((n /= base) > 0);
	st[i] = '\0';
	
	unsigned int y = 0;
	for (;i > 0;i--)
	{
		s[y] = st[i-1];
		y++;
	}
	s[y] = '\0';
}
unsigned int atoi(const char *s)
{
	size_t digit = 0;
	size_t highest = 0;
	size_t i;
    for (i=0;s[i] != '\0';i++)
	{
		if (s[i] < '0' ||
			s[i] > '9')break;
		if (highest == 0)highest = 1;
		else highest *= 10;
		digit++;
	}
	unsigned int result = 0;
	for (i=0;i < digit;i++)
	{
		result += (s[i] - '0') * highest;
		highest /= 10;
	}
	return result;
}
//

long atol(const char *str)
{
    if (!*str) {
        return 0;
    }
    int positive = 1;
    long result;
    int length = 0;
    //Vorzeichen
    if (*str == '-') {
        positive = 0;
        str++;
    } else if (*str == '+') {
        str++;
    }
    //Zeichenkette auswerten
    while (*str)
    {
        char c = *str;
        if (!c) {
            if (!length) {
                return 0;
            }
            return result;
        } else if ((c >= '0') && (c <= '9')) {
            length++;
            //Ziffer hinzufügen
            result = result * 10 + c - '0';
        } else {
            if (!length) {
                return 0;
            }
            return result;
        } 
        str++;
    }
}
